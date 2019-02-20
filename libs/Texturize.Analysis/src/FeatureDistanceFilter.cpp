#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Euclidean distance transform implementation	                                          /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void FeatureDistanceFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat s = (cv::Mat)sample;
	cv::Mat r(s.size(), CV_32FC1);
	cv::Mat n(s.size(), CV_8UC1);

	s.convertTo(s, CV_8UC1);
	cv::threshold(s, n, 0, 255, cv::THRESH_BINARY_INV);

	cv::Mat dist, distNeg;
	cv::distanceTransform(s, dist, cv::DIST_L2, 5);
	cv::distanceTransform(n, distNeg, cv::DIST_L2, 5);
	
	cv::Mat rn = distNeg - dist;
	cv::normalize(rn, s, 255, 0, cv::NORM_MINMAX);
	
	s.convertTo(r, CV_32FC1, 1.f / 255.f);
	result = Sample(r);
}