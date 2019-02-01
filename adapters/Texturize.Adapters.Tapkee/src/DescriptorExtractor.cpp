#include "stdafx.h"

#include <Adapters/tapkee.hpp>
#include <sampling.hpp>

#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <opencv2/core/eigen.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// PCA descriptor extractor implementation                                                 /////
///////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat Tapkee::PCADescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar) const
{
	// Create a UV-Map for the sample.
	cv::Mat uv = this->createContinuousUvMap(exemplar);

	// Calculate the descriptors.
	return this->calculateNeighborhoodDescriptors(exemplar, uv);
}

cv::Mat Tapkee::PCADescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const
{
	// Get the pixel neighborhoods and a buffer to store the projected pixels to.
	cv::Mat projected, neighborhoods = this->getPixelNeighborhoods(exemplar, uv);
	
	// Convert into an Eigen matrix.
	tapkee::DenseMatrix eigenNeighbors;
	cv::cv2eigen(neighborhoods, eigenNeighbors);

	// Apply PCA.
	tapkee::ParametersSet parameters = tapkee::kwargs[
		tapkee::method = tapkee::PCA,
		tapkee::target_dimension = exemplar.channels()
	];

	tapkee::TapkeeOutput result = tapkee::initialize()
		.withParameters(parameters)
		.embedUsing(eigenNeighbors);

	// Convert it back.
	cv::eigen2cv(result.embedding, projected);

	// Return the projected neighborhoods.
	TEXTURIZE_ASSERT(projected.cols == exemplar.channels());

	return projected;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// t-SNE descriptor extractor implementation                                               /////
///////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat Tapkee::SNEDescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar) const
{
	throw;
}

cv::Mat Tapkee::SNEDescriptorExtractor::calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const
{
	throw;
}