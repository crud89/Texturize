#include "stdafx.h"

#include <sampling.hpp>

#include <cmath>
#include <random>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Perlin's 2d gradient vector hash implementation                                         /////
///////////////////////////////////////////////////////////////////////////////////////////////////

CoordinateHash::CoordinateHash(unsigned int seed, int maskSize) :
	_permutation(maskSize), _gradients(maskSize), _mask(maskSize - 1)
{
	TEXTURIZE_ASSERT(isPoT(maskSize));							// The mask size must be a PoT-number in order to allow bit masking.

	this->init(seed);
}

void CoordinateHash::init(unsigned int seed)
{
	std::mt19937 rng(seed);
	unsigned int ps = static_cast<unsigned int>(_permutation.size());

	for (unsigned int i(0); i < ps; ++i)
	{
		float angle = static_cast<float>(i) / static_cast<float>(ps);

		auto other = rng() % (i + 1);

		if (i > other)
			_permutation[i] = _permutation[other];

		_permutation[other] = i;
		_gradients[i] = cv::Vec2f(cosf(2.f * angle * static_cast<float>(M_PI)), sinf(2.f * angle * static_cast<float>(M_PI)));
	}

	// Shuffle in order for hashes to be distributed non-regularily.
	std::shuffle(_permutation.begin(), _permutation.end(), rng);
}

cv::Vec2i CoordinateHash::calculate(int x, int y) const
{
	auto hash = _permutation[(_permutation[x & _mask] + y) & _mask];
	return _gradients[hash];
}

cv::Vec2i CoordinateHash::calculate(const cv::Vec2i& v) const
{
	return this->calculate(v[0], v[1]);
}