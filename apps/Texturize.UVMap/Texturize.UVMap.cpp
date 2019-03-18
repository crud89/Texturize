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
	"{input in          |    | The name of an image file that should be remapped.}"
	"{uvmap uv          |    | The name of an image file that contains the uv map that is used to remap the input.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- UV Remapping", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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
	std::string uvMapFileName = parser.get<std::string>("uvmap");
	std::string resultFileName = parser.get<std::string>("result");

	std::cout << "Input: " << inputFileName << std::endl <<
		"UV-Map: " << uvMapFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		std::endl;

	// Load the input sample and uv map.
	Sample sample, uvMap, result;
	_persistence.loadSample(inputFileName, sample);
	_persistence.loadSample(uvMapFileName, uvMap);

	if (uvMap.channels() != 2) {
		std::cout << "Warning: The uv map should contain only 2 channels. Additional channels are ignored." << std::endl;

		Sample remapped(2, uvMap.width(), uvMap.height());
		remapped.map({ 2, 0, 1, 1 }, uvMap);
		uvMap = remapped;
	}

	// Remap the input sample.
	sample.sample((cv::Mat)uvMap, result);

	// Store the sample.
	_persistence.saveSample(resultFileName, result);
}