#include "stdafx.h"

#include <iostream>
#include <chrono>

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>

#include <opencv2/highgui.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message.}"
	"{input in          |    | The name of an image file or a list of image files (seperated by \";\") that should be clustered.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{bins clusters b c | 64 | The number of clusters in the result.}"
	"{iterations i      | 10 | The number of iterations executed to find an optimal solution.}"
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- k-means Clustering", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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

	// Parse parameters.
	std::string inputFileNames = parser.get<std::string>("input");
	std::string resultFileName = parser.get<std::string>("result");
	int clusters = parser.get<int>("bins");
	int iterations = parser.get<int>("iterations");

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
		"Clusters: " << clusters << std::endl <<
		"Iterations: " << iterations << std::endl << 
		std::endl;

	// Load all input samples into one sample.
	std::vector<Sample> inputSamples;

	for each (auto& fileName in inputFiles)
	{
		Sample sample;
		_persistence.loadSample(fileName, sample);
		inputSamples.push_back(sample);
	}

	Sample material = Sample::mergeSamples(std::initializer_list<const Sample>(inputSamples.data(), inputSamples.data() + inputSamples.size()));

	// Cluster the material.
	Sample result;
	KMeansClusterFilter filter(clusters, iterations);

	auto start = std::chrono::high_resolution_clock::now();
	filter.apply(result, material);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Store the sample.
	_persistence.saveSample(resultFileName, result);
}