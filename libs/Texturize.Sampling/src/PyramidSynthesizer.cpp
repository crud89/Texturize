#include "stdafx.h"

#include <sampling.hpp>
#include <tbb/tbb.h>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Pyramid Synthesizer implementation	                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

PyramidSynthesizer::PyramidSynthesizer(std::shared_ptr<ISearchIndex> catalog) :
	SynthesizerBase(std::move(catalog))
{
}

void PyramidSynthesizer::synthesize(int width, int height, Sample& result, const SynthesisSettings& config) const
{
	// The configuration must contain arguments for pyramidal synthesis.
	const PyramidSynthesisSettings* settings = dynamic_cast<const PyramidSynthesisSettings*>(&config);

	TEXTURIZE_ASSERT(settings != nullptr);							// The synthesis settings must be compatible.
	TEXTURIZE_ASSERT(settings->validate());							// The synthesis configuration must be valid.

	// Since synthesis works on pyramids, the pyramid resolutions are limited to integral PoT numbers.
	// If other sizes are provided, we perform synthsis to a larger sample and crop it afterwards.
	// Note that this most certainly removes the ability to tile the texture.
	int w(nextPoT(width)), h(nextPoT(height));

	// The sample size represents a square with a length euqal to the largest dimension.
	cv::Size sampleSize = w >= h ? cv::Size(w, w) : cv::Size(h, h);
	
	// Calculate the pyramid depth.
	int depth = log2(static_cast<uint32_t>(sampleSize.width));

	// The synthesis is performed within uv-space, so the result is initialized as two-channel sample (including 
	// channels u and v). It will be initialized with the seed coords of the exemplar, which represents the coordinates 
	// inside the first pyramid level. This can also be seen as "initial translation", meaning that it results in a 
	// linear shift within the actual synthesis result, if not set to zero.
	cv::Mat sample(1, 1, CV_32FC2);
	sample.at<cv::Vec2f>(0, 0) = config._seedCoords;

	// Get a state object to handle common synthesizer configuration.
	PyramidSynthesizerState state(*settings);

	// Perform synthesis on each pyramid level.
	for (int l(0); l < depth; ++l)
	{
		state.update(l, sample);
		this->synthesizeLevel(sample, state);
	}

	// Crop the result and return it.
	TEXTURIZE_ASSERT_DBG(sample.size().width >= width);
	TEXTURIZE_ASSERT_DBG(sample.size().height >= height);

	result = Sample(sample(cv::Rect(0, 0, width, height)));
}

void PyramidSynthesizer::synthesize(const cv::Size& size, Sample& result, const SynthesisSettings& config) const
{
	this->synthesize(size.width, size.height, result, config);
}

void PyramidSynthesizer::transferStyle(const Sample& target, Sample& result, const SynthesisSettings& config) const
{
	// The configuration must contain arguments for pyramidal synthesis.
	const PyramidSynthesisSettings* settings = dynamic_cast<const PyramidSynthesisSettings*>(&config);

	TEXTURIZE_ASSERT(settings != nullptr);							// The synthesis settings must be compatible.
	TEXTURIZE_ASSERT(settings->validate());							// The synthesis configuration must be valid.

	// Get a state object to handle common synthesizer configuration.
	PyramidSynthesizerState state(*settings);

	// Perform the style transfer.
	this->transferTo(target, result, state);
}

void PyramidSynthesizer::synthesizeLevel(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	// Start by upsampling the current result. This increases the current resolution by a factor of two into each dimension.
	this->upsample(sample, state);

	// Simply upsampling would lead to a simple tiled texture wall, so to introduce spatial randomness, shift each tile a 
	// little bit. This process is called jitter.
	this->jitter(sample, state);

	// Perform multiple correction passes, if synthesis has reached a certain threshold.
	// If the threshold has not been reached, report the progress - otherwise this is done for each sub-pass.
	if (state.level() < state.config()._correctionLevelThreshold)
	{
		state.config()._progressHandler.execute(state.level(), -1, sample);
		return;
	}

	for (unsigned int p(0); p < state.config()._correctionPasses; ++p)
	{
		this->correct(sample, state);
		state.config()._progressHandler.execute(state.level(), p, sample);
	}
}

void PyramidSynthesizer::upsample(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	TEXTURIZE_ASSERT_DBG(sample.channels() == 2);					// The sample must contain only two channels (representing image coordinates in UV space).
	TEXTURIZE_ASSERT_DBG(sample.depth() == CV_32F);					// The sample must use 32 bit floating point pixel values.

	// Create a new sample that is twice as large.
	cv::Mat upsample(sample.size() * 2, CV_32FC2);

	// One pixel within the current sample results in four pixels in the result sample.
	float spacing = state.getSpacing();

	for (int r = 0; r < sample.rows; ++r)
	for (int c = 0; c < sample.cols; ++c)
	{
		int row = 2 * r;
		int col = 2 * c;
		cv::Vec2f& coords = sample.at<cv::Vec2f>(cv::Point2i(c, r));

		upsample.at<cv::Vec2f>(cv::Point2i(col, row))			= this->scaleTexel(coords, cv::Vec2f(0.f, 0.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col + 1, row))		= this->scaleTexel(coords, cv::Vec2f(1.f, 0.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col, row + 1))		= this->scaleTexel(coords, cv::Vec2f(0.f, 1.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col + 1, row + 1))	= this->scaleTexel(coords, cv::Vec2f(1.f, 1.f), spacing);
	}

	// Send the temporary result to handlers.
	state.config()._feedbackHandler.execute("Upsampled", upsample);

	// Return the upsampled exemplar.
	sample = upsample;
}

void PyramidSynthesizer::jitter(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	TEXTURIZE_ASSERT_DBG(sample.channels() == 2);					// The sample must contain only two channels (representing image coordinates in UV space).
	TEXTURIZE_ASSERT_DBG(sample.depth() == CV_32F);					// The sample must use 32 bit floating point pixel values.

	// The jitter is a texel-wise operation that randomly shifts the texture coordinates around, based on a simple two-dimensional hash function.
	int level = state.level();

	for (int r = 0; r < sample.rows; ++r)
	for (int c = 0; c < sample.cols; ++c)
	{
		cv::Point2i point(c, r);
		cv::Vec2f& coords = sample.at<cv::Vec2f>(point);
		coords += this->translateTexel(point, state);

		// Wrap the coords, so that they are in a range (0.f, 1.f].
		Sample::wrapCoords(coords);
	}

	// Send the temporary result to handlers.
	state.config()._feedbackHandler.execute("Jittered", sample);
}

void PyramidSynthesizer::correct(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	// Get the total number of sub-passes. The number of passes must be executed along each axis.
	const unsigned int subPasses = state.config()._correctionSubPasses;
	const unsigned int totalSubPasses = subPasses * subPasses;
	const unsigned int width = sample.cols, height = sample.rows;
	std::shared_ptr<IDescriptorExtractor> descriptorExtractor = _catalog->getDescriptorExtractor();
	
	// Request a reference of the exemplar.
	std::shared_ptr<const Sample> exemplar;
	_catalog->getSearchSpace()->sample(exemplar);

	// Get the guidance channel map for the current scale and reshape it. After reshaping, the matrix contains one row that can be appended to the descriptors.
	std::optional<Sample> guidanceDescriptors;

	if (state.config()._guidanceMap.has_value()) {
		cv::Mat guidanceMap = (cv::Mat)state.config()._guidanceMap.value();
		cv::resize(guidanceMap, guidanceMap, cv::Size(width, height));
		guidanceDescriptors = Sample(guidanceMap.reshape(guidanceMap.channels(), 1));
	}

	// Apply each sub-pass subsequently.
	for (unsigned int sp(0); sp < totalSubPasses; ++sp) {
		// Get the neighborhood descriptors for the current sub-pass. The descriptors are rebuild for each sub-pass, so that the sample converges against the expected result.
		cv::Mat descriptors = descriptorExtractor->calculateNeighborhoodDescriptors(*exemplar, sample).t();

		// Append guidance channels.
		if (guidanceDescriptors.has_value())
			for (int cn(0); cn < guidanceDescriptors.value().channels(); ++cn)
				descriptors.push_back(guidanceDescriptors.value().getChannel(cn));

		descriptors = descriptors.t();

		for (int r = 0; r < sample.rows; ++r)
		for (int c = 0; c < sample.cols; ++c) {
			// Check if the pixel should be corrected within the current sub-pass.
			int col = c % subPasses;
			int row = r % subPasses;
			int subPass = row * subPasses + col;

			// Only correct the pixel, if it should be done within this sub-pass.
			if (subPass != sp)
				continue;

			// Match the descriptor with the search space.
			cv::Point2i point(c, r);
			cv::Vec2f newCoords;
			cv::Vec2f& coords = sample.at<cv::Vec2f>(point);

			if (_catalog->findNearestNeighbor(descriptors, sample, point, newCoords))
				coords = newCoords;
		}

		// Send the temporary result to handlers.
		state.config()._feedbackHandler.execute("Corrected", sample);
	}
}

cv::Vec2f PyramidSynthesizer::scaleTexel(const cv::Vec2f& uv, const cv::Vec2f& delta, float spacing) const
{
	// Factor `2.f` results from upsampled (row = 2 * currRow).
	cv::Vec2f r = (2.f * uv) + (spacing * delta);
	//cv::Vec2f r = uv + (spacing * delta);
	Sample::wrapCoords(r);

	return r;
}

cv::Vec2f PyramidSynthesizer::translateTexel(const cv::Point2i& at, const PyramidSynthesizerState& state) const
{
	float spacing = state.getSpacing();
	float randomness = state.getRandomness(state.level());

	// NOTE: state.getHash() returns a quantized vector H:Z² -> [-1, 1]². The implicit cast happening here produces non-null
	// vectors during the calculation of `t`, which would not be possible otherwise.
	cv::Vec2f o = state.getHash()->calculate(at.x, at.y);
	cv::Vec2f t = (spacing * o * randomness);
	//t += cv::Vec2f(0.5f, 0.5f);

	return t;
}

std::vector<float> PyramidSynthesizer::getNeighborhoodDescriptor(const Sample* exemplar, const cv::Mat& sample, const cv::Point2i& uv, int k, bool weight) const
{	
	// Calculate the kernel extent.
	TEXTURIZE_ASSERT_DBG(k % 2 == 1);
	int extent = (k - 1) / 2, w(0), wx(0), wy(0);
	std::vector<float> neighborhood = std::vector<float>(k  * k  * exemplar->channels());
	cv::Mat kernel = weight ? cv::getGaussianKernel(k, 0, CV_32F) : cv::Mat::ones(k, k, CV_32F);

	for (int _x(-extent); _x <= extent; ++_x)
	{
		for (int _y(-extent); _y <= extent; ++_y)
		{
			// Calculate the weight of the pixel values.
			float g = weight ? kernel.at<float>(_x + extent) * kernel.at<float>(_y + extent) : 1.f;

			// Calculate the pixel coordinates.
			wx = uv.x + _x; wy = uv.y + _y;
			wx += wx < 0 ? sample.cols : 0;
			wx -= wx >= sample.cols ? sample.cols : 0;
			wy += wy < 0 ? sample.rows : 0;
			wy -= wy >= sample.rows ? sample.rows : 0;
			cv::Vec2f coords = sample.at<cv::Vec2f>(cv::Point2i(wx, wy));

			// Get the pixel value.
			std::vector<float> val;
			exemplar->at(coords, val);

			// Store the value.
			for each (const float v in val)
				neighborhood[w++] = g * v;
		}
	}

	// Return the neighborhood descriptor.
	return neighborhood;
}

void PyramidSynthesizer::transferTo(const Sample& target, Sample& result, const PyramidSynthesizerState& state) const
{
	const unsigned int subPasses = state.config()._correctionSubPasses;
	const unsigned int totalSubPasses = subPasses * subPasses;
	std::shared_ptr<IDescriptorExtractor> descriptorExtractor = _catalog->getDescriptorExtractor();

	// Initialize a matrix that will contain the result.
	cv::Mat sample(target.size(), CV_32FC2);
	const int width = target.width(), height = target.height();

	sample.forEach<cv::Vec2f>([&width, &height](cv::Vec2f& uv, const int* idx) -> void {
		uv[0] = static_cast<float>(idx[1]) / static_cast<float>(width);
		uv[1] = static_cast<float>(idx[0]) / static_cast<float>(height);
	});

	// Transform the target into the search space.
	Sample targetSpace;
	_catalog->getSearchSpace()->transform(target, targetSpace);

	// For each pixel in the sample, lookup the best match.
	// NOTE: This is similar to an initial correction pass.
	for (unsigned int sp(0); sp < totalSubPasses; ++sp)
	{
		// Get the neighborhood descriptors of the target space.
		const cv::Mat descriptors = descriptorExtractor->calculateNeighborhoodDescriptors(targetSpace);

		for (int r = 0; r < sample.rows; ++r)
		for (int c = 0; c < sample.cols; ++c)
		{
			// Check if the pixel should be corrected within the current sub-pass.
			int col = c % subPasses;
			int row = r % subPasses;
			int subPass = row * subPasses + col;

			// Only correct the pixel, if it should be done within this sub-pass.
			if (subPass != sp)
				continue;

			// Match the descriptor with the search space.
			cv::Point2i point(c, r);
			cv::Vec2f newCoords;
			cv::Vec2f& coords = sample.at<cv::Vec2f>(point);

			// Get the descriptor at the current location.
			_catalog->findNearestNeighbor(descriptors, sample, point, newCoords);
			coords = newCoords;
		}

		// TODO: Run correction passes.
		state.config()._feedbackHandler.execute("Status", sample);
	}

	// Send the temporary result to handlers.
	state.config()._feedbackHandler.execute("Transferred", sample);
	result = Sample(sample);
}

std::unique_ptr<SynthesizerBase> PyramidSynthesizer::createSynthesizer(std::shared_ptr<ISearchIndex> catalog)
{
	return std::unique_ptr<PyramidSynthesizer>(new PyramidSynthesizer(catalog));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Parallel Pyramid Synthesizer implementation	                                          /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void ParallelPyramidSynthesizer::upsample(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	TEXTURIZE_ASSERT_DBG(sample.channels() == 2);					// The sample must contain only two channels (representing image coordinates in UV space).
	TEXTURIZE_ASSERT_DBG(sample.depth() == CV_32F);					// The sample must use 32 bit floating point pixel values.

	// Create a new sample that is twice as large.
	cv::Mat upsample(sample.size() * 2, CV_32FC2);

	// One pixel within the current sample results in four pixels in the result sample.
	float spacing = state.getSpacing();

	sample.forEach<cv::Vec2f>([this, spacing, &upsample](const cv::Vec2f& coords, const int* idx) -> void {
		int row = 2 * idx[0];
		int col = 2 * idx[1];
		
		upsample.at<cv::Vec2f>(cv::Point2i(col, row))			= this->scaleTexel(coords, cv::Vec2f(0.f, 0.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col + 1, row))		= this->scaleTexel(coords, cv::Vec2f(1.f, 0.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col, row + 1))		= this->scaleTexel(coords, cv::Vec2f(0.f, 1.f), spacing);
		upsample.at<cv::Vec2f>(cv::Point2i(col + 1, row + 1))	= this->scaleTexel(coords, cv::Vec2f(1.f, 1.f), spacing);
	});

	// Return the upsampled exemplar.
	sample = upsample;

	// Send the temporary result to handlers.
	state.config()._feedbackHandler.execute("Upsampled", sample);
}

void ParallelPyramidSynthesizer::jitter(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	TEXTURIZE_ASSERT_DBG(sample.channels() == 2);					// The sample must contain only two channels (representing image coordinates in UV space).
	TEXTURIZE_ASSERT_DBG(sample.depth() == CV_32F);					// The sample must use 32 bit floating point pixel values.

	// The jitter is a texel-wise operation that randomly shifts the texture coordinates around, based on a simple two-dimensional hash function.
	int level = state.level();
	
	sample.forEach<cv::Vec2f>([this, &state, level](cv::Vec2f& coords, const int* idx) -> void {
		coords += this->translateTexel(cv::Point2i(idx[1], idx[0]), state);

		// Wrap the coords, so that they are in a range (0.f, 1.f].
		Sample::wrapCoords(coords);
	});

	// Send the temporary result to handlers.
	state.config()._feedbackHandler.execute("Jittered", sample);
}

void ParallelPyramidSynthesizer::correct(cv::Mat& sample, const PyramidSynthesizerState& state) const
{
	// Get the total number of sub-passes. The number of passes must be executed along each axis.
	const unsigned int subPasses = state.config()._correctionSubPasses;
	const unsigned int totalSubPasses = subPasses * subPasses;
	const unsigned int width = sample.cols, height = sample.rows;

	std::shared_ptr<ISearchIndex> searchIndex = _catalog;
	std::shared_ptr<IDescriptorExtractor> descriptorExtractor = searchIndex->getDescriptorExtractor();

	// Request a reference of the exemplar.
	std::shared_ptr<const Sample> exemplar;
	searchIndex->getSearchSpace()->sample(exemplar);

	// Get the guidance channel map for the current scale and reshape it. After reshaping, the matrix contains one row that can be appended to the descriptors.
	std::optional<Sample> guidanceDescriptors;

	if (state.config()._guidanceMap.has_value()) {
		cv::Mat guidanceMap = (cv::Mat)state.config()._guidanceMap.value();
		cv::resize(guidanceMap, guidanceMap, cv::Size(width, height));
		guidanceDescriptors = Sample(guidanceMap.reshape(guidanceMap.channels(), 1));
	}

	// Apply each sub-pass subsequently.
	for (unsigned int sp(0); sp < totalSubPasses; ++sp)
	{
		// Get the neighborhood descriptors for the current sub-pass. The descriptors are rebuild for each sub-pass, so that the sample converges against the expected result.
		cv::Mat descriptors = descriptorExtractor->calculateNeighborhoodDescriptors(*exemplar, sample).t();

		// Append guidance channels.
		if (guidanceDescriptors.has_value())
			for (int cn(0); cn < guidanceDescriptors.value().channels(); ++cn)
				descriptors.push_back(guidanceDescriptors.value().getChannel(cn));

		descriptors = descriptors.t();

		sample.forEach<cv::Vec2f>([&sample, &searchIndex, &descriptors, &sp, &subPasses, &width, &height](cv::Vec2f& coords, const int* idx) -> void {
			// Check if the pixel should be corrected within the current sub-pass.
			int col = idx[1] % subPasses;
			int row = idx[0] % subPasses;
			int subPass = row * subPasses + col;

			// Only correct the pixel, if it should be done within this sub-pass.
			if (subPass != sp)
				return;

			// Match the descriptor with the search space.
			cv::Vec2f newCoords;

			if (searchIndex->findNearestNeighbor(descriptors, sample, cv::Point2i(idx[1], idx[0]), newCoords))
				coords = newCoords;
		});

		// Send the temporary result to handlers.
		state.config()._feedbackHandler.execute("Corrected", sample);
	}
}

void ParallelPyramidSynthesizer::transferTo(const Sample& target, Sample& result, const PyramidSynthesizerState& state) const
{
	std::shared_ptr<ISearchIndex> searchIndex = _catalog;
	std::shared_ptr<IDescriptorExtractor> descriptorExtractor = searchIndex->getDescriptorExtractor();

	const unsigned int subPasses = state.config()._correctionSubPasses;
	const unsigned int totalSubPasses = subPasses * subPasses;

	// Initialize a matrix that will contain the result.
	cv::Mat sample(target.size(), CV_32FC2);
	const int width = target.width(), height = target.height();

	sample.forEach<cv::Vec2f>([&width, &height](cv::Vec2f& uv, const int* idx) -> void {
		uv[0] = static_cast<float>(idx[1]) / static_cast<float>(width);
		uv[1] = static_cast<float>(idx[0]) / static_cast<float>(height);
	});

	//// Transform the target into the search space.
	Sample targetSpace;
	searchIndex->getSearchSpace()->transform(target, targetSpace);

	// For each pixel in the sample, lookup the best match.
	// NOTE: This is similar to an initial correction pass.
	for (unsigned int sp(0); sp < totalSubPasses; ++sp)
	{
		// Get the neighborhood descriptors of the target space.
		const cv::Mat descriptors = descriptorExtractor->calculateNeighborhoodDescriptors(targetSpace);

		sample.forEach<cv::Vec2f>([&sample, &searchIndex, &descriptors , &sp, &subPasses](cv::Vec2f& coords, const int* idx) -> void {
			// Check if the pixel should be corrected within the current sub-pass.
			int col = idx[1] % subPasses;
			int row = idx[0] % subPasses;
			int subPass = row * subPasses + col;

			// Only correct the pixel, if it should be done within this sub-pass.
			if (subPass != sp)
				return;

			// Get the descriptor at the current location.
			searchIndex->findNearestNeighbor(descriptors, sample, cv::Point2i(idx[1], idx[0]), coords);
		});

		// TODO: Run correction passes.
		//state.config()._feedbackHandler.execute("Status", sample);
	}

	// Send the temporary result to handlers.
	//state.config()._feedbackHandler.execute("Transferred", sample);
	result = Sample(sample);
}

std::unique_ptr<SynthesizerBase> ParallelPyramidSynthesizer::createSynthesizer(std::shared_ptr<ISearchIndex> catalog)
{
	return std::unique_ptr<ParallelPyramidSynthesizer>(new ParallelPyramidSynthesizer(std::move(catalog)));
}