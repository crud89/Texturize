#pragma once

#include "analysis.hpp"

using namespace Texturize;

#ifndef __cplusplus
#error The sample.hpp header can only be compiled using C++.
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Sample implementation                                                                   /////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <size_t cn>
Sample_<cn>::Sample_() :
	Sample(cn)
{
}

template <size_t cn>
Sample_<cn>::Sample_(const cv::Mat& raw) :
	Sample(raw)
{
	TEXTURIZE_ASSERT(raw.channels() == cn);								// Channels count of the input exemplar must equal the template parameter.
}

template <size_t cn>
Sample_<cn>::Sample_(const cv::Size& size) :
	Sample(cn, size)
{
}

template <size_t cn>
Sample_<cn>::Sample_(const int width, const int height) :
	Sample(cn, width, height)
{
}

template <size_t cn>
template <size_t _cn>
void Sample_<cn>::extract(const int* fromTo, const size_t pairs, Sample_<_cn>& sample) const
{
	TEXTURIZE_ASSERT(fromTo != nullptr);								// The mapping array must be initialized.
	TEXTURIZE_ASSERT(pairs > 0);										// The number of pairs in the mapping array must be positive.

	std::vector<int> map(fromTo, pairs * 2);
	this->extract(map, sample);
}

template <size_t cn>
template <size_t _cn>
void Sample_<cn>::extract(const std::vector<int>& fromTo, Sample_<_cn>& sample) const
{
	TEXTURIZE_ASSERT(fromTo.size() > 0 && fromTo.size() % 2 == 0);		// The vector contains pairs and there must be at least one pair and no value without counter-part.
	TEXTURIZE_ASSERT(sample.size() == this->size());					// The target sample should have the same size as the current sample.

	for (int i(0); i < fromTo.size(); i += 2)
	{
		int from = fromTo[i], to = fromTo[i + 1];
		TEXTURIZE_ASSERT(from >= 0 && from < cn);						// The source index must address a valid channel within the current sample.
		TEXTURIZE_ASSERT(to >= 0 && to < _cn);							// The target index must address a valid channel within the target sample.

		sample.setChannel(to, _channels[from]);
	}
}

template <size_t cn>
template <size_t _cn>
void Sample_<cn>::map(const int* fromTo, const size_t pairs, const Sample_<_cn>& sample)
{
	TEXTURIZE_ASSERT(fromTo != nullptr);								// The mapping array must be initialized.
	TEXTURIZE_ASSERT(pairs > 0);										// The number of pairs in the mapping array must be positive.

	std::vector<int> map(fromTo, pairs * 2);
	this->map(map, sample);
}

template <size_t cn>
template <size_t _cn>
void Sample_<cn>::map(const std::vector<int>& fromTo, const Sample_<_cn>& sample)
{
	TEXTURIZE_ASSERT(fromTo.size() > 0 && fromTo.size() % 2 == 0);		// The vector contains pairs and there must be at least one pair and no value without counter-part.

	for (int i(0); i < fromTo.size(); i += 2)
	{
		int from = fromTo[i], to = fromTo[i + 1];
		TEXTURIZE_ASSERT(from >= 0 && from < _cn);						// The source index must address a valid channel within the target sample.
		TEXTURIZE_ASSERT(to >= 0 && to < cn);							// The target index must address a valid channel within the current sample.

		sample.getChannel(from, _channels[to]);
	}
}

template <size_t cn>
template <size_t _sk>
void Sample_<cn>::getNeighborhood(const int x, const int y, cv::Vec<float, _sk * cn>& v, const bool weight) const
{
	TEXTURIZE_ASSERT(sk % 2 == 1);										// Only allow odd kernel sizes (even steps into each direction, plus the current pixel row/col)
	int extent = (sk - 1) / 2, w(0), wx, wy;
	cv::Vec<float, sk * cn> neighborhood;
	cv::Mat kernel = weight ? cv::getGaussianKernel(sk, 0, CV_32F) : cv::Mat::ones(_sk, _sk, CV_32F);

	for (int _x(-extent); _x <= extent; ++_x)
	{
		for (int _y(-extent); _y <= extent; ++_y)
		{
			// Get the actual pixel coordinates.
			wx = x + _x; wy = y + _y;
			this->wrapCoords(wx, wy);

			// Calculate the weight of the pixel values.
			float g = weight ? kernel.at<float>(_x + extent) * kernel.at<float>(_y + extent) : 1.f;

			// Extract the value from each channel individually.
			for (int c(0); c < cn; ++c)
				neighborhood[w++] = g * _channels[c].at<float>(wy, wx);
		}
	}

	return neighborhood;
}

template <size_t cn>
template <size_t _sk>
void Sample_<cn>::getNeighborhood(const cv::Point& p, cv::Vec<float, _sk * cn>& v, const bool weight) const
{
	return this->getNeighborhood(p.x, p.y, v, weight);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Filter implementation                                                                   /////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TResult = Sample, typename TSample = Sample>
FunctionFilter<TResult, TSample>::FunctionFilter(Functor fn) :
	_functor(fn)
{
}

template <typename TResult = Sample, typename TSample = Sample>
void FunctionFilter<TResult, TSample>::apply(Sample& result, const Sample& sample) const
{
	result = this->_functor(sample);
}