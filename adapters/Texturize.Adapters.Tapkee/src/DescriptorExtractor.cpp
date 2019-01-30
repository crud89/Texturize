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
	//tapkee::float_distance_callback distanceCallback;
	//tapkee::float_kernel_callback	kernelCallback;
	//tapkee::float_features_callback	featureCallback;

	// Get the pixel neighborhoods and a buffer to store the projected pixels to.
	cv::Mat projected, neighborhoods = this->getPixelNeighborhoods(exemplar, uv);

	// Convert into an Eigen matrix.
	Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> eigenNeighbors;
	cv::cv2eigen(neighborhoods.t(), eigenNeighbors);

	// Apply PCA.
	tapkee::ParametersSet parameters = tapkee::kwargs[
		tapkee::method = tapkee::PCA,
		tapkee::target_dimension = 4
	];

	tapkee::TapkeeOutput result = tapkee::initialize()
		.withParameters(parameters)
		//.withDistance(distanceCallback)
		//.withFeatures(featureCallback)
		//.withKernel(kernelCallback)
		.embedUsing(eigenNeighbors);

	// Convert it back.
	cv::eigen2cv(result.embedding, projected);

	// Return the projected neighborhoods.
	TEXTURIZE_ASSERT(projected.rows == exemplar.channels());

	return projected.t();
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