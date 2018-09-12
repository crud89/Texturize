#include "stdafx.h"

#include <sampling.hpp>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex base interface                                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SearchIndex::SearchIndex(const ISearchSpace* searchSpace) : 
	_searchSpace(searchSpace)
{
	TEXTURIZE_ASSERT(searchSpace != nullptr);
}

const ISearchSpace* SearchIndex::getSearchSpace() const
{
	return _searchSpace;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// FeatureMatching-based search index implementation                                       /////
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureMatchingIndex::FeatureMatchingIndex(const ISearchSpace* searchSpace, const cv::NormTypes norm, const bool crossCheck) :
	SearchIndex(searchSpace), _matcher(cv::BFMatcher::create(norm, crossCheck))
{
	this->init();
}

FeatureMatchingIndex::FeatureMatchingIndex(const ISearchSpace* searchSpace, cv::flann::IndexParams* indexParams, cv::flann::SearchParams* searchParams) :
	SearchIndex(searchSpace), _matcher(cv::makePtr<cv::FlannBasedMatcher>(indexParams, searchParams))
{
	this->init();
}

void FeatureMatchingIndex::init()
{
	// Precompute the neighborhood descriptors used to train data.
	const Sample* sample;
	this->getSearchSpace()->sample(&sample);

	// Form a descriptor vector from the sample.
	cv::Mat descriptors = DescriptorExtractor::calculateNeighborhoodDescriptors(*sample);

	// Train the matcher with the sample set.
	_matcher->add(descriptors);
	_matcher->train();
}

cv::Mat FeatureMatchingIndex::calculateNeighborhoodDescriptors() const
{
	const Sample* exemplar;
	this->getSearchSpace()->sample(&exemplar);

	return DescriptorExtractor::calculateNeighborhoodDescriptors(*exemplar);
}

cv::Mat FeatureMatchingIndex::calculateNeighborhoodDescriptors(const cv::Mat& uv) const
{
	const Sample* exemplar;
	this->getSearchSpace()->sample(&exemplar);

	return DescriptorExtractor::calculateNeighborhoodDescriptors(*exemplar, uv);
}

bool FeatureMatchingIndex::findNearestNeighbor(const std::vector<float>& descriptor, cv::Vec2f& match, float minDist, float* dist) const
{
	// Find the best match only.
	std::vector<cv::Vec2f> matches;
	std::vector<float> distances;

	if (!this->findNearestNeighbors(descriptor, matches, 1, minDist, &distances))
		return false;

	if (matches.size() == 0)
		return false;
	
	// Return the match and distance.
	match = matches[0];

	if (dist != nullptr)
		*dist = distances[0];

	return true;
}

bool FeatureMatchingIndex::findNearestNeighbors(const std::vector<float>& descriptor, std::vector<cv::Vec2f>& mtx, const int k, float minDist, std::vector<float>* dist) const
{
	// Perform a knn-search.
	std::vector<std::vector<cv::DMatch>> matches;
	_matcher->knnMatch(descriptor, matches, k);

	TEXTURIZE_ASSERT_DBG(matches.size() == 1);								// There should only be one result set, since the query was for one texel.

	// Store the sample to find the keypoints.
	int width, height;
	this->getSearchSpace()->sampleSize(width, height);

	if (matches[0].size() == 0)
		return false;

	// Copy the coords of each match.
	for each (auto match in matches[0])
	{
		int row = match.trainIdx / width;
		int col = match.trainIdx % width;
		float d = match.distance;

		if (d < minDist)
			continue;

		if (dist != nullptr)
			dist->push_back(d);

		mtx.push_back(cv::Vec2f(col / static_cast<float>(width), row / static_cast<float>(height)));
	}

	return true;
}

bool FeatureMatchingIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist, float* dist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbor(targetDescriptor, match, minDist, dist);
}

bool FeatureMatchingIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k, float minDist, std::vector<float>* dist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbors(targetDescriptor, matches, k, minDist, dist);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex implementation based on coherent pixels.                                    /////
///////////////////////////////////////////////////////////////////////////////////////////////////

CoherentIndex::CoherentIndex(const ISearchSpace* searchSpace, cv::NormTypes distanceMeasure) :
	SearchIndex(searchSpace), _normType(distanceMeasure)
{
	this->init();
}

void CoherentIndex::init()
{
	// Precompute the neighborhood descriptors used to train data.
	const Sample* sample;
	this->getSearchSpace()->sample(&sample);

	// Form a descriptor vector from the sample.
	_exemplarDescriptors = DescriptorExtractor::indexNeighborhoods(*sample);
}

CoherentIndex::TDistance CoherentIndex::measureVisualDistance(const std::vector<float>& targetDescriptor, const cv::Point2i& towards) const
{
	// Get the search space transformed exemplar.
	const Sample* exemplar;
	this->getSearchSpace()->sample(&exemplar);

	std::vector<float> exemplarDescriptor = _exemplarDescriptors.row(towards.y * exemplar->width() + towards.x);

	// Measure the visual distance based on the provided norm type (default is sum of squared distances).
	TEXTURIZE_ASSERT(exemplarDescriptor.size() == targetDescriptor.size());

	return static_cast<float>(cv::norm(targetDescriptor, exemplarDescriptor, _normType));
}

void CoherentIndex::getCoherentCandidate(const std::vector<float>& targetDescriptor, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta, TMatch& match) const
{
	// Get the search space transformed exemplar.
	const Sample* exemplar;
	this->getSearchSpace()->sample(&exemplar);
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

RandomWalkIndex::RandomWalkIndex(const ISearchSpace* searchSpace, cv::NormTypes distanceMeasure) :
	CoherentIndex(searchSpace, distanceMeasure)
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
		const Sample* exemplar;
		this->getSearchSpace()->sample(&exemplar);

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