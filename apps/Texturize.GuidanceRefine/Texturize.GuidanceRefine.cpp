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
	"{reference ref     |    | The name of an image file, containing the reference guidance channel that the input should be refined to.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
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

	// Load the input and reference samples.
	Sample inputSample, referenceSample;
	_persistence.loadSample(inputFileName, inputSample);
	_persistence.loadSample(referenceFileName, referenceSample);

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

	// Build up Laplacian pyramids for both, input and refrence samples. 
	LaplacianImagePyramid inputPyramid, referencePyramid;
	inputPyramid.construct(inputSample, inputLevel);
	referencePyramid.construct(referenceSample, referenceLevel);

	// Convert the level count variables to valid indices of the coarsest levels.
	referenceLevel--; inputLevel--;

	// Match histograms of both coarsest levels.
	Sample referenceCoarsestLevel;
	referenceCoarsestLevel = referencePyramid.getLevel(referenceLevel);
	std::unique_ptr<IFilter> filter = std::make_unique<HistogramMatchingFilter>(referenceCoarsestLevel);
	inputPyramid.filterLevel(filter, inputLevel);

	// Apply noise to each level from the second-coarsest to the finest level.
	int maxLevel = std::min(referenceLevel, inputLevel);

	for (int lvl(1); lvl < maxLevel; ++lvl)
	{
		// Reconstruct image pyramid at this level in order to deduce the noise function.
		Sample reference = referencePyramid.getLevel(referenceLevel - lvl);
		//referencePyramid.reconstruct(reference, referenceLevel - l);
		std::unique_ptr<IFilter> noise = MatchingVarianceNoise::FromSample(reference);

		// Add perlin noise to the laplacian at the current level.
		inputPyramid.filterLevel(noise, inputLevel - lvl);
	}

	// Reconstruct the sample that now contains per-level noise.
	Sample refined;
	inputPyramid.reconstruct(refined);

	// Normalize and store the sample.
	//std::unique_ptr<IFilter> normalizationFilter = std::make_unique<NormalizationFilter>();
	//normalizationFilter->apply(refined, refined);
	_persistence.saveSample(resultFileName, refined);

	return 0;
}