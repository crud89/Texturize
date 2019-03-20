#include "stdafx.h"

#include <sampling.hpp>

#include <algorithm>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex implementation based on coherent pixels.                                    /////
///////////////////////////////////////////////////////////////////////////////////////////////////

CoherentIndex::CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace, const int k) :
	SearchIndex(searchSpace, std::make_unique<PCADescriptorExtractor>()), _candidatesPerDescriptor(k)
{
	std::random_device seed;
	_rng = std::mt19937(seed());

	this->init(k);
}

CoherentIndex::CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap, const int k) :
	SearchIndex(searchSpace, std::make_unique<PCADescriptorExtractor>()), _guidanceMap(guidanceMap), _candidatesPerDescriptor(k)
{
	std::random_device seed;
	_rng = std::mt19937(seed());

	this->init(k);
}

void CoherentIndex::init(const int& k)
{
	// Precompute the neighborhood descriptors used to train data.
	int kernel, kernelDev;
	std::shared_ptr<const Sample> sample;
	_searchSpace->sample(sample);
	_searchSpace->kernel(kernel);
	kernelDev = (kernel - 1) / 2;

	TEXTURIZE_ASSERT(sample->width() == sample->height());
	TEXTURIZE_ASSERT_DBG(kernel % 2 == 1);
	TEXTURIZE_ASSERT(k < kernel * kernel - 1);				// k should be smaller than the number of pixels inside the window around a pixel, excluding itself.

	// Get appearance space descriptors at each level.
	// TODO: min level should be the same as in config -> Need to pass this!
	const int sampleLevel = log2(sample->width());
	TEXTURIZE_ASSERT(sampleLevel >= 3);

	for (int level(0); level <= sampleLevel; ++level) {
		// Skip the first three levels. We do, however, require a descriptor and candidate set for those levels... which could also be reworked.
		if (level < 3) {
			_exemplarDescriptors.push_back(cv::Mat(0, 0, CV_32FC1));
			_candidates.push_back(cv::Mat(10, 0, CV_32S));
			continue;
		}

		// Blur the image for levels below the exemplar level.
		cv::Mat blurred = (cv::Mat)(*sample);
		double sigma = static_cast<double>(sampleLevel - level - 1);

		// NOTE: This should be the preferred implementation, since it is memory-constant. However, OpenCV does currently not allow
		//       BORDER_WRAP in it's cv::FilterEngine implementation, hence this workaround. See:
		//       - https://github.com/opencv/opencv/issues/4599
		//       - https://github.com/opencv/opencv/pull/10197
		//if (level < sampleLevel)
		//	cv::GaussianBlur(blurred, blurred, cv::Size(kernel, kernel), sigma, sigma, cv::BORDER_WRAP);
		if (level < sampleLevel) {
			cv::Rect mask(blurred.cols, blurred.rows, blurred.cols, blurred.rows);
			cv::Mat buffer;
			cv::repeat(blurred, 3, 3, buffer);
			cv::GaussianBlur(buffer, buffer, cv::Size(kernel, kernel), sigma);
			blurred = cv::Mat(buffer, mask);
		}

		// Form a descriptor vector from the sample at the current scale. Each row contains a descriptor.
		cv::Mat descriptors = _descriptorExtractor->calculateNeighborhoodDescriptors(Sample(blurred));

		// Store the descriptors at each level.
		_exemplarDescriptors.push_back(descriptors);

		// TODO: Discard border pixels here.

		// Now that the descriptors are extracted, compute a set of candidates from a neighborhood around 
		// each pixel. The neighborhood candidates are fetched from an increasing grid around it.
		int stride = sample->width() / static_cast<int>((std::pow<int>(2, level)));

		// Create a candidate array, where each row stores k candidate indices.
		cv::Mat candidates = cv::Mat::zeros(0, k, CV_32S);

		// Search distances for each descriptor.
		for (int r(0); r < descriptors.rows; ++r) {
			// Compute x and y coordinates from the row.
			int x{ r % sample->width() }, y{ r / sample->width() };

			// Collect indices of the neighborhood.
			std::vector<std::pair<int, double>> neighbors;

			for (int i(-kernelDev); i <= kernelDev; ++i)
			for (int j(-kernelDev); j <= kernelDev; ++j) {
				// Exclude the center pixel.
				if (i == 0 && j == 0)
					continue;

				// Compute the neighborhood coordinate index and distance.
				int cx{ x + (i * stride) }, cy{ y + (j * stride) };
				Sample::wrapCoords(sample->width(), sample->height(), cx, cy);

				// Get the index of the candidate and compute the distance.
				int index = cy * sample->width() + cx;
				double distance = cv::norm(descriptors.row(r), descriptors.row(index), _normType), guidanceDistance{ 0 };

				// If there are source guidance channels, also factor them in.
				if (_guidanceMap.has_value()) {
					Sample::Texel sourceGuidance, targetGuidance;
					_guidanceMap.value().at(cv::Point2i(x, y), sourceGuidance);
					_guidanceMap.value().at(cv::Point2i(cx, cy), targetGuidance);

					for (size_t i(0); i < sourceGuidance.size(); ++i)
						guidanceDistance += abs(sourceGuidance[i] - targetGuidance[i]);
				}

				neighbors.push_back(std::make_pair(index, guidanceDistance));
				//neighbors.push_back(std::make_pair(index, distance + guidanceDistance));
			}

			// Sort the indices, based on the distances.
			std::sort(neighbors.begin(), neighbors.end(), [](std::pair<int, double> lhs, std::pair<int, double> rhs) {
				return lhs.second < rhs.second;
			});

			// Keep the k candidates.
			std::vector<int> matches;

			for (int n(0); n < k; ++n)
				matches.push_back(neighbors[n].first);

			cv::Mat c = cv::Mat(matches).t();
			candidates.push_back(c);
		}

		// Store the candidate set for the current level.
		_candidates.push_back(candidates);
	}
}

void CoherentIndex::getCoherentCandidate(const cv::Point2i& exemplarCoords, int& candidate) const
{
	// Get the search space transformed exemplar.
	std::shared_ptr<const Sample> exemplar;
	_searchSpace->sample(exemplar);
	int width = exemplar->width();
	int height = exemplar->height();
	int level = log2(width);

	// Make sure the coords are valid.
	cv::Point2i coherentPos = exemplarCoords;
	Sample::wrapCoords(width, height, coherentPos);

	// Get the candidate indices at the position.
	std::vector<int> candidates;
	_candidates[level].row(coherentPos.y * width + coherentPos.x).copyTo(candidates);

	// Randomly select a candidate.
	std::uniform_int_distribution<int> dist(0, candidates.size() - 1);
	candidate = candidates[dist(_rng)];
}

void CoherentIndex::getCoherentCandidate(const cv::Point2i& exemplarCoords, const cv::Vec2i& delta, int& candidate) const
{
	// Resolve the delta and return the candidate.
	cv::Point2i coords(exemplarCoords.x + delta[0], exemplarCoords.y + delta[1]);

	this->getCoherentCandidate(coords, candidate);
}

cv::Mat CoherentIndex::getDescriptor(const int level, const int index) const
{
	TEXTURIZE_ASSERT(level > 0 && level < _exemplarDescriptors.size());
	TEXTURIZE_ASSERT(index > 0 && index < _exemplarDescriptors[level].rows);

	return _exemplarDescriptors[level].row(index);
}

bool CoherentIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, MatchType& match, DistanceType minDist) const
{
	// Call findNearestNeighbors with k = 1.
	std::vector<MatchType> matches;

	if (!this->findNearestNeighbors(descriptors, uv, at, matches, 1, minDist))
		return false;

	match = matches.front();
	return true;
}

bool CoherentIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<MatchType>& mtch, const int k, DistanceType minDist) const
{
	TEXTURIZE_ASSERT(uv.channels() == 2);                                      // The UV map should contain two channels, one for u and one for v coordinates.
	TEXTURIZE_ASSERT(uv.depth() == cv::DataType<CoordinateType>::type);        // The type of the uv map should match the position type.
	TEXTURIZE_ASSERT(k > 0 && k <= _candidatesPerDescriptor);                  // The number of candidates should be a non-zero, positive number, but also lower than k.

	// TODO: k > _candidatesPerDescriptor -> collect multiple candidates from one coherent pixel.

	// Get the search space transformed exemplar.
	std::shared_ptr<const Sample> exemplar;
	_searchSpace->sample(exemplar);
	CoordinateType width = static_cast<CoordinateType>(exemplar->width());
	CoordinateType height = static_cast<CoordinateType>(exemplar->height());
	int level = log2(exemplar->width());

	// For each neighboring pixel, request the coherent candidate coordinates.
	std::vector<int> candidates;

	for (int x(-1); x <= 1; ++x)
	for (int y(-1); y <= 1; ++y) {
		// Do not include self.
		if (x == 0 && y == 0)
			continue;

		// Get the uv coords at the position.
		PositionType uvCoords = uv.at<PositionType>(at);

		// Make them absolute.
		cv::Point2i coords = cv::Point2i(uvCoords[0] * width, uvCoords[1] * height);

		// Get a candidate for the neighbor and remember it.
		int candidate;
		this->getCoherentCandidate(coords, cv::Vec2i(x, y), candidate);
		candidates.push_back(candidate);

		//// Get the neighboring uv coords at the position and make them absolute
		//cv::Point2i coords(at.x + x, at.y + y);
		//Sample::wrapCoords(uv.cols, uv.rows, coords);
		//PositionType uvCoords = uv.at<PositionType>(coords);
		//coords = cv::Point2i(uvCoords[0] * width, uvCoords[1] * height);

		//// Get a candidate for the neighbor and remember it. 
		//// The coordinate shift in uvCoords is inversely applied in the exemplar to get the neighbor of the pixel 
		//// inside the exemplar and then search for candidates of this pixel.
		//int candidate;
		//this->getCoherentCandidate(coords, cv::Vec2i(-x, -y), candidate);
		//candidates.push_back(candidate);
	}

	// Get the descriptor for the current sample at the requested position.
	Sample::Texel targetGuidance, targetDescriptor = descriptors.row(at.y * uv.rows + at.x);

	// If there is a guidance map, ensure that all guidance channels are provided in the descriptor.
	if (_guidanceMap.has_value()) {
		int guidanceChannels = _exemplarDescriptors[level].cols;
		TEXTURIZE_ASSERT(targetDescriptor.size() == guidanceChannels + _guidanceMap.value().channels());

		// Furthermore, divide the target descriptor in two parts: The actual descriptor values and the appended guidance values.
		targetGuidance = std::vector<float>(targetDescriptor.begin() + guidanceChannels, targetDescriptor.end());
		targetDescriptor = std::vector<float>(targetDescriptor.begin(), targetDescriptor.begin() + guidanceChannels);
	}

	// Compare each candidate descriptor with target descriptor.
	std::vector<MatchType> matches;

	for each (const auto& candidate in candidates) {
		// Compute the position of the candidate.
		PositionType candidatePos(static_cast<CoordinateType>(candidate % exemplar->width()) / width, static_cast<CoordinateType>(candidate / exemplar->width()) / height);

		// Get the pixel descriptor of the pixel in the exemplar.
		Sample::Texel sourceGuidance, sourceDescriptor = _exemplarDescriptors[level].row(candidate);

		// Compute the distance between the descriptors.
		DistanceType distance = static_cast<DistanceType>(cv::norm(sourceDescriptor, targetDescriptor, _normType));

		// If a guidance map is provided, get the guidance channel values of the candidate and factor the distance 
		// between the guidance channels into the actual distance.
		if (_guidanceMap.has_value()) {
			// Get the source guidance channels.
			_guidanceMap.value().at(candidatePos, sourceGuidance);
			
			TEXTURIZE_ASSERT(sourceGuidance.size() == targetGuidance.size());

			// Get the distance between each channel.
			DistanceType guidanceDistance;

			for (size_t i(0); i < sourceGuidance.size(); ++i)
				guidanceDistance += abs(sourceGuidance[i] - targetGuidance[i]);

			distance = guidanceDistance;
			//distance += guidanceDistance;
		}

		// Discard candidates that are too similar.
		if (minDist > distance)
			continue;

		// Add the match.
		matches.push_back(std::make_pair<PositionType, DistanceType>(std::move(candidatePos), std::move(distance)));
	}

	// Sort the matches by their distance.
	std::sort(matches.begin(), matches.end(), [](MatchType& lhs, MatchType& rhs) {
		return lhs.second < rhs.second;
	});

	// Return the matches.
	if (matches.size() == 0)
		return false;
	else if (k <= matches.size())
		mtch = std::vector<MatchType>(matches.begin(), matches.begin() + k);
	else
		mtch = matches;

	return true;
}