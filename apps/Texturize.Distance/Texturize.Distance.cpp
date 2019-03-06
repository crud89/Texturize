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
	"{input in          |    | The name of an image file or a list of image files (seperated by \";\") that should be mapped.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{norm n            |    | The distance norm to compute pairwise distances (\"emd\": Earth Mover's Distance (default), \"L2\": Euclidean Distance, \"ChiSqr\": Chi Squared Distance).}"
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

enum DistanceNorm {
	EarthMovers	= 0x01,
	Euclidean	= 0x02,
	ChiSquared	= 0x04
};

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
	std::string norm = parser.get<std::string>("norm");
	int stride = parser.get<int>("stride");
	int kernel = parser.get<int>("kernel");
	int bins = parser.get<int>("bins");
	DistanceNorm distanceNorm;

	// Parse the reduction method.
	if (norm.empty() || cmpStrI(norm, "emd"))
		distanceNorm = DistanceNorm::EarthMovers;
	else if (cmpStrI(norm, "l2"))
		distanceNorm = DistanceNorm::Euclidean;
	else if (cmpStrI(norm, "chisqr"))
		distanceNorm = DistanceNorm::ChiSquared;
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
		"Stride: " << stride << std::endl <<
		"Distance Norm: ";

	// Define a distance metric.
	std::unique_ptr<Texturize::Tapkee::IDistanceMetric> metric;

	switch (distanceNorm)
	{
	case DistanceNorm::EarthMovers:
		std::cout << "Earth Mover's Distance" << std::endl;
		metric = std::make_unique<Tapkee::EarthMoversDistanceMetric>();
		break;
	case DistanceNorm::Euclidean:
		std::cout << "Euclidean Distance" << std::endl;
		metric = std::make_unique<Tapkee::EuclideanDistanceMetric>();
		break;
	case DistanceNorm::ChiSquared:
		std::cout << "Chi-Squared Distance" << std::endl;
		std::cout << "ERROR: This distance norm is currently not implemented." << std::endl;
		parser.printErrors();

		return EXIT_FAILURE;
	}

	std::cout << std::endl;

	// Create a distance extractor.
	Tapkee::PairwiseDistanceExtractor distanceExtractor(std::move(metric));

	// Load all input samples.
	HistogramExtractionFilter filter(bins, kernel, stride);
	std::vector<cv::Mat> samples;
	int totalRows{ -1 };

	for each (auto& fileName in inputFiles)
	{
		std::cout << "Computing descriptors for \"" << fileName << "\"... ";

		// Load the sample.
		Sample sample, result;
		_persistence.loadSample(fileName, sample);

		if (totalRows == -1)
			totalRows = sample.height();
		else
			TEXTURIZE_ASSERT(totalRows == sample.height());

		// Get the pixel histograms from the sample and store it.
		filter.apply(result, sample);
		samples.push_back((cv::Mat)result);

		std::cout << "Done!" << std::endl;
	}

	// Calculate matrix of pairwise distances.
	std::vector<tapkee::IndexType> indices;

	std::cout << "Computing distance matrix...";
	auto start = std::chrono::high_resolution_clock::now();
	tapkee::DenseSymmetricMatrix distances = distanceExtractor.computeDistances(samples, indices);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Save the distance matrix.
	cv::Mat distanceMatrix;
	cv::eigen2cv(distances, distanceMatrix);
	_persistence.saveSample(resultFileName, Sample(distanceMatrix));
}