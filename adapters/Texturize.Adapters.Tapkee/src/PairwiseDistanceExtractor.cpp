#include "stdafx.h"

#include <Adapters/tapkee.hpp>

#include <tbb/blocked_range.h>
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
		tbb::parallel_for(tbb::blocked_range<int>(0, numFeatures), [&numFeatures, &distances, &cost, &sample, this] (const tbb::blocked_range<int>& range) {
			for (int x = range.begin(); x != range.end(); ++x) {
				const int _x{ x };

				tbb::parallel_for(tbb::blocked_range<int>(_x + 1, numFeatures), [&distances, &_x, &cost, &sample, this] (const tbb::blocked_range<int>& range) {
					for (int y = range.begin(); y != range.end(); ++y) {
						float distance = _distanceMetric->calculateDistance(sample.row(_x), sample.row(y), cost);
						distances.at<float>(_x, y) += distance;
						distances.at<float>(y, _x) += distance;
					}
				});
			}
		});
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