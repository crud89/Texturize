#include "stdafx.h"

#include <iostream>
#include <chrono>
#include <algorithm>

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>
#include <Codecs/exr.hpp>
#include <Adapters/tapkee.hpp>

#include <opencv2/highgui.hpp>

#include <tapkee/callbacks/precomputed_callbacks.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message.}"
	"{input in          |    | The name of an image file or a list of image files (seperated by \";\") that should be mapped.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{method m          |    | The method used to reduce the input to the control map (\"mds\": Multidimensional Scaling (default), \"isomap\": Isometric Mapping, \"pca\": Principal Component Analysis).}"
	"{stride s          | 8  | The stride between two descriptor windows, used to speed up calculation. Undersampled points are interpolated.}"
	"{kernel k          | 49 | The size of the kernel window around each pixel to calculate the histogram in.}"
	"{bins b            | 64 | The number of bins per sample histogram. This also represents the depth of an individual pixel descriptor.}"
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
	std::string inputFileNames = parser.get<std::string>("input");
	std::string resultFileName = parser.get<std::string>("result");
	std::string reductionMethod = parser.get<std::string>("method");
	int stride = parser.get<int>("stride");
	int kernel = parser.get<int>("kernel");
	int bins = parser.get<int>("bins");
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

	// Get the individual input file names.
	std::vector<std::string> inputFiles;
	std::istringstream tokens(inputFileNames);
	std::string token;

	while (std::getline(tokens, token, ';'))
	{
		if (token.empty())
			continue;

		inputFiles.push_back(token);
	}

	for (size_t file(0); file < inputFiles.size(); ++file)
		std::cout << "Input [" << file + 1 << "/" << inputFiles.size() << "]: " << inputFiles[file] << std::endl;

	std::cout << "Output: " << resultFileName << std::endl <<
		"Method: " << reductionMethod << std::endl <<
		"Stride: " << stride << std::endl <<
		std::endl;

	// Load all input samples.
	HistogramExtractionFilter filter(bins, kernel, stride);
	std::vector<cv::Mat> samples;

	for each (auto& fileName in inputFiles)
	{
		std::cout << "Computing descriptors for \"" << fileName << "\"... ";

		// Load the sample.
		Sample sample, result;
		_persistence.loadSample(fileName, sample);

		// Get the pixel histograms from the sample and store it.
		filter.apply(result, sample);
		samples.push_back((cv::Mat)result);

		std::cout << "Done!" << std::endl;
	}

	// Compute the distance matrix.
	std::vector<tapkee::IndexType> indices;

	std::cout << "Computing distance matrix... ";
	Tapkee::PairwiseDistanceExtractor distanceExtractor(std::make_unique<Tapkee::EarthMoversDistanceMetric>());
	tapkee::DenseSymmetricMatrix distances = distanceExtractor.computeDistances(samples, indices);
	std::cout << "Done!" << std::endl;

	// 
	tapkee::precomputed_distance_callback distanceCallback(distances);
	tapkee::ParametersSet parameters = tapkee::kwargs[
		tapkee::method = method, tapkee::target_dimension = 1
	];

	std::cout << "Performing dimensionality reduction... ";
	auto start = std::chrono::high_resolution_clock::now();
	tapkee::TapkeeOutput output = tapkee::initialize()
		.withParameters(parameters)
		.withDistance(distanceCallback)
		.embedUsing(indices);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	tapkee::DenseMatrix manifold = output.embedding.transpose();
}