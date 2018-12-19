#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Gaussian Blur Filter implementation		                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

GaussianBlurFilter::GaussianBlurFilter(const float& deviation) :
	GaussianBlurFilter(deviation, 5)
{
}

GaussianBlurFilter::GaussianBlurFilter(const float& deviation, const int& kernel) :
	_deviation(deviation), _kernel(kernel)
{
	TEXTURIZE_ASSERT(kernel > 0);			// The kernel should be positive.
	TEXTURIZE_ASSERT(kernel % 2);			// Evaluates to 1 (TRUE) if kernel is odd, which it is required to be.
}

void GaussianBlurFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat s = (cv::Mat)sample;
	cv::Mat r(s.size(), s.type());

	cv::GaussianBlur(s, r, cv::Size(5, 5), _deviation, _deviation, cv::BORDER_WRAP);

	result = Sample(r);
}