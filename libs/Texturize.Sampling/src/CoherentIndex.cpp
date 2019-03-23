#include "stdafx.h"

#include <sampling.hpp>

#include <algorithm>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

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
	int sampleLevel = log2(sample->width());

	//TEXTURIZE_ASSERT(sample->width() == sample->height());
	TEXTURIZE_ASSERT_DBG(kernel % 2 == 1);						// The kernel must be an odd number of pixels.

	// Generate an RNG, that randomly selects pixels.
	std::uniform_int_distribution<int> distribution(0, (sample->width() * sample->height()) - 1);

	// Get the source descriptors.
	_exemplarDescriptors = _descriptorExtractor->calculateNeighborhoodDescriptors(*sample);

	// Initialize the candidate set.
	_candidates = cv::Mat(sample->width() * sample->height(), k, CV_32SC1);

	// Calculate the k-coherent candidates for each pixel.
	tbb::parallel_for(tbb::blocked_range2d<size_t>(0, sample->width(), 0, sample->height()), [this, &sample, &distribution, k](const tbb::blocked_range2d<size_t>& range) {
		for (size_t x = range.cols().begin(); x < range.cols().end(); ++x)
		for (size_t y = range.rows().begin(); y < range.rows().end(); ++y) {
			// Get the neighborhood descriptor for the pixel at the current position.
			Sample::Texel neighborhood = _exemplarDescriptors.row(y * sample->width() + x);

			// TODO: Original implementation applies 3 box filters and keeps candidates from 64, 16 and 4 random samples from fine to coarse.
			std::vector<int> candidates;

			for (int i(0); i < k; ++i) {
				int candidateIndex{ -1 };
				DistanceType candidateDistance{ std::numeric_limits<DistanceType>::max() };

				for (int j(0); j < 64; ++j) {
					// Randomly select a valid index.
					int index = distribution(_rng);

					// Get the x/y coordinates of the sample.
					cv::Point2i candidatePos(index % sample->width(), index / sample->width());

					// TODO: Discard positions that are edge pixels and pixels that are too close to the source neighborhood.
					//const int boundarySize = 5;		// 5px from edges.
					//const int horizontalDistance = sample->width() / 20;
					//const int verticalDistance = sample->height() / 20;
					//if ((x < boundarySize || sample->width() - candidatePos.x < boundarySize) ||
					//	(y < boundarySize || sample->height() - candidatePos.y < boundarySize) ||
					//	(std::abs(static_cast<int>(x) - candidatePos.x) < horizontalDistance) ||
					//	(std::abs(static_cast<int>(y) - candidatePos.y) < horizontalDistance)) {
					//	j--;
					//	continue;
					//}

					// Get the candidate neighborhood and compute the distance.
					Sample::Texel candidateNeighborhood = _exemplarDescriptors.row(index);
					DistanceType distance = cv::norm(neighborhood, candidateNeighborhood, _normType);

					//// If required, factor in guidance channels.
					//if (_guidanceMap.has_value()) {
					//	Sample::Texel sourceGuidance, targetGuidance;
					//	_guidanceMap.value().at(cv::Point2i(x, y), sourceGuidance);
					//	_guidanceMap.value().at(candidatePos, targetGuidance);

					//	for (size_t i(0); i < sourceGuidance.size(); ++i)
					//		distance += abs(sourceGuidance[i] - targetGuidance[i]);
					//}

					// If the current candidate is better, keep it.
					if (candidateDistance > distance) {
						candidateDistance = distance;
						candidateIndex = index;
					}
				}

				// Keep most similar ones.
				candidates.push_back(candidateIndex);
			}

			// Store the candidates.
			int neighborhoodIndex = y * sample->width() + x;

			for (int i(0); i < k; ++i)
				_candidates.at<int>(neighborhoodIndex, i) = candidates[i];
		}
	});
}

cv::Mat CoherentIndex::getDescriptor(int index) const
{
	TEXTURIZE_ASSERT(index >= 0 && index < _exemplarDescriptors.rows);

	return _exemplarDescriptors.row(index);
}

void CoherentIndex::getCoherentCandidates(const cv::Point2i& exemplarCoords, const cv::Vec2i& delta, std::vector<int>& candidates) const
{
	// Get the search space transformed exemplar.
	std::shared_ptr<const Sample> exemplar;
	_searchSpace->sample(exemplar);
	int width = exemplar->width();
	int height = exemplar->height();

	// Make sure the coords are valid.
	cv::Point2i coherentPos(exemplarCoords.x + delta[0], exemplarCoords.y + delta[1]);
	Sample::wrapCoords(width, height, coherentPos);

	// Get the k-coherent candidates.
	int coherentIndex = coherentPos.y * width + coherentPos.x;
	candidates = _candidates.row(coherentIndex);
	
	// Also append the coherent candidate.
	candidates.push_back(coherentIndex);
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
	int level = log2(uv.cols);

	// For each neighboring pixel, request the coherent candidate coordinates.
	std::vector<int> candidates;

	for (int x(-1); x <= 1; ++x)
	for (int y(-1); y <= 1; ++y) {
		// Do not include self.
		if (x == 0 && y == 0)
			continue;

		// Get the uv coords at the position.
		cv::Point2i coords(at.x + x, at.y + y);
		Sample::wrapCoords(uv.cols, uv.rows, coords);
		PositionType uvCoords = uv.at<PositionType>(coords);

		// Make them absolute.
		coords = cv::Point2i(uvCoords[0] * width, uvCoords[1] * height);

		// Get a candidate for the neighbor and remember it.
		std::vector<int> c;
		this->getCoherentCandidates(coords, cv::Vec2i(-x, -y), c);
		candidates.insert(candidates.end(), c.begin(), c.end());
	}

	// Get the descriptor for the current sample at the requested position.
	Sample::Texel targetGuidance, targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	// If there is a guidance map, ensure that all guidance channels are provided in the descriptor.
	if (_guidanceMap.has_value()) {
		TEXTURIZE_ASSERT(targetDescriptor.size() == exemplar->channels() + _guidanceMap.value().channels());

		// Furthermore, divide the target descriptor in two parts: The actual descriptor values and the appended guidance values.
		// NOTE: Order matters here!
		targetGuidance = std::vector<float>(targetDescriptor.begin() + exemplar->channels(), targetDescriptor.end());
		targetDescriptor = std::vector<float>(targetDescriptor.begin(), targetDescriptor.begin() + exemplar->channels());
	}

	// Compare each candidate descriptor with target descriptor.
	std::vector<MatchType> matches;

	for each (const auto& candidate in candidates) {
		// Compute the position of the candidate.
		PositionType candidatePos(static_cast<CoordinateType>(candidate % exemplar->width()) / width, static_cast<CoordinateType>(candidate / exemplar->width()) / height);

		// Get the pixel descriptor of the pixel in the exemplar.
		Sample::Texel sourceGuidance, sourceDescriptor = _exemplarDescriptors.row(candidate);

		// Compute the distance between the descriptors.
		DistanceType distance = static_cast<DistanceType>(cv::norm(sourceDescriptor, targetDescriptor, _normType));

		// If a guidance map is provided, get the guidance channel values of the candidate and factor the distance 
		// between the guidance channels into the actual distance.
		if (_guidanceMap.has_value()) {
			// Get the source guidance channels.
			_guidanceMap.value().at(candidatePos, sourceGuidance);

			TEXTURIZE_ASSERT(sourceGuidance.size() == targetGuidance.size());

			// Get the distance between each channel.
			for (size_t i(0); i < sourceGuidance.size(); ++i)
				distance += abs(sourceGuidance[i] - targetGuidance[i]);
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