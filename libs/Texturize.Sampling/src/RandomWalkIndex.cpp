#include "stdafx.h"

#include <sampling.hpp>

#include <algorithm>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex implementation based on random pixel walk.                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

RandomWalkIndex::RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace, const int k) :
	CoherentIndex(searchSpace, k)
{
}

RandomWalkIndex::RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap, const int k) :
	CoherentIndex(searchSpace, guidanceMap, k)
{
}

RandomWalkIndex::PositionType RandomWalkIndex::getRandomPixelAround(const PositionType& point, int radius, int dominantDimensionExtent) const
{
	return this->getRandomPixelAround(point, static_cast<CoordinateType>(radius) / static_cast<CoordinateType>(dominantDimensionExtent));
}

RandomWalkIndex::PositionType RandomWalkIndex::getRandomPixelAround(const PositionType& point, CoordinateType radius) const
{
	std::uniform_real_distribution<CoordinateType> distribution(-radius, radius);
	return PositionType(point[0] + distribution(_rng), point[1] + distribution(_rng));
}

bool RandomWalkIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, MatchType& match, DistanceType minDist) const
{
	std::vector<MatchType> matches;

	if (!this->findNearestNeighbors(descriptors, uv, at, matches, 1, minDist))
		return false;

	match = matches.front();
	return true;
}

bool RandomWalkIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<MatchType>& matches, const int k, DistanceType minDist) const
{
	// Get the target descriptor in order to calculate the distance later on.
	// NOTE: The descriptors are indexed by their UV-coordinates (i.e. one descriptor for each point in UV-space).
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.rows + at.x);

	// Perform a coherent search for the best candidate.
	std::vector<MatchType> candidates;

	if (!CoherentIndex::findNearestNeighbors(descriptors, uv, at, candidates, k, minDist))
		return false;

	// If the candidate is somewhere near the edge of the sample, do not search further. This is since candidates are not well defined at edges.
	uint32_t level = log2(static_cast<uint32_t>(uv.cols));
	uint32_t threshold = pow(2, level);

	// Refine search for pixels inside the threshold.
	std::vector<MatchType> refinedCandidates;

	if ((at.x > threshold && at.x < uv.cols - threshold) && (at.y > threshold && at.y < uv.rows - threshold)) {
		for each (const auto& candidate in candidates) {
			// Randomly walk around the environment of the match, trying to find a better one.
			// The radius around the pixel is calculated from the smaller dimension. Initially it is half as large as the exemplar width.
			std::shared_ptr<const Sample> exemplar;
			_searchSpace->sample(exemplar);
			
			// Create a candidate instance for the refined candidate.
			MatchType refinedCandidate = candidate;

			// Perform as long, as the environment is non-trivial, i.e. there is an environment which does not only contain the candidate pixel.
			// The radius get's halved with each iteration.
			for (int radius(exemplar->width() >> 1); radius >= 2; radius >>= 1)
			{
				// Get a random point around the current candidate.
				PositionType candidatePos = this->getRandomPixelAround(candidate.first, radius, exemplar->width());
				Sample::wrapCoords(candidatePos);

				// Compute the distance between the corrected pixel and the current best match.
				cv::Point2i pixelCoords(static_cast<int>(candidatePos[0] * static_cast<CoordinateType>(exemplar->width())), static_cast<int>(candidatePos[1] * static_cast<CoordinateType>(exemplar->height())));
				int descriptorIndex = pixelCoords.x * exemplar->width() + pixelCoords.y;
				DistanceType distance = static_cast<DistanceType>(cv::norm(this->getDescriptor(level, descriptorIndex), targetDescriptor, _normType));

				// If the distance is lower than the one found within the coherent search, replace the candidate and continue.
				if (distance < candidate.second)
					refinedCandidate = std::make_pair<PositionType, DistanceType>(std::move(candidatePos), std::move(distance));
			}
		}
	}

	// Sort the matches by their distance.
	std::sort(refinedCandidates.begin(), refinedCandidates.end(), [](MatchType& lhs, MatchType& rhs) {
		return lhs.second < rhs.second;
	});

	// Return the matches.
	if (refinedCandidates.size() == 0)
		return false;
	else if (k <= refinedCandidates.size())
		matches = std::vector<MatchType>(refinedCandidates.begin(), refinedCandidates.begin() + k);
	else
		matches = refinedCandidates;

	return true;
}