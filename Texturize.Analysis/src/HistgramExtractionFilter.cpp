#include "stdafx.h"

#include <numeric>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

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
	//TEXTURIZE_ASSERT(sample.channels() <= 4);

	const cv::Mat source = (cv::Mat)sample;

	// One sample channel maps to one dimension in the histogram.
	const int dimensions = static_cast<int>(sample.channels());
	cv::Size histogramSize(pow(_binsPerDimension, dimensions), sample.height() * sample.width());
	cv::Mat histogram = cv::Mat::zeros(histogramSize, CV_32FC1);

	// Create multiple bins for each dimension.
	const std::vector<int> bins(dimensions, _binsPerDimension);

	// Define the value range for one dimension. Since all channels should be normalized CV_32F, 0..1 is good.
	float range[2] = { 0.f, 1.f };
	const std::vector<float*> ranges(dimensions, range);

	// Calculate and return the histogram descriptor for each pixel.
	tbb::parallel_for(tbb::blocked_range2d<size_t>(0, sample.height(), 0, sample.width()), [&source, &histogram, &dimensions, &bins, &ranges, this](const tbb::blocked_range2d<size_t>& range) {
		for (size_t x = range.cols().begin(); x < range.cols().end(); ++x) {
			for (size_t y = range.rows().begin(); y < range.rows().end(); ++y) {
				// Generate a mask for a 49x49 pixel kernel at [x, y].
				auto mask = this->mask(source.size(), cv::Point2i(x, y), 49);

				// Calculate the histogram at this point.
				cv::Mat descriptor;
				cv::calcHist(&source, 1, nullptr, mask, descriptor, dimensions, const_cast<const int*>(bins.data()), const_cast<const float**>(ranges.data()), true, false);

				// Store the descriptor.
				auto row = y * source.cols + x;
				histogram.row(row) = descriptor.reshape(1, 1);
			}
		}
	});

	result = Sample(histogram);
}

cv::Mat HistogramExtractionFilter::mask(const cv::Size& sampleSize, const cv::Point2i& at, const int kernel, const bool wrap) const
{
	TEXTURIZE_ASSERT(kernel % 2 == 1);										// The kernel should be an odd number.
	TEXTURIZE_ASSERT(at.x >= 0 && at.x < sampleSize.width);					// X should be valid.
	TEXTURIZE_ASSERT(at.y >= 0 && at.y < sampleSize.height);				// Y should be valid.

	cv::Mat mask = cv::Mat::zeros(sampleSize, CV_8UC1);
	int relativeKernel = kernel / 2;

	// For each pixel inside the kernel...
	// TODO: Check if range fits.
	tbb::parallel_for(tbb::blocked_range2d<int>(-relativeKernel, relativeKernel, -relativeKernel, relativeKernel), 
		[&mask, &wrap, &at](const tbb::blocked_range2d<int>& range) {
		for (int x = range.cols().begin(); x < range.cols().end(); ++x) {
			for (int y = range.rows().begin(); y < range.rows().end(); ++y) {
				// Get the coordiantes of the point in the mask.
				int _x{ at.x + x }, _y{ at.y + y };

				// Check if the position is out of bounds.
				if (_x >= mask.size().width || _y >= mask.size().height || _x < 0 || _y < 0)
				{
					// In case no wrapping should be applied, ignore the pixel. Otherwise, wrap its coordinates.
					if (!wrap) 
						continue;
					else
						Sample::wrapCoords(mask.size().width, mask.size().height, _x, _y);
				}

				// Set the mask.
				mask.at<TX_BYTE>(cv::Point2i(_x, _y)) = TRUE;
			}
		}
	});

	return mask;
}