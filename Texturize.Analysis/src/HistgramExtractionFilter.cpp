#include "stdafx.h"

#include <numeric>

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Histogram extraction filter implementation				                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

HistogramExtractionFilter::HistogramExtractionFilter(const int& binsPerDim) :
	_binsPerDimension(binsPerDim)
{
}

void HistogramExtractionFilter::apply(Sample& result, const Sample& sample) const
{
	// One sample channel maps to one dimension in the histogram.
	int dimensions = static_cast<int>(sample.channels());
	cv::Mat histogram, source = (cv::Mat)sample;

	// Create multiple bins for each dimension.
	std::vector<int> bins(dimensions, _binsPerDimension);

	// Define the value range for one dimension. Since all channels should be normalized CV_32F, 0..1 is good.
	float range[2] = { 0.f, 1.f };
	std::vector<float*> ranges(dimensions, range);

	// Calculate and return the histogram.
	// TODO: Do this for each 48x48 pixel neighborhood, store the result
	cv::calcHist(&source, 1, nullptr, cv::noArray(), histogram, dimensions, const_cast<const int*>(bins.data()), const_cast<const float**>(ranges.data()), true, false);

	result = Sample(histogram);
}