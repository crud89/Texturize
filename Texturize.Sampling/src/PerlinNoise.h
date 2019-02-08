
//
//	PerlinNoise.{h|cpp} - perlin noise image generator.
//
// github:
//     https://github.com/yoggy/cv_perlin_noise
//
// license:
//     Copyright (c) 2015 yoggy <yoggy0@gmail.com>
//     Released under the MIT license
//     http://opensource.org/licenses/mit-license.php
//
// reference:
//     http://cs.nyu.edu/~perlin/noise/
//
#pragma once

#pragma warning(disable : 4819)

#include <texturize.hpp>
#include <analysis.hpp>

cv::Mat CreatePerlinNoiseImage(const cv::Size &size, const double &scale = 0.05);