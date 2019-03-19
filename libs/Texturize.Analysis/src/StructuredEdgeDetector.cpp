#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Random Forest Edge Detector implementation                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

StructuredEdgeDetector::StructuredEdgeDetector(const std::string& modelName) :
	_detector(cv::ximgproc::createStructuredEdgeDetection(modelName))
{
}

void StructuredEdgeDetector::apply(Sample& result, const Sample& sample) const
{
	cv::Mat s = (cv::Mat)sample;
	cv::Mat r(s.size(), CV_32FC1);

	_detector->detectEdges(s, r);
	cv::normalize(r, r, 1.f, 0.f, cv::NORM_MINMAX);

	result = Sample(r);
}