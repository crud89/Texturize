#include "stdafx.h"

#include <Adapters/tapkee.hpp>

using namespace Texturize;
using namespace Texturize::Tapkee;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Distance metric implementations.                                                        /////
///////////////////////////////////////////////////////////////////////////////////////////////////

float EuclideanDistanceMetric::calculateDistance(const cv::Mat& lhs, const cv::Mat& rhs, const cv::InputArray& cost) const
{
	return static_cast<TAPKEE_CUSTOM_INTERNAL_NUMTYPE>(cv::norm(lhs, rhs, cv::NORM_L2));
}

float EarthMoversDistanceMetric::calculateDistance(const cv::Mat& lhs, const cv::Mat& rhs, const cv::InputArray& cost) const
{
	TEXTURIZE_ASSERT(lhs.channels() == 1 && rhs.channels() == 1);								// Only single-channel descriptors are allowed.
	TEXTURIZE_ASSERT((lhs.cols == 1 || lhs.rows == 1) && (rhs.cols == 1 || rhs.rows == 1));		// The descriptors should be one-dimensional.
	TEXTURIZE_ASSERT(lhs.type() == CV_32F && rhs.type() == CV_32F);								// Descriptors require types to be single-precision floatings point values.

	cv::Mat l = lhs.cols == 1 ? lhs.t() : lhs;
	cv::Mat r = rhs.cols == 1 ? rhs.t() : rhs;

	cv::Mat binIndices(1, lhs.cols, CV_32F);

	for (int bin(0); bin < lhs.cols; ++bin)
		binIndices.at<float>(0, bin) = static_cast<float>(bin);

	l.push_back(binIndices);
	r.push_back(binIndices);

	return static_cast<float>(cv::EMD(l.t(), r.t(), cv::DIST_L2, cost));
}