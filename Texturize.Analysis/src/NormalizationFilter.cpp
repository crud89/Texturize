#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Normalization filter implementation				                                      /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void GrayscaleFilter::apply(Sample& result, const Sample& sample) const
{
	TEXTURIZE_ASSERT(sample.channels() == 3);

	cv::Mat r;
	cv::cvtColor((cv::Mat)sample, r, cv::COLOR_BGR2GRAY);

	result = Sample(r);
}