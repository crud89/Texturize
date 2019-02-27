#include "stdafx.h"

#include <Adapters/tapkee.hpp>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

#include <opencv2/core/eigen.hpp>

using namespace Texturize;
using namespace Texturize::Tapkee;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Pairwise distance extractor.                                                            /////
///////////////////////////////////////////////////////////////////////////////////////////////////

PairwiseDistanceExtractor::PairwiseDistanceExtractor(std::unique_ptr<IDistanceMetric> distanceMetric) :
	_distanceMetric(std::move(distanceMetric))
{
}

cv::Mat PairwiseDistanceExtractor::computeDistances(const cv::Mat& sample) const
{
	std::vector<cv::Mat> samples{ sample };
	return this->computeDistances(samples);
}

cv::Mat PairwiseDistanceExtractor::computeDistances(const std::vector<cv::Mat>& samples) const
{
	TEXTURIZE_ASSERT(!samples.empty());										// The sample set must not be empty.
	size_t numFeatures = samples.front().rows;

	for each (const auto& sample in samples) {
		TEXTURIZE_ASSERT(sample.rows == numFeatures);						// Each sample requires to contain the same number of feature descriptors.
	}

	// Create the distance matrix.
	cv::Mat distances = cv::Mat::zeros(numFeatures, numFeatures, CV_32FC1);

	for each (const auto& sample in samples) {
		// Calculate cost matrix for the current descriptor set.
		cv::Mat cost = cv::Mat::zeros(sample.cols, sample.cols, CV_32FC1);

		for (int x(0); x < static_cast<int>(sample.channels()); ++x)
			for (int y(0); y < static_cast<int>(sample.channels()); ++y)
				cost.at<float>(x, y) = static_cast<float>(abs(x - y));

		// Compute the distance matrix as a symmetric matrix of pairwise distances.
		// NOTE: The diagonal represents the distances between a feature with itself, thus it always reduces to 0.
		// TODO: This could run in parallel.
		for (int x(1); x < static_cast<int>(numFeatures) - 1; ++x) {
			for (int y(x + 1); y < static_cast<int>(numFeatures); ++y) {
				TEXTURIZE_ASSERT_DBG(x != y);

				float distance = _distanceMetric->calculateDistance(sample.row(x), sample.row(y), cost);
				distances.at<float>(x, y) += distance;
				distances.at<float>(y, x) += distance;
			}
		}
	}

	// Return the distances.
	return distances;
}

tapkee::DenseSymmetricMatrix PairwiseDistanceExtractor::computeDistances(const cv::Mat& sample, std::vector<tapkee::IndexType>& indices) const
{
	std::vector<cv::Mat> samples{ sample };
	return this->computeDistances(samples, indices);
}

tapkee::DenseSymmetricMatrix PairwiseDistanceExtractor::computeDistances(const std::vector<cv::Mat>& samples, std::vector<tapkee::IndexType>& indices) const
{
	// Compute the distances.
	cv::Mat distances = this->computeDistances(samples);

	TEXTURIZE_ASSERT(distances.rows == distances.cols);						// The distance matrix is a symmetrical, diagonal matrix.

	// Convert to tapkee::DenseSymmetricMatrix.
	tapkee::DenseSymmetricMatrix eigenDistances(distances.rows, distances.cols);
	cv::cv2eigen(distances, eigenDistances);

	// Indices are linear.
	indices.resize(distances.rows);

	for (tapkee::IndexType idx(0); idx < indices.size(); ++idx)
		indices[idx] = idx;

	return eigenDistances;
}