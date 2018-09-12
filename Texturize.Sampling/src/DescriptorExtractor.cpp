#include "stdafx.h"

#include <sampling.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Descriptor Extractor implementation                                                     /////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<float> DescriptorExtractor::getProxyPixel(const Sample& exemplar, const cv::Point2i& at, const cv::Vec2i& delta)
{
	TEXTURIZE_ASSERT(delta[0] >= -1 && delta[0] <= 1);
	TEXTURIZE_ASSERT(delta[1] >= -1 && delta[1] <= 1);

	std::vector<std::vector<float>> neighborhood(3);

	exemplar.at(cv::Point2i(at.x + delta[0], at.y + delta[1]), neighborhood[0]);
	exemplar.at(cv::Point2i(at.x + (delta[0] * 2), at.y + delta[1]), neighborhood[1]);
	exemplar.at(cv::Point2i(at.x + delta[0], at.y + (delta[1] * 2)), neighborhood[2]);

	std::vector<float> result(exemplar.channels());

	for (int i(0); i < exemplar.channels(); ++i)
		result[i] = (neighborhood[0][i] + neighborhood[1][i] + neighborhood[2][i]) / 3.f;

	return result;
}

std::vector<float> DescriptorExtractor::getProxyPixel(const Sample& exemplar, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta)
{
	TEXTURIZE_ASSERT(uv.type() == CV_32FC2);						// The UV-Map must be a two-channel single-precision floating point matrix.
	//TEXTURIZE_ASSERT(uv.size() == sample.size());					// The UV-Map must be equally sized as the sample.
	TEXTURIZE_ASSERT(delta[0] >= -1 && delta[0] <= 1);
	TEXTURIZE_ASSERT(delta[1] >= -1 && delta[1] <= 1);

	// Resolve the pixel coordinates from the UV-Map.
	cv::Vec2f coords[3];
	cv::Point2i point;

	point = cv::Point2i(at.x + delta[0], at.y + delta[1]);
	Sample::wrapCoords(uv.cols, uv.rows, point);
	coords[0] = uv.at<cv::Vec2f>(point);

	point = cv::Point2i(at.x + (delta[0] * 2), at.y + delta[1]);
	Sample::wrapCoords(uv.cols, uv.rows, point);
	coords[1] = uv.at<cv::Vec2f>(point);

	point = cv::Point2i(at.x + delta[0], at.y + (delta[1] * 2));
	Sample::wrapCoords(uv.cols, uv.rows, point);
	coords[2] = uv.at<cv::Vec2f>(point);

	// Get the actual pixel values.
	std::vector<std::vector<float>> neighborhood(3);

	exemplar.at(coords[0], neighborhood[0]);
	exemplar.at(coords[1], neighborhood[1]);
	exemplar.at(coords[2], neighborhood[2]);

	// Calculate the average values and store it within the result vector.
	std::vector<float> result(exemplar.channels());

	for (int i(0); i < exemplar.channels(); ++i)
		result[i] = (neighborhood[0][i] + neighborhood[1][i] + neighborhood[2][i]) / 3.f;

	return result;
}

cv::Mat DescriptorExtractor::indexNeighborhoods(const Sample& exemplar)
{
	// Create a UV-Map for the sample.
	cv::Mat uv = this->createContinuousUvMap(exemplar);

	// Calculate the descriptors.
	// NOTE: This one calls the overload, where the projector is non-constant (due to the method's non-const scope)
	//		 The projector will be created and stored. Re-calling this method will alter the projector. Successive calls should therefor
	//       go to calculateNeighborhoodDescriptors directly, which is implemented similar, but does not affect the existing projector.
	//       If there is a projector, it will be used, otherwise a temporary one will be created in this (const) case.
	// 
	//		 In order to disable this behaviour and let the extractor create new projections with each call, pass nullptr as projector in 
	//       here, or do not call this method from client code (instead call one of the public overloads of
	//       `calculateNeighborhoodDescriptors` directly).
	return this->calculateNeighborhoodDescriptors(exemplar, uv, this->_projector);
	//return this->calculateNeighborhoodDescriptors(exemplar, uv, nullptr);
}

cv::Mat DescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar) const
{
	// Create a UV-Map for the sample.
	cv::Mat uv = this->createContinuousUvMap(exemplar);

	// Calculate the descriptors.
	return this->calculateNeighborhoodDescriptors(exemplar, uv);
}

cv::Mat DescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const
{
	// Calculate the descriptors.
	return this->calculateNeighborhoodDescriptors(exemplar, uv, this->_projector);
}

cv::Mat DescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv, std::unique_ptr<cv::PCA>& projector) const
{
	// Get the pixel neighborhoods;
	cv::Mat projected, neighborhoods = this->getPixelNeighborhoods(exemplar, uv);

	// If the projector is provided, use it - otherwise create a new one and return it.
	if (projector.get() == nullptr)
		projector = std::make_unique<cv::PCA>(neighborhoods, cv::Mat(), cv::PCA::DATA_AS_COL, static_cast<int>(exemplar.channels()));

	// Project the pixel neighborhoods for efficient search.
	projected = projector->project(neighborhoods);

	// Return the projected neighborhoods.
	TEXTURIZE_ASSERT(projected.rows == exemplar.channels());

	return projected.t();
}

cv::Mat DescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv, const std::unique_ptr<cv::PCA>& projector) const
{
	// Get the pixel neighborhoods;
	cv::Mat projected, neighborhoods = this->getPixelNeighborhoods(exemplar, uv);

	// If the projector is provided, use it - otherwise create a new one.
	if (projector.get() != nullptr)
		projected = projector->project(neighborhoods);
	else
	{
		std::unique_ptr<cv::PCA> prj = std::make_unique<cv::PCA>(neighborhoods, cv::Mat(), cv::PCA::DATA_AS_COL, static_cast<int>(exemplar.channels()));
		projected = prj->project(neighborhoods);
	}

	// Return the projected neighborhoods.
	TEXTURIZE_ASSERT(projected.rows == exemplar.channels());

	return projected.t();
}

cv::Mat DescriptorExtractor::createContinuousUvMap(const Sample& exemplar) const
{
	cv::Mat uv(exemplar.size(), CV_32FC2);
	const int width = exemplar.width(), height = exemplar.height();

	uv.forEach<cv::Vec2f>([&width, &height](cv::Vec2f& uv, const int* idx) -> void {
		uv[0] = static_cast<float>(idx[1]) / static_cast<float>(width);
		uv[1] = static_cast<float>(idx[0]) / static_cast<float>(height);
	});
	
	return uv;
}

cv::Mat DescriptorExtractor::getPixelNeighborhoods(const Sample& exemplar, const cv::Mat& uv) const
{
	TEXTURIZE_ASSERT(uv.type() == CV_32FC2);						// The UV-Map must be a two-channel single-precision floating point matrix.
	//TEXTURIZE_ASSERT(uv.size() == sample.size());					// The UV-Map must be equally sized as the sample.

	// Create a matrix that stores 4 proxy pixels of each pixel of the sample in a column.
	cv::Mat neighborhoods(exemplar.channels() * 4, uv.rows * uv.cols, CV_32F);

	// Calculate the neighborhoods for each pixel.
	const cv::Size size = uv.size();

	// TODO: Implement this on GPU!
	uv.forEach<cv::Vec2f>([&neighborhoods, &exemplar, &size, &uv](const cv::Vec2f& at, const int* idx) -> void {
		// Calculate the neighborhood column coordinate for the pixel.
		int col = idx[0] * size.width + idx[1];

		// Calculate the proxy pixels.
		std::vector<float> pixel(exemplar.channels()), texel(exemplar.channels() * 4);

		// Top Left
		pixel = DescriptorExtractor::getProxyPixel(exemplar, uv, cv::Point2i(idx[1], idx[0]), cv::Vec2i(-1, -1));
		std::copy(pixel.begin(), pixel.end(), texel.begin() + (0 * exemplar.channels()));
		
		// Bottom Left
		pixel = DescriptorExtractor::getProxyPixel(exemplar, uv, cv::Point2i(idx[1], idx[0]), cv::Vec2i(-1, 1));
		std::copy(pixel.begin(), pixel.end(), texel.begin() + (1 * exemplar.channels()));

		// Top Right
		pixel = DescriptorExtractor::getProxyPixel(exemplar, uv, cv::Point2i(idx[1], idx[0]), cv::Vec2i(1, -1));
		std::copy(pixel.begin(), pixel.end(), texel.begin() + (2 * exemplar.channels()));

		// Bottom Right
		pixel = DescriptorExtractor::getProxyPixel(exemplar, uv, cv::Point2i(idx[1], idx[0]), cv::Vec2i(1, 1));
		std::copy(pixel.begin(), pixel.end(), texel.begin() + (3 * exemplar.channels()));

		// Store the texel within the neighborhood set.
		cv::Mat(exemplar.channels() * 4, 1, CV_32F, texel.data()).copyTo(neighborhoods.col(col));
	});

	return neighborhoods;
}