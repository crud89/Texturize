#include "stdafx.h"

#include <numeric>

#include <analysis.hpp>
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

void PerlinNoise2D::apply(Sample& result, const Sample& sample) const
{
	// Store sample in result and add noise to it.
	this->makeNoise(result = sample);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// 2D Perlin noise generator from a reference variance  		                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

MatchingVarianceNoise::MatchingVarianceNoise(const float referenceVariance) :
	_referenceVariance({ referenceVariance })
{
}

MatchingVarianceNoise::MatchingVarianceNoise(const std::vector<float>& referenceVariances) :
	_referenceVariance(referenceVariances)
{
}

void MatchingVarianceNoise::makeNoise(Sample& sample) const
{
	// If the filter has been provided with multiple variance values, there must be enough for each sample channel. In case
	// the filter only got provided with one variance value, the same amplitude is applied to all channels.
	TEXTURIZE_ASSERT(_referenceVariance.size() == 1 || sample.channels() <= _referenceVariance.size());

    cv::Scalar mean, deviation;
    cv::meanStdDev((cv::Mat)sample, mean, deviation);
	Sample result(sample.channels());

	for (int channel(0); channel < sample.channels(); ++channel)
	{
		// Create 2D perlin noise and convert it to a floating-point image.
		cv::Mat noise, buffer;
		::CreatePerlinNoiseImage(sample.size()).convertTo(noise, CV_32FC1, 1.f / 255.f);
			
		// Calculate the amplitude as the ratio between reference and sample variances.
		// In case only one reference variance value is provided, it is used over all channels, otherwise
		// it is selected for the current channel.
		float amplitude = _referenceVariance.size() == 1 ?
			_referenceVariance[0] / std::pow(static_cast<float>(deviation.val[channel]), 2.f) :
			_referenceVariance[channel] / std::pow(static_cast<float>(deviation.val[channel]), 2.f);

		noise *= amplitude;

		// Mix the results and store it within the respective sample channel.
		cv::add(sample.getChannel(channel), noise, buffer, cv::noArray(), CV_32F);

		// TODO: Normalizing here should affect the variance ratio and should typically not be required. On the other
		//       hand, not normalizing results in invalid refined channels.
		cv::normalize(buffer, buffer);
		result.setChannel(channel, buffer);
	}

    sample = Sample(result);
}

std::unique_ptr<IFilter> MatchingVarianceNoise::FromSample(const Sample& sample)
{
	std::vector<float> variances;

	cv::Scalar mean, deviation;
	cv::meanStdDev((cv::Mat)sample, mean, deviation);

	for (int channel(0); channel < sample.channels(); ++channel)
		variances.push_back(static_cast<float>(pow(deviation.val[channel], 2)));

    return std::make_unique<MatchingVarianceNoise>(variances);
}