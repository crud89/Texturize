#include "stdafx.h"

#include <analysis.hpp>

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

#include <cmath>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Helper functions for missing OpenCV methods.								              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

/// \private
void getSamplingPoints(std::vector<cv::Point2f>& pts, const int ksize) {
	int relativeKernel = ksize / 2;

	for (int x(-relativeKernel); x <= relativeKernel; ++x)
		for (int y(relativeKernel); y >= relativeKernel; --y)
			pts.push_back(cv::Point2f(static_cast<float>(x), static_cast<float>(y)));
}

/// \private
void gauss1d(cv::Mat& points, const int phase, const float sigma, const float mean = 0) {
	TEXTURIZE_ASSERT(points.rows == 1 || points.cols == 1);				// The point array should be 1D.

	// Transpose, so that all coordinates are within row(0).
	if (points.cols == 1)
		points = points.t();

	// Get the variance and gauss denominator.
	const float variance = pow(sigma, 2.f);
	const float denominator = variance * 2.f;

	points.forEach<float>([&phase, &variance, &denominator, &mean](float& val, const int* idx) -> void {
		// Compute gaussian (order 0).
		val -= mean;
		const float squared = (val *= val);
		val = exp(-val / denominator) / sqrt(M_PI * denominator);

		// Depending on the phase, compute the 1st or 2nd order derivative.
		switch (phase) {
		case 1:
			val = -val * (val / variance);
			break;
		case 2:
			val = val * ((squared - variance) / pow(variance, 2.f));
			break;
		}
	});
}

/// \private
float laplacianOfGaussian(const int x, const int y, const float sigma) {
	float scale = ((x * x) + (y * y)) / (2 * (sigma * sigma));
	return -1.f / (M_PI * pow(sigma, 4)) * (1 - scale) * exp(-scale);
}

/// \private
cv::Mat getLaplacianOfGaussianKernel(int ksize, float sigma) {
	TEXTURIZE_ASSERT(ksize % 2 == 1);									// Kernel size must be odd.

	// Generate the kernel.
	cv::Mat kernel(cv::Size(ksize, ksize), CV_32F);
	int halfSize = ksize / 2;

	kernel.forEach<float>([&halfSize, &sigma](float& val, const int* idx) -> void {
		val = laplacianOfGaussian(idx[1] - halfSize, idx[0] - halfSize, sigma);
	});

	return kernel;
}

/// \private
/// \brief
///
///
/// \param ksize
/// \param sigma
/// \param order
/// \param ktype
cv::Mat getDerivGaussianKernel(int ksize, float scale, float mean = 0, float phi = 0.f, int order = 0) {
	TEXTURIZE_ASSERT(ksize % 2 == 1);									// Kernel size must be odd.
	TEXTURIZE_ASSERT(order >= 0 && order < 3);							// Currently only first and second order derivatives are supported.

	// Compute the sampling points, which in general represents a set of points into each 
	// direction (horizontal and vertical). The gaussian function is sampled by iterating
	// a oriented set of those points.
	std::vector<cv::Point2f> pts;
	::getSamplingPoints(pts, ksize);

	// Calculate the rotation matrix and rotate all points.
	cv::Mat rotator = cv::getRotationMatrix2D(cv::Point2f(0.f, 0.f), phi, 1.f);
	cv::Mat rotatedPoints(2, pts.size(), CV_32F);

	for (std::vector<cv::Point2f>::size_type i(0); i < pts.size(); ++i)
		rotatedPoints.col(i).setTo(rotator * cv::Mat(pts[i]));

	// Calculate horizontal gaussian kernel.
	::gauss1d(rotatedPoints.row(0), 0, 3 * scale, mean);
	::gauss1d(rotatedPoints.row(1), order, scale, mean);

	// Calculate the 2d kernel.
	cv::Mat kernel(cv::Size(ksize, ksize), CV_32F);
	auto data = kernel.ptr<float>(0);

	for (int i(0); i < rotatedPoints.cols; ++i)
		data[i] = rotatedPoints.at<float>(0, i) * rotatedPoints.at<float>(1, i);

	// TODO: Normalize?!

	// Return the kernel.
	return kernel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Maximum Response (MR8, MRS4, MR4) filter bank implementation				              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

MaxResponseFilterBank::MaxResponseFilterBank(const int kernelSize) :
	_rootFilterSet(this->computeRootFilterSet(kernelSize))
{
}

Sample MaxResponseFilterBank::computeRootFilterSet(const int kernelSize) const
{
	Sample rootFilterSet;
	this->computeRootFilterSet(rootFilterSet, kernelSize);
	return rootFilterSet;
}

void MaxResponseFilterBank::computeRootFilterSet(Sample& bank, const int kernelSize) const
{
	TEXTURIZE_ASSERT(kernelSize % 2 == 1);								// The kernel window extent needs to be odd.

	// The MR filter bank uses a root filter set (RFS) at three scales into six different 
	// orientations (i.e. 3 * 6 = 18). Those 18 kernels are generated for two filters:
	// - An edge filter kernel
	// - A bar filter kernel
	// Additionally, two orientation-invariant kernels - Gaussian and Laplacian of Gaussian 
	// (LoG) - are added, resulting in a RFS of 38 filter kernels.
	//
	// For more information and a detailled explanation, refer to:
	// http://www.robots.ox.ac.uk/~vgg/research/texclass/filters.html

	// The three scale coefficients for edge and bar filter kernels.
	constexpr float scales[] = { 1.f, 2.f, 4.f };
	// The angle between two kernels at the same scale (6 discrete steps = 1/6*Pi)
	constexpr float angle = (1.f / 6.f) * M_PI;
	std::vector<cv::Mat> edgeKernels, barKernels;

	// At each scale, build six edge and six bar filters.
	for (int scale(0); scale < sizeof(scales); scale++)
	for (float phi(0.f); phi < M_PI; phi += angle)
	{
		edgeKernels.push_back(::getDerivGaussianKernel(kernelSize, scales[scale], 0.0, phi, 1));
		barKernels.push_back(::getDerivGaussianKernel(kernelSize, scales[scale], 0.0, phi, 2));
	}

	// Copy the channels into the filter bank.
	bank = Sample(38);
	int filterIdx(0);

	for each(const auto& kernel in edgeKernels)
		bank.setChannel(filterIdx++, kernel);

	for each(const auto& kernel in barKernels)
		bank.setChannel(filterIdx++, kernel);

	// Calculate Gaussian and Laplacian of Gaussian kernels.
	bank.setChannel(filterIdx++, cv::getGaussianKernel(kernelSize, 10.f, CV_32F));
	bank.setChannel(filterIdx++, ::getLaplacianOfGaussianKernel(kernelSize, 10.f));

	TEXTURIZE_ASSERT_DBG(bank.channels() == 38);							// The MR filter bank RFS has 38 kernels.
}

void MaxResponseFilterBank::apply(Sample& result, const Sample& sample) const
{
	throw;
}