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
	"{input in          |    | The name of a file, containing pairwise distances between pixel descriptors.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{method m          |    | The method used to reduce the input to the control map (\"mds\": Multidimensional Scaling (default), \"isomap\": Isometric Mapping, \"pca\": Principal Component Analysis).}"
	"{neighbors kn      | 7  | The number of neighbors in the neighborhood graph.}"
	"{width w           |    | The number of horizontal pixels of the progression map.}"
	"{height h          |    | The number of vertical pixels of the progression map.}"
	"{stride s          | 8  | The spacing between pixel samples.}"
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
	const int neighbors = parser.get<int>("neighbors");
	const int width = parser.get<int>("width");
	const int height = parser.get<int>("height");
	const int stride = parser.get<int>("stride");
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

	std::cout << "Distances:" << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Method: ";

	switch (method)
	{
	case tapkee::DimensionReductionMethod::MultidimensionalScaling:
		std::cout << "Multidimensional Scaling" << std::endl;
		break;
	case tapkee::DimensionReductionMethod::Isomap:
		std::cout << "Isometric Mapping" << std::endl;
		break;
	case tapkee::DimensionReductionMethod::PCA:
		std::cout << "Principal Component Analysis" << std::endl;
		break;
	}

	std::cout << std::endl;

	// Load the matrix of pairwise distances.
	Sample distanceMatrix;
	tapkee::DenseSymmetricMatrix distances;
	_persistence.loadSample(inputFileName, distanceMatrix);
	cv::cv2eigen((cv::Mat)distanceMatrix, distances);

	// Validate the input dimensions against the distance matrix.
	const int horizontalSamples = width / stride;
	const int verticalSamples = height / stride;
	const int sampleCount = horizontalSamples * verticalSamples;
	
	if (sampleCount != distances.rows()) {
		std::cout << "ERROR: The number of samples (" << sampleCount << ") mismatches the expected number of samples (" << distances.rows() << ")." << std::endl <<
			"Make sure to provide the sample stride as used for calculating the distance matrix. Also the width and height parameters should match the exemplar dimensions." << std::endl;
		return EXIT_FAILURE;
	}

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

	std::cout << "Performing low-dimensional embedding...";
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
	cv::Mat frequencyMap = cv::Mat::zeros(horizontalSamples, verticalSamples, CV_32SC1);
	
	// Start by finding the maximum and minimum distances.
	double minCoord, maxCoord;
	cv::minMaxLoc(manifold, &minCoord, &maxCoord);

	TEXTURIZE_ASSERT(maxCoord > minCoord);

	// 
	cv::subtract(manifold, cv::Scalar(minCoord), manifold);
	std::cout << manifold << std::endl;
	cv::divide(maxCoord - minCoord, manifold, manifold);
	std::cout << manifold << std::endl;

	// 


	//manifold = manifold.reshape(1, totalRows / stride);
	//Sample result(manifold);
	//_persistence.saveSample(resultFileName, result);

	cv::imshow("Manifold", manifold);
	cv::waitKey(0);
}