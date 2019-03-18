#include "stdafx.h"

#include <sampling.hpp>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex base interface                                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SearchIndex::SearchIndex(const std::shared_ptr<ISearchSpace> searchSpace, const std::shared_ptr<IDescriptorExtractor> descriptorExtractor, cv::NormTypes normType) :
	_searchSpace(std::move(searchSpace)), _descriptorExtractor(std::move(descriptorExtractor)), _normType(normType)
{
	TEXTURIZE_ASSERT(searchSpace != nullptr);
	TEXTURIZE_ASSERT(descriptorExtractor != nullptr);
}

std::shared_ptr<ISearchSpace> SearchIndex::getSearchSpace() const
{
	return _searchSpace;
}

std::shared_ptr<IDescriptorExtractor> SearchIndex::getDescriptorExtractor() const
{
	return _descriptorExtractor;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// FLANN-based ANN search index implementation                                             /////
///////////////////////////////////////////////////////////////////////////////////////////////////

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	SearchIndex(searchSpace, descriptorExtractor, normType)
{
	this->init(*indexParams);
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	ANNIndex(searchSpace, std::make_unique<PCADescriptorExtractor>(), indexParams, normType)
{
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& weightMap, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	SearchIndex(searchSpace, descriptorExtractor, normType)
{
	this->init(*indexParams, weightMap);
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& weightMap, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	ANNIndex(searchSpace, std::make_unique<PCADescriptorExtractor>(), weightMap, indexParams, normType)
{
}

void ANNIndex::init(const cv::flann::IndexParams& indexParams)
{
	this->init(indexParams, { });
}

void ANNIndex::init(const cv::flann::IndexParams& indexParams, std::optional<std::reference_wrapper<const Sample>> weightMap)
{
	// Precompute the neighborhood descriptors used to train data.
	std::shared_ptr<const Sample> sample;
	_searchSpace->sample(sample);

	int height, weightChannels(-1);
	sample->getSize(_sampleWidth, height);

	// Form a descriptor vector from the sample.
	_descriptors = _descriptorExtractor->calculateNeighborhoodDescriptors(*sample);
	TEXTURIZE_ASSERT(_descriptors.isContinuous());

	// Check if weights need to be added; if so, append them to the descriptors.
	if (weightMap.has_value()) {
		// Validate the weight map pixel count against the number of descriptors.
		auto map = weightMap.value().get();
		TEXTURIZE_ASSERT(_descriptors.rows == (map.height() * map.width()));
		weightChannels = map.channels();

		// Transpose descriptors, so that each column represents one descriptor.
		_descriptors = _descriptors.t();
		
		// From each channel, append the values to the descriptors.
		for (int cn(0); cn < weightChannels; ++cn)
			_descriptors.push_back(map.getChannel(cn).reshape(1, 1));

		// Again, transpose the descriptors, so that each row holds on descriptor.
		_descriptors = _descriptors.t();
	}

	// Create the index instance and initialize it.
	MatrixType dataset((ElementType*)_descriptors.data, _descriptors.rows, _descriptors.cols);
	_index = std::make_unique<IndexType>(dataset, *(cvflann::IndexParams*)(indexParams.params), DistanceType(weightChannels));
	_index->buildIndex();
}

bool ANNIndex::findNearestNeighbor(const std::vector<float>& descriptor, cv::Vec2f& match, float minDist, float* dist) const
{
	std::vector<cv::Vec2f> matches;
	std::vector<float> distances;

	// Find the k=1 nearest neighbor.
	if (!this->findNearestNeighbors(descriptor, matches, 1, minDist, &distances))
		return false;

	// Return the match...
	match = matches[0];

	// ... and the distance, if required.
	if (dist)
		*dist = distances[0];

	return true;
}

bool ANNIndex::findNearestNeighbors(const std::vector<float>& descriptor, std::vector<cv::Vec2f>& mtx, const int k, float minDist, std::vector<float>* dist) const
{
	typedef typename ANNIndex::_L2Ex<float>     Distance;
	typedef typename Distance::ElementType      ElementType;
	typedef typename Distance::ResultType       DistanceType;

	// NOTE: One descriptor per row.
	cv::InputArray _query = descriptor;
	cv::Mat indices(1, k, CV_32SC1);
	cv::Mat distances(1, k, CV_32FC1);
	cv::Mat query = _query.getMat();

	TEXTURIZE_ASSERT(static_cast<size_t>(k) <= _index->size());
	TEXTURIZE_ASSERT(query.type() == cv::DataType<ElementType>::type);
	TEXTURIZE_ASSERT(indices.type() == CV_32S);
	TEXTURIZE_ASSERT(distances.type() == cv::DataType<DistanceType>::type);
	TEXTURIZE_ASSERT(query.isContinuous() && indices.isContinuous() && distances.isContinuous());

	::cvflann::Matrix<ElementType> q((ElementType*)query.data, query.rows, query.cols);
	::cvflann::Matrix<int> i(indices.ptr<int>(), indices.rows, indices.cols);
	::cvflann::Matrix<DistanceType> d(distances.ptr<DistanceType>(), distances.rows, distances.cols);

	//_index->knnSearch(descriptor, indices, distances, k);
	_index->knnSearch(q, i, d, k, ::cvflann::SearchParams());

	// Retain and store the pixel coordinates and distances for each match.
	for (int _k(0); _k < k; ++_k)
	{
		cv::Vec2f match;

		int idx = indices.at<int>(0, _k);
		match[0] = static_cast<float>(idx % _sampleWidth) / static_cast<float>(_sampleWidth);
		match[1] = static_cast<float>(idx / _sampleWidth) / static_cast<float>(_sampleWidth);

		mtx.push_back(match);

		if (dist)
			dist->push_back(distances.at<float>(0, _k));
	}

	return true;
}

bool ANNIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist, float* dist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbor(targetDescriptor, match, minDist, dist);
}

bool ANNIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k, float minDist, std::vector<float>* dist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbors(targetDescriptor, matches, k, minDist, dist);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// KD-Tree-based search index implementation                                               /////
///////////////////////////////////////////////////////////////////////////////////////////////////

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor) :
	ANNIndex(searchSpace, descriptorExtractor, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace) :
	ANNIndex(searchSpace, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& weightMap) :
	ANNIndex(searchSpace, descriptorExtractor, weightMap, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& weightMap) :
	ANNIndex(searchSpace, weightMap, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex implementation based on coherent pixels.                                    /////
///////////////////////////////////////////////////////////////////////////////////////////////////

CoherentIndex::CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace) :
	SearchIndex(searchSpace, std::make_unique<PCADescriptorExtractor>())
{
	this->init();
}

CoherentIndex::CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap) :
	SearchIndex(searchSpace, std::make_unique<PCADescriptorExtractor>()), _guidanceMap(guidanceMap)
{
	this->init();
}

void CoherentIndex::init()
{
	// Precompute the neighborhood descriptors used to train data.
	std::shared_ptr<const Sample> sample;
	_searchSpace->sample(sample);

	// Form a descriptor vector from the sample.
	_exemplarDescriptors = _descriptorExtractor->calculateNeighborhoodDescriptors(*sample);
}

CoherentIndex::TDistance CoherentIndex::measureVisualDistance(const std::vector<float>& targetDescriptor, const cv::Point2i& towards) const
{
	// Get the search space transformed exemplar.
	std::shared_ptr<const Sample> exemplar;
	_searchSpace->sample(exemplar);

	std::vector<float> exemplarDescriptor = _exemplarDescriptors.row(towards.y * exemplar->width() + towards.x);

	// Only compare the dimensions of the target descriptor, that are also present in the exemplar descriptor.
	std::vector<float> descriptor(&targetDescriptor[0], &targetDescriptor[exemplarDescriptor.size()]);

	// Measure the visual distance based on the provided norm type (default is sum of squared distances).
	TEXTURIZE_ASSERT(exemplarDescriptor.size() == descriptor.size());

	// Compute the guidance weight.
	float weight(0.f);
	Sample::Texel texel;
	_guidanceMap.getNeighborhood(towards, 1, texel);

	for (size_t cn(exemplarDescriptor.size()); cn < targetDescriptor.size(); ++cn)
		weight += std::abs(texel[cn - exemplarDescriptor.size()] - targetDescriptor[cn]);

	weight /= static_cast<float>(targetDescriptor.size() - exemplarDescriptor.size());
	return static_cast<float>(cv::norm(descriptor, exemplarDescriptor, _normType)) * weight;
}

void CoherentIndex::getCoherentCandidate(const std::vector<float>& targetDescriptor, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta, TMatch& match) const
{
	// Get the search space transformed exemplar.
	std::shared_ptr<const Sample> exemplar;
	_searchSpace->sample(exemplar);
	int width = exemplar->width();
	int height = exemplar->height();

	// From the sample neighbor described by the UV coordinates and the delta, retrieve the respective exemplar pixel.
	cv::Point2i uvPos(at.x + delta[0], at.y + delta[1]);
	Sample::wrapCoords(uv.cols, uv.rows, uvPos);
	cv::Vec2f exemplarCoords = uv.at<cv::Vec2f>(uvPos);

	// Calculate the pixel position in the exemplar from the UV coordinates.
	cv::Point2i exemplarPos;
	exemplarPos.x = static_cast<int>(roundf(exemplarCoords[0] * static_cast<float>(width))) - delta[0];
	exemplarPos.y = static_cast<int>(roundf(exemplarCoords[1] * static_cast<float>(height))) - delta[1];
	Sample::wrapCoords(width, height, exemplarPos);

	// Compare the exemplar descriptor to the target descriptor.
	TDistance distance = this->measureVisualDistance(targetDescriptor, exemplarPos);

	// Create and return a match instance.
	std::get<0>(match) = cv::Vec2f(static_cast<float>(exemplarPos.x) / static_cast<float>(exemplar->width()), static_cast<float>(exemplarPos.y) / static_cast<float>(exemplar->height()));
	std::get<1>(match) = distance;
}

bool CoherentIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist, float* dist) const
{
	// Get the target descriptor in order to calculate the distance later on.
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);
	
	// Store the best coherent candidate of the exemplar, retrieved from the neighborhood of the pixel in the sample.
	TMatch bestCandidate(cv::Vec2f(0.f, 0.f), -1.f);

	// For each neighboring pixel, request the coherent candidate coordinates.
	for (int x(-1); x <= 1; ++x)
	for (int y(-1); y <= 1; ++y)
	{
		// if (x == 0 && y == 0) continue;

		// Get the neighborhood descriptor for the pixel within the exemplar.
		TMatch candidateMatch;
		this->getCoherentCandidate(targetDescriptor, uv, at, cv::Vec2i(x, y), candidateMatch);

		// Only return matches, if they are further away than the provided minimum distance within the exemplar (measured by euclidean distance).
		cv::Vec2f originalPos = uv.at<cv::Vec2f>(at);
		cv::Vec2f candidatePos(std::get<0>(candidateMatch));
		float distance = cv::norm(originalPos - candidatePos);

		if (distance < minDist)
			continue;

		// Reset the current best match, if the new one is closer.
		if (std::get<1>(bestCandidate) < 0.f || std::get<1>(candidateMatch) < std::get<1>(bestCandidate))
			bestCandidate = candidateMatch;
	}

	// If no fitting match has been found, there's nothing to return.
	if (std::get<1>(bestCandidate) < 0.f)
		return false;

	// Return the best candidate match.
	match = std::get<0>(bestCandidate);
	
	if (dist)
		*dist = std::get<1>(bestCandidate);

	return true;
}

bool CoherentIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k, float minDist, std::vector<float>* dist) const
{
	// Get the target descriptor in order to calculate the distance later on.
	// NOTE: The descriptors are indexed by their UV-coordinates (i.e. one descriptor for each point in UV-space).
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.rows + at.x);

	// Same as above; Return k candidates (remove duplicates, so n can be < k)
	// Construct a list of coherent candidates in the exemplar from the neighborhood of the pixel in the sample.
	std::vector<TMatch> candidates;

	// For each neighboring pixel, request the coherent candidate coordinates.
	for (int x(-1); x <= 1; ++x)
	for (int y(-1); y <= 1; ++y)
	{
		// if (x == 0 && y == 0) continue;

		// Get the neighborhood descriptor for the pixel within the exemplar.
		TMatch candidateMatch;
		this->getCoherentCandidate(targetDescriptor, uv, at, cv::Vec2i(x, y), candidateMatch);

		// Only return matches, if they are further away than the provided minimum distance within the exemplar (measured by euclidean distance).
		cv::Vec2f originalPos = uv.at<cv::Vec2f>(at);
		cv::Vec2f candidatePos(std::get<0>(candidateMatch));
		float distance = cv::norm(originalPos - candidatePos);

		if (distance < minDist)
			continue;

		// Reset the current best match, if the new one is closer.
		float visualDistance = std::get<1>(candidateMatch);

		// Reverse traverse the candidates until we found one that is visually closer to the current candidate.
		for (auto c = candidates.size() - 1; c >= 0; --c)
		{
			auto candidate = candidates[c];
			
			if (std::get<1>(candidate) < visualDistance)
			{
				candidates.insert(candidates.begin() + c, candidateMatch);
				break;
			}
		}
	}

	// If no fitting match has been found, there's nothing to return.
	if (candidates.size() == 0)
		return false;

	// Return the best candidates.
	matches.clear();
	
	if (dist)
		dist->clear();

	for each(auto candidate in candidates)
	{
		matches.push_back(std::get<0>(candidate));

		if (dist)
			dist->push_back(std::get<1>(candidate));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex implementation based on random pixel walk.                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

RandomWalkIndex::RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace) :
	CoherentIndex(searchSpace)
{
	std::random_device seed;
	_rng = std::mt19937(seed());
}

RandomWalkIndex::RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap) :
	CoherentIndex(searchSpace, guidanceMap)
{
	std::random_device seed;
	_rng = std::mt19937(seed());
}

cv::Vec2f RandomWalkIndex::getRandomPixelAround(const cv::Vec2f& point, int radius, int dominantDimensionExtent) const
{
	return this->getRandomPixelAround(point, static_cast<float>(radius) / static_cast<float>(dominantDimensionExtent));
}

cv::Vec2f RandomWalkIndex::getRandomPixelAround(const cv::Vec2f& point, float radius) const
{
	std::uniform_real_distribution<float> distribution(-radius, radius);
	return cv::Vec2f(point[0] + distribution(_rng), point[1] + distribution(_rng));
}

bool RandomWalkIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist, float* dist) const
{
	// Get the target descriptor in order to calculate the distance later on.
	// NOTE: The descriptors are indexed by their UV-coordinates (i.e. one descriptor for each point in UV-space).
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.rows + at.x);

	// Perform a coherent search for the best candidate.
	TMatch candidate;

	if (!CoherentIndex::findNearestNeighbor(descriptors, uv, at, std::get<0>(candidate), minDist, &std::get<1>(candidate)))
		return false;

	// If the candidate is somewhere near the edge of the sample, do not search further. This is since candidates are not well defined at edges.
	uint32_t level = log2(static_cast<uint32_t>(uv.cols));
	uint32_t threshold = pow(2, level);

	if ((at.x > threshold && at.x < uv.cols - threshold) &&
		(at.y > threshold && at.y < uv.rows - threshold))
	{
		// Randomly walk around the environment of the match, trying to find a better one.
		// The radius around the pixel is calculated from the smaller dimension. Initially it is half as large as the exemplar width.
		std::shared_ptr<const Sample> exemplar;
		_searchSpace->sample(exemplar);

		// Perform as long, as the environment is non-trivial, i.e. there is an environment which does not only contain the candidate pixel.
		// The radius get's halved with each iteration.
		for (int radius(exemplar->width() >> 1); radius >= 2; radius >>= 1)
		{
			// Get a random point around the current candidate.
			cv::Vec2f pixel = this->getRandomPixelAround(std::get<0>(candidate), radius, exemplar->width());
			Sample::wrapCoords(pixel);

			// Correct the coordinates.
			cv::Point2i pixelCoords(static_cast<int>(pixel[0] * static_cast<float>(exemplar->width())), static_cast<int>(pixel[1] * static_cast<float>(exemplar->height())));

			// Measure the visual distance between both candidates.
			TDistance distance = this->measureVisualDistance(targetDescriptor, pixelCoords);

			// If the distance is lower than the one found within the coherent search, replace the candidate and continue.
			if (distance < std::get<1>(candidate))
			{
				std::get<0>(candidate) = pixel;
				std::get<1>(candidate) = distance;
			}
		}
	}

	// Return the distance and coordinates.
	match = std::get<0>(candidate);

	if (dist)
		*dist = std::get<1>(candidate);

	return true;
}

bool RandomWalkIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k, float minDist, std::vector<float>* dist) const
{
	// TODO: Implement me!
	return false;
}