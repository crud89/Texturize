#include "stdafx.h"

#include <numeric>

#include <sampling.hpp>
#include <opencv2/core/fast_math.hpp>

#include "PerlinNoise.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// 2D Perlin noise generator                            		                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void PerlinNoise2D::makeNoise(Sample& sample) const
{
    cv::Mat noise = CreatePerlinNoiseImage(sample.size(), 1.0), result;
    cv::add((cv::Mat)sample, noise, result);
    
    sample = Sample(result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// 2D Perlin noise generator from a reference variance  		                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

MatchingVarianceNoise::MatchingVarianceNoise(const float referenceVariance) :
    _referenceVariance(referenceVariance)
{
}

void MatchingVarianceNoise::makeNoise(Sample& sample) const
{
	TEXTURIZE_ASSERT(sample.channels() == 1);

    cv::Scalar mean, deviation;
    cv::meanStdDev((cv::Mat)sample, mean, deviation);

    float amplitude = _referenceVariance / static_cast<float>(pow(mean.val[0], 2));
    cv::Mat noise = CreatePerlinNoiseImage(sample.size(), amplitude), result;
    cv::add((cv::Mat)sample, noise, result);

    sample = Sample(result);
}

std::unique_ptr<MatchingVarianceNoise> MatchingVarianceNoise::FromSample(const Sample& sample)
{
	TEXTURIZE_ASSERT(sample.channels() == 1);

    cv::Scalar mean, deviation;
    cv::meanStdDev((cv::Mat)sample, mean, deviation);

    return std::make_unique<MatchingVarianceNoise>(static_cast<float>(pow(mean.val[0], 2)));
}