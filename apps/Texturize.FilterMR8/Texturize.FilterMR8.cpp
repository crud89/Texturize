#include "stdafx.h"

#include <iostream>
#include <chrono>

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>
#include <Codecs/exr.hpp>

#include <opencv2/highgui.hpp>

#include "anigauss.h"

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
	//MaxResponseFilterBank filterBank(ksize);

	// Filter the sample.
	std::cout << "Executing... ";
	Sample result;
	auto start = std::chrono::high_resolution_clock::now();
	//filterBank.apply(result, sample);

	{
		// Compute mean of each row.
		cv::Mat image, output(sample.size(), CV_64FC1);
		((cv::Mat)sample).convertTo(image, CV_64FC1);
		cv::Scalar mean = cv::mean(image);
		cv::subtract(image, mean, image);
		cv::Mat sqrImg;
		cv::multiply(image, image, sqrImg);
		mean = cv::mean(sqrImg);
		mean[0] = sqrt(mean[0]);
		cv::divide(image, mean, image);

		// The result is a 8-channel image of the same size as the input.
		result = Sample(8, image.size());

		double sfac{ 1.0 }, mulfac{ 2.0 }, s1{ 3.0 /* * sfac*/ }, s2{ 1.0 /* * sfac*/ };
		int channel(0);

		for (int mode(0); mode < 2; ++mode) {
			cv::Mat r1 = cv::Mat::zeros(image.size(), CV_32FC1), r2 = cv::Mat::zeros(image.size(), CV_32FC1), buffer(image.size(), CV_32FC1);

			for (int scale(0); scale < 5; ++scale) {
				double phi = scale / 6.0 * 180.0;	// `anigauss` requires deg.
				
				anigauss(reinterpret_cast<double*>(image.data), reinterpret_cast<double*>(output.data), image.cols, image.rows, s1, s2, phi, 0, 1);
				output.convertTo(buffer, CV_32FC1);
				buffer = cv::abs(buffer);
				cv::max(r1, buffer, r1);

				anigauss(reinterpret_cast<double*>(image.data), reinterpret_cast<double*>(output.data), image.cols, image.rows, s1, s2, phi, 0, 2);
				output.convertTo(buffer, CV_32FC1);
				cv::max(r2, buffer, r2);
			}

			result.setChannel(channel++, r1);
			result.setChannel(channel++, r2);

			s1 *= mulfac;
			s2 *= mulfac;
		}

		cv::Mat gaussian = cv::Mat::zeros(image.size(), CV_32FC1), laplacian = cv::Mat::zeros(image.size(), CV_32FC1);
		double sigma = 10.0 * sfac;

		anigauss(reinterpret_cast<double*>(image.data), reinterpret_cast<double*>(output.data), image.cols, image.rows, sigma, sigma, 0.0, 2, 0);
		output.convertTo(gaussian, CV_32FC1);
		anigauss(reinterpret_cast<double*>(image.data), reinterpret_cast<double*>(output.data), image.cols, image.rows, sigma, sigma, 0.0, 0, 2);
		output.convertTo(laplacian, CV_32FC1);
		cv::add(gaussian, laplacian, gaussian);
		result.setChannel(channel++, gaussian);

		anigauss(reinterpret_cast<double*>(image.data), reinterpret_cast<double*>(output.data), image.cols, image.rows, sigma, sigma, 0.0, 0, 0);
		output.convertTo(gaussian, CV_32FC1);
		result.setChannel(channel++, gaussian);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Store the sample.
	_persistence.saveSample(resultFileName, result);
}