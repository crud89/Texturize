#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Feature extraction implementation				                                          /////
///////////////////////////////////////////////////////////////////////////////////////////////////

FeatureExtractor::FeatureExtractor()
{
	_cascade.append(std::make_shared<DynamicThresholdFilter>());
	_cascade.append(std::make_shared<FeatureDistanceFilter>());
}

void FeatureExtractor::apply(Sample& result, const Sample& sample) const
{
	_cascade.apply(result, sample);
}