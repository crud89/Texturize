#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// OTSU Binarization implementation			                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DynamicThresholdFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat s = (cv::Mat)sample;
	cv::Mat r(s.size(), CV_32FC1); 
	
	s.convertTo(s, CV_8UC1, 255.f);
	cv::threshold(s, s, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	s.convertTo(r, CV_32FC1, 1 / 255.f);

	result = Sample(r);
}