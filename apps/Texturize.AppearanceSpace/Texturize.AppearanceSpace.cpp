#include "stdafx.h"

#include <iostream>
#include <chrono>

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>
#include <Codecs/exr.hpp>

#include <opencv2/highgui.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message.}"
	"{input in          |    | The name of an image (RGB or grey) file that should be filtered.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- Appearance Space Transform", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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

	// Print parameters.
	std::string inputFileName = parser.get<std::string>("input");
	std::string resultFileName = parser.get<std::string>("result");

	std::cout << "Input: " << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		std::endl;

	// Load the input sample.
	Sample sample;
	_persistence.loadSample(inputFileName, sample);

	// Store the sample.
	_persistence.saveSample(resultFileName, result);
}