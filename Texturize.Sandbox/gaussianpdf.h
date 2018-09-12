#pragma once

template <typename T = float>
T normalDistSym(T l, T extent, T alpha)
{
	T sigma = extent * T(0.5);
	T a = (l - alpha) / (sigma / T(3.0));
	return std::exp(-T(0.5) * a * a);
}

template <typename T = float>
T normalDistAssym(T l, T extent, T alpha)
{
	T sigma = l < alpha ? alpha : extent - alpha;
	T a = (l - alpha) / (sigma / T(3.0));
	return std::exp(-T(0.5) * a * a);
}