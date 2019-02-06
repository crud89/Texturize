#include "stdafx.h"

#include <numeric>

#include <analysis.hpp>
#include <opencv2/core/fast_math.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Histogram equalization filter implementation				                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

HistogramMatchingFilter::HistogramMatchingFilter(const Sample& referenceSample) :
	_referenceCdf(sampleCdf(referenceSample))
{
}

cv::Mat& HistogramMatchingFilter::sampleCdf(const Sample& sample) const
{
	TEXTURIZE_ASSERT(sample.channels() == 1);								// Only greyscale histograms are supported.

	// Convert to 8 bit greyscale in order to be able to get a discrete histogram.
	cv::Mat greyscale, histogram;
	((cv::Mat)sample).convertTo(greyscale, CV_8UC1, 255.f);

	// Calculate the reference histogram.
	int bins = 256;										// Use 256 discrete bins.
	float discreteRange[] = { 0.f, static_cast<float>(bins) };
	const float* channelRanges = { discreteRange };		// One per channel.
	cv::calcHist(&greyscale, 1, nullptr, cv::noArray(), histogram, 1, &bins, &channelRanges);

	// Calculate the pdf (normalize histogram to a range of [0, 1].
	cv::Mat pdf = histogram / (sample.width() * sample.height());

	// Calculate the cummulative pdf.
	cv::Mat cdf = static_cast<float>(bins - 1) * cummulate(pdf);

	// Discretize the cdf.
	cdf.forEach<float>([](float& val, const int* index) -> void {
		val = static_cast<float>(cvRound(val));
	});

	return cdf;
}

cv::Mat& HistogramMatchingFilter::cummulate(const cv::Mat& histogram) const
{
	// The histogram should be a 1D array, that stores bins within its rows.
	TEXTURIZE_ASSERT(histogram.cols == 1);
	TEXTURIZE_ASSERT(histogram.rows > 0);

	std::vector<float> cumsum(histogram.rows);
	std::partial_sum(histogram.begin<float>(), histogram.end<float>(), cumsum.begin());
	return cv::Mat(cumsum, false);
}

void HistogramMatchingFilter::apply(Sample& result, const Sample& sample) const
{
	// Get the cdf of the sample.
	cv::Mat sampleCdf = this->sampleCdf(sample);
	std::vector<BYTE> transformLookupTable(sampleCdf.rows);

	// Find the transformation function from the reference cdf to the sample cdf. This is done by iterating
	// the sample cdf and looking for the point within the reference cdf that has the same frequency (i.e. 
	// number of pixels with that value). The offset is then remembered for each frequency within the
	// transformation function.
	_referenceCdf.forEach<float>([&sampleCdf, &transformLookupTable](float& val, const int* index) -> void {
		// The value represents the probability that a pixel has a certain frequency within the reference
		// sample. The index contains the current row, i.e. the associated frequency.
		const int referenceFrequency = index[0];
		
		// Iteratively walk the sample cdf to find the frequency with minimum deviation.
		float minimum = std::numeric_limits<float>::max();
		int sampleFrequency(0);
		
		for (auto it = sampleCdf.begin<float>(); it < sampleCdf.end<float>(); ++it, ++sampleFrequency)
		{
			// Get the difference.
			float difference = abs(*it - val);

			// Now there are two cases: If the difference is lower than the current minimum, continue
			// minimization. Otherwise, the minimum has been surpassed and found.
			if (difference <= minimum)
				minimum = difference;
			else
			{
				transformLookupTable[sampleFrequency] = static_cast<BYTE>(referenceFrequency);
				break;
			}
		}
	});

	// The transformLookupTable now contains a mapping, that assigns each sample frequency a frequency of 
	// the reference sample. All thats left to do is to apply this mapping to the sample.
	cv::Mat matchedSample;
	cv::LUT((cv::Mat)sample, cv::Mat(transformLookupTable, false), matchedSample);

	result = Sample(matchedSample);
}