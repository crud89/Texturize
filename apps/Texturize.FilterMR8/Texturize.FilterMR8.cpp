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
	"{ksize k kernel    | 7  | The size of the filter kernel window (must be odd).}"
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- MR8 Filter Bank", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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
	int ksize = parser.get<int>("ksize");

	std::cout << "Input: " << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Kernel: " << ksize << "*" << ksize << " Pixels" << std::endl << 
		std::endl;

	// Load the input sample.
	Sample sample;
	_persistence.loadSample(inputFileName, sample);

	// In case the sample is colored, convert it to greyscale.
	if (sample.channels() == 3 || sample.channels() == 4)
	{
		cv::Mat greyscale;
		cv::cvtColor((cv::Mat)sample, greyscale, cv::COLOR_RGB2GRAY);
		sample = Sample(greyscale);
	}
	else if (sample.channels() != 1)
	{
		std::cout << "The input sample has " << sample.channels() << " channels and cannot be converted to greyscale. Please convert the sample manually or provide an RGB color image instead." << std::endl;
		return EXIT_FAILURE;
	}

	// Create a filter bank instance.
	MaxResponseFilterBank filterBank(ksize);

	// Filter the sample.
	std::cout << "Executing... ";
	Sample result;
	auto start = std::chrono::high_resolution_clock::now();
	filterBank.apply(result, sample);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;


	for (int i(0); i < result.channels(); ++i)
	{
		cv::Mat filtered = result.getChannel(i);
		std::cout << filtered;
		cv::imshow("Filtered", filtered);
		cv::waitKey(0);
	}

	// Store the sample.
	_persistence.saveSample(resultFileName, result);
}