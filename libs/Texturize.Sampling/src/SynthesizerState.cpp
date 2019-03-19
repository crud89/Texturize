#include "stdafx.h"

#include <sampling.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Synthesis state implementation                                                          /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SynthesizerState::SynthesizerState(const SynthesisSettings& config) :
	_config(config)
{
	TEXTURIZE_ASSERT(config.validate());						// The configuration must be valid.
}

SynthesisSettings SynthesizerState::config() const
{
	return _config;
}

void SynthesizerState::config(SynthesisSettings& config) const
{
	config = _config;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Pyramid synthesizer state implementation                                                /////
///////////////////////////////////////////////////////////////////////////////////////////////////

PyramidSynthesizerState::PyramidSynthesizerState(const PyramidSynthesisSettings& config) :
	SynthesizerState(config), _hash(std::make_unique<CoordinateHash>(config._rngState)), _configEx(config)
{
}

PyramidSynthesisSettings PyramidSynthesizerState::config() const
{
	return _configEx;
}

void PyramidSynthesizerState::config(PyramidSynthesisSettings& config) const
{
	config = _configEx;
}

const CoordinateHash* PyramidSynthesizerState::getHash() const
{
	return _hash.get();
}

float PyramidSynthesizerState::getRandomness() const
{
	return _randomness;
}

float PyramidSynthesizerState::getSpacing() const
{
	float power = 1.f / std::pow<float>(2.f, static_cast<float>(this->level() + 1.f));
	return power * _configEx._scale;
}

void PyramidSynthesizerState::update(const int level, const cv::Mat& sample)
{
	_level = level;
	_sample = sample;
	_randomness = _configEx._randomnessSelector(level, sample);
}

cv::Mat PyramidSynthesizerState::sample() const
{
	return _sample;
}

void PyramidSynthesizerState::sample(cv::Mat& sample) const
{
	sample = _sample;
}

int PyramidSynthesizerState::level() const
{
	return _level;
}

void PyramidSynthesizerState::level(int& level) const
{
	level = _level;
}