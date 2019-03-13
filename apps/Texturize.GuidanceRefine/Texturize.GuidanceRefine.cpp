#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

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
	"{input in          |    | The name of an image file, containing the guidance channel that should be refined.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{reference ref     |    | The name of an image file, containing the reference guidance channel that the input should be refined to.}"
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- Guidance channel refinement", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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
	std::string referenceFileName = parser.get<std::string>("reference");

	std::cout << "Input: " << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Reference: " << referenceFileName << std::endl <<
		std::endl;

	// Load the input and reference samples.
	Sample inputSample, referenceSample;
	_persistence.loadSample(inputFileName, inputSample);
	_persistence.loadSample(referenceFileName, referenceSample);

	// Try to convert the input samples to grayscale, if they aren't already.
	GrayscaleFilter toGrey;

	if (inputSample.channels() > 1)
		toGrey.apply(inputSample, inputSample);

	if (referenceSample.channels() > 1)
		toGrey.apply(referenceSample, referenceSample);

	// Comput the coarsest (smallest) level of the pyramid from the maximum possible number of levels, skipping the 3 coarsest ones.
	int inputLevel = (inputSample.width() >= inputSample.height() ? log2(inputSample.width()) : log2(inputSample.height())) - 3;
	int referenceLevel = (referenceSample.width() >= referenceSample.height() ? log2(referenceSample.width()) : log2(referenceSample.height())) - 3;

	if (inputLevel < 0)
	{
		std::cout << "Error: the provided image \"" << inputFileName << "\" resolution (" << inputSample.width() << "x" << inputSample.height() << " Px) is too low." << std::endl;
		return EXIT_FAILURE;
	}

	if (referenceLevel < 0)
	{
		std::cout << "Error: the provided image \"" << referenceFileName << "\" resolution (" << referenceSample.width() << "x" << referenceSample.height() << " Px) is too low." << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Refining guidance... ";
	auto start = std::chrono::high_resolution_clock::now();

	// Build up Laplacian pyramids for both, input and refrence samples. 
	LaplacianImagePyramid inputPyramid, referencePyramid;
	inputPyramid.construct(inputSample, inputLevel);
	referencePyramid.construct(referenceSample, referenceLevel);

	// Convert the level count variables to valid indices of the coarsest levels.
	//referenceLevel--; inputLevel--;

	// Match histograms of both coarsest levels.
	Sample referenceCoarsestLevel;
	referenceCoarsestLevel = referencePyramid.getLevel(0);
	std::unique_ptr<IFilter> filter = std::make_unique<HistogramMatchingFilter>(referenceCoarsestLevel);
	inputPyramid.filterLevel(filter, 0);

	// Apply noise to each level from the second-coarsest to the finest level.
	int maxLevel = std::min(referenceLevel, inputLevel);

	for (int lvl(1); lvl < maxLevel; ++lvl)
	{
		// Reconstruct image pyramid at this level in order to deduce the noise function.
		Sample reference = referencePyramid.getLevel(lvl);
		//referencePyramid.reconstruct(reference, referenceLevel - lvl);
		std::unique_ptr<IFilter> noise = MatchingVarianceNoise::FromSample(reference);

		// Add perlin noise to the laplacian at the current level.
		inputPyramid.filterLevel(noise, lvl);
	}

	// Reconstruct the sample that now contains per-level noise.
	Sample refined;
	inputPyramid.reconstruct(refined, maxLevel);

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Normalize and store the sample.
	//std::unique_ptr<IFilter> normalizationFilter = std::make_unique<NormalizationFilter>();
	//normalizationFilter->apply(refined, refined);
	_persistence.saveSample(resultFileName, refined);
}