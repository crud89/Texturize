#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Normalization filter implementation				                                      /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void NormalizationFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat r;
	cv::normalize((cv::Mat)sample, r, 1.f, 0.f, cv::NORM_MINMAX);
	result = Sample(r);
}