#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Histogram equalization filter implementation				                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

HistogramMatchingFilter::HistogramMatchingFilter(const Sample& target) :
    _target((cv::Mat)target)
{
    // TODO: Calculate historgram instead.
    //TEXTURIZE_ASSERT(target.channels() == 1);                       // Only greyscale histograms are supported.
    //_target.convertTo(_target, CV_8UC1, 255.f);
}

void HistogramMatchingFilter::apply(Sample& result, const Sample& sample) const
{
    throw;
}