#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <filesystem>

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>
#include <Codecs/exr.hpp>
#include <Adapters/tapkee.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/core/eigen.hpp>

#include <tapkee/callbacks/precomputed_callbacks.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message.}"
	"{input in          |    | The name of the image file containing the exemplar albedo (rgb) or albedo intensities (greyscale).}"
	"{distances d       |    | The name of a file, containing pairwise distances between pixel descriptors.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{method m          |    | The method used to reduce the input to the control map (\"mds\": Multidimensional Scaling (default), \"isomap\": Isometric Mapping, \"pca\": Principal Component Analysis).}"
	"{neighbors knn     | 7  | The number of neighbors in the neighborhood graph.}"
	"{sigma s           | 0  | Sigma for gaussian blur applied after upsampling.}"
};

// Persistence providers.
DefaultPersistence _persistence;

bool cmpStrI(const std::string& lhs, const std::string& rhs) {
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](const char& l, const char& r) {
		return ::toupper(l) == ::toupper(r);
	});
}

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- Control map generator", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

	if (parser.has("help") || argc == 1)
	{
		parser.printMessage();
		return 0;
	}

	if (!parser.check())
	{
		parser.printErrors();
		return EXIT_FAILURE;
	}

	// Register EXR codec.
	_persistence.registerCodec("txr", std::make_unique<EXRCodec>());

	// Parse parameters.
	std::string inputFileName = parser.get<std::string>("input");
	std::string resultFileName = parser.get<std::string>("result");
	std::string reductionMethod = parser.get<std::string>("method");
	std::string distanceFileName = parser.get<std::string>("distances");
	const int neighbors = parser.get<int>("neighbors");
	const double sigma = parser.get<double>("sigma");
	tapkee::DimensionReductionMethod method;

	// Parse the reduction method.
	if (reductionMethod.empty() || cmpStrI(reductionMethod, "mds"))
		method = tapkee::DimensionReductionMethod::MultidimensionalScaling;
	else if (cmpStrI(reductionMethod, "isomap"))
		method = tapkee::DimensionReductionMethod::Isomap;
	else if (cmpStrI(reductionMethod, "pca"))
		method = tapkee::DimensionReductionMethod::PCA;
	else {
		std::cout << "ERROR: Invalid reduction method." << std::endl;
		parser.printErrors();
		return EXIT_FAILURE;
	}

	std::cout << "Input:" << inputFileName << std::endl <<
		"Distances:" << distanceFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Method: ";

	switch (method)
	{
	case tapkee::DimensionReductionMethod::MultidimensionalScaling:
		std::cout << "Multidimensional Scaling (" << neighbors << " neighbors)" << std::endl;
		break;
	case tapkee::DimensionReductionMethod::Isomap:
		std::cout << "Isometric Mapping (" << neighbors << " neighbors)" << std::endl;
		break;
	case tapkee::DimensionReductionMethod::PCA:
		std::cout << "Principal Component Analysis (" << neighbors << " neighbors)" << std::endl;
		break;
	}

	std::cout << std::endl;

	// Load the albedo and (if neccessary, convert it to greyscale).
	Sample intensities;
	_persistence.loadSample(inputFileName, intensities);

	// In case the sample is colored, convert it to greyscale.
	if (intensities.channels() == 3 || intensities.channels() == 4)
	{
		cv::Mat greyscale;
		cv::cvtColor((cv::Mat)intensities, greyscale, cv::COLOR_RGB2GRAY);
		intensities = Sample(greyscale);
	}
	else if (intensities.channels() != 1)
	{
		std::cout << "The input sample has " << intensities.channels() << " channels and cannot be converted to greyscale. Please convert the sample manually or provide an RGB color image instead." << std::endl;
		return EXIT_FAILURE;
	}

	// Load the matrix of pairwise distances.
	Sample distanceMatrix;
	tapkee::DenseSymmetricMatrix distances;
	_persistence.loadSample(distanceFileName, distanceMatrix);
	cv::cv2eigen((cv::Mat)distanceMatrix, distances);

	// Compute and validate the stride.
	const double aspectRatio = intensities.width() / intensities.height();
	const int stride = intensities.width() / static_cast<int>(std::sqrt(static_cast<double>(distances.rows()) * aspectRatio));
	const int horizontalSamples = intensities.width() / stride;
	const int verticalSamples = intensities.height() / stride;
	const int sampleCount = verticalSamples * horizontalSamples;

	TEXTURIZE_ASSERT(sampleCount == distances.rows());
	
	// Create index vector.
	std::vector<tapkee::IndexType> indices(distances.rows());

	for (tapkee::IndexType idx(0); idx < indices.size(); ++idx)
		indices[idx] = idx;

	// Reduce dimensionality to a 1D manifold.
	tapkee::precomputed_distance_callback distanceCallback(distances);
	tapkee::ParametersSet parameters = tapkee::kwargs[
		tapkee::method = method,
		tapkee::num_neighbors = neighbors,
		tapkee::target_dimension = 1 
		//tapkee::check_connectivity = 0
	];

	std::cout << "Computing low-dimensional embedding...";
	auto start = std::chrono::high_resolution_clock::now();
	tapkee::TapkeeOutput output = tapkee::initialize()
		.withParameters(parameters)
		.withDistance(distanceCallback)
		.embedUsing(indices);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Convert the manifold embedding into a matrix.
	cv::Mat manifold;
	cv::eigen2cv(output.embedding, manifold);

	// Use the embedding to generate the progression map.
	cv::Mat progressionMap = cv::Mat::zeros(horizontalSamples, verticalSamples, CV_32FC1);
	
	// Start by finding the maximum and minimum distances.
	double minCoord, maxCoord;
	cv::minMaxLoc(manifold, &minCoord, &maxCoord);

	TEXTURIZE_ASSERT(maxCoord > minCoord);

	// Normalize the manifold.
	manifold -= minCoord;
	manifold /= (maxCoord - minCoord);

	// Assign each point a 1D coordinate.
	for (int s(0); s < sampleCount; ++s)
	{
		// Get coordinates of the current sample within the exemplar.
		const cv::Point2i sampleCoords(s % horizontalSamples, s / horizontalSamples);
		progressionMap.at<float>(sampleCoords) = 1.f - manifold.at<float>(s);
	}

	// Upsample the progression map.
	cv::resize(progressionMap, progressionMap, cv::Size(intensities.width(), intensities.height()), 0.0, 0.0, cv::INTER_CUBIC);
	
	// Blur, if requested.
	if (sigma > 0.0) {
		const int kernelSize = stride % 2 == 0 ? stride + 1 : stride;
		cv::GaussianBlur(progressionMap, progressionMap, cv::Size(kernelSize, kernelSize), sigma);
	}

	// Compute difference between intensities and (inverse) progression map in order to maximize the progression amplitude.
	cv::Mat difference = progressionMap - (cv::Mat)intensities;
	cv::Mat inverse = (1.f - progressionMap) - (cv::Mat)intensities;

	if (cv::sum(difference)[0] >= cv::sum(inverse)[0])
		progressionMap = 1.f - progressionMap;

	// Store the result.
	Sample result(progressionMap);
	_persistence.saveSample(resultFileName, result);
}