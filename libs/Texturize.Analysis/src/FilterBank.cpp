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
		for (int y(relativeKernel); y >= -relativeKernel; --y)
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

	cv::normalize(kernel, kernel);
	return kernel;
}

/// \private
cv::Mat getGaussianKernel(int ksize, float sigma) {
	cv::Mat kernel(cv::Size(ksize, ksize), CV_32FC1);
	cv::Mat linearKernel = cv::getGaussianKernel(ksize, sigma, CV_32F);

	kernel.forEach<float>([&linearKernel](float& val, const int* idx) -> void {
		val = linearKernel.at<float>(idx[1], 0) * linearKernel.at<float>(idx[0], 0);
	});

	cv::normalize(kernel, kernel);
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
	const float sine	= sinf(phi);
	const float cosine	= cosf(phi);
	std::vector<float> rotations = { cosine, sine, -sine, cosine };
	cv::Mat rotator = cv::Mat(2, 2, CV_32FC1, rotations.data());
	cv::Mat rotatedPoints(2, pts.size(), CV_32FC1);

	for (std::vector<cv::Point2f>::size_type i(0); i < pts.size(); ++i)
	{
		cv::Mat rt = rotator * cv::Mat(pts[i]);
		rt.copyTo(rotatedPoints.col(i));
	}

	// Calculate horizontal gaussian kernel.
	::gauss1d(rotatedPoints.row(0), 0, 3 * scale, mean);
	::gauss1d(rotatedPoints.row(1), order, scale, mean);

	// Calculate the 2d kernel.
	cv::Mat kernel(cv::Size(ksize, ksize), CV_32F);
	auto data = kernel.ptr<float>(0);

	for (int i(0); i < rotatedPoints.cols; ++i)
		data[i] = rotatedPoints.at<float>(0, i) * rotatedPoints.at<float>(1, i);

	// Normalize.
	cv::normalize(kernel, kernel);

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
	constexpr float angularDifference = (1.f / 6.f) * M_PI;
	std::vector<cv::Mat> edgeKernels, barKernels;
	float phi(0.f);

	// At each scale, build six edge and six bar filters.
	for (int scale(0); scale < 3; ++scale)
	for (int angle(0); angle < 6; ++angle, phi += angularDifference)
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
	TEXTURIZE_ASSERT(sample.channels() == 1);								// Only greyscale/intensity images are allowed.

	// There are 3 scales for each filter mode and 6 orientations for each filter at each
	// scale, plus two orientation and scale invariant filter kernels. The filter bank is
	// computed by applying each kernel, keeping only the maximum response for a pixel at
	// a certain scale.
	std::vector<cv::Mat> responses(8);
	int filterIndex(0), responseIndex(0);

	// Apply filters of different modes: mode 0 represents edge, mode 1 bar filters.
	for (int mode(0); mode < 2; ++mode)
	{
		for (int scale(0); scale < 3; ++scale)
		{
			// Store the maximum response for each scale.
			cv::Mat maxResponse = cv::Mat::zeros(sample.size(), CV_32F);

			for (int angle(0); angle < 6; ++angle)
			{
				// Start by requesting the kernel.
				cv::Mat kernel = _rootFilterSet.getChannel(filterIndex++);

				// Next, filter the image.
				cv::Mat filterResponse;
				cv::filter2D((cv::Mat)sample, filterResponse, CV_32F, kernel);

				// In case of edge filter mode (1st order derivative), take the absolute.
				if (mode == 0)
					filterResponse = cv::abs(filterResponse);

				// Overwrite the values, where they are greater.
				maxResponse = cv::max(filterResponse, maxResponse);
			}

			// Keep the maximum response for the current scale.
			responses[responseIndex++] = maxResponse;
		}
	}

	// Finally, apply the remaining two kernels.
	TEXTURIZE_ASSERT_DBG(responseIndex == 6);
	TEXTURIZE_ASSERT_DBG(filterIndex == 36);

	// First the gaussian kernel.
	cv::Mat gaussResponse, gaussKernel = _rootFilterSet.getChannel(filterIndex++);
	cv::filter2D((cv::Mat)sample, gaussResponse, CV_32F, gaussKernel);
	responses[responseIndex++] = gaussResponse;

	// Then the Laplacian of Gaussian kernel (LoG).
	cv::Mat laplacianResponse, laplacianKernel = _rootFilterSet.getChannel(filterIndex++);
	cv::filter2D((cv::Mat)sample, laplacianResponse, CV_32F, laplacianKernel);
	responses[responseIndex++] = laplacianResponse;

	// The result should be an 8 channel sample containing the maximum responses.
	TEXTURIZE_ASSERT_DBG(responseIndex == 8);
	TEXTURIZE_ASSERT_DBG(filterIndex == 38);

	result = Sample(responseIndex, sample.size());
	
	for (int i(0); i < static_cast<int>(result.channels()); ++i)
		result.setChannel(i, responses[i]);
}