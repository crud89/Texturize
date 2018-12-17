#include <stdint.h>
#include <intrin.h>

#pragma intrinsic(_BitScanReverse)

/// \brief Approximates the log2 of an integral 32-bit value, by counting the index of the most significant bit of the input.
static inline uint32_t log2(const uint32_t x) {
	unsigned long y(0);

	if (x > 0)
		_BitScanReverse(&y, x);

	return y;
}

#ifdef _WIN64
/// \brief Approximates the log2 of an integral 64-bit value, by counting the index of the most significant bit of the input.
static inline uint32_t log2(const uint64_t x) {
	unsigned long y(0);

	if (x > 0)
		_BitScanReverse64(&y, x);

	return y;
}
#else
/// \brief Approximates the log2 of an integral 64-bit value, by counting the index of the most significant bit of the input.
static inline uint32_t log2(const uint64_t x) {
	return x > 0 ? std::log2l(x) : 0;
}
#endif

/// \brief If the parameter \p from already is a *power-of-two* number, it will be returned unchanged, otherwise the next highest number within the power-of-two is calculated.
///
/// \param from A number to start searching for the next highest power-of-two number from.
/// \returns The next highest power-of-two number, starting from \p from, or \p from, if it already is a power-of-two number.
static inline int nextPoT(int from)
{
	TEXTURIZE_ASSERT(from > 0);

	// Get the index of the most significant bit and create a new number by shifting one towards the index.
	int msb = log2(static_cast<uint32_t>(from));
	int r = 1 << msb;

	// If the result equals the input, the input already is a valid PoT number, otherwise perform an additional shift.
	return r == from ? r : r << 1;
}

/// \brief Checks a given value \p v if it's a power-of-two number.
///
/// \param v A value that should be checked to be a power-of-two number.
/// \returns `true`, if the provided value \p v is a power-of-two number, otherwise `false`.
static inline bool isPoT(int n)
{
	// http://www.graphics.stanford.edu/~seander/bithacks.html
	return n && !(n & (n - 1));
}