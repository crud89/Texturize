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
	"{input in          |    | The name of an image file or a list of image files (seperated by \";\") that should be transformed into appearance space.}"
	"{result r          |    | The name of the image file, the result is stored to.}"
	"{dimensionality td | 8  | Resulting dimensionality of the result sample.}"
};

// Persistence providers.
DefaultPersistence _persistence;

template <typename T = float>
void calculateRetainedVariance(const cv::Mat& eigenValues, std::vector<T>& variancePerDim) {
	TEXTURIZE_ASSERT(eigenValues.type() == cv::DataType<T>::type);
	TEXTURIZE_ASSERT_DBG(variancePerDim.empty());
	
	// Create a matrix, containing the sum of eigen values.
	cv::Mat g = cv::Mat::zeros(eigenValues.size(), eigenValues.type());

	for (int i(0); i < g.rows; ++i)
	for (int j(0); j <= i; ++j)
		g.at<T>(i, 0) += eigenValues.at<T>(j, 0);

	// For each eigen value, compute the retained variance.
	variancePerDim.resize(eigenValues.rows);

	for (int v(0); v < eigenValues.rows; ++v)
		variancePerDim[v] = g.at<T>(v, 0) / g.at<T>(g.rows - 1, 0);
}

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
	std::string inputFileNames = parser.get<std::string>("input");
	std::string resultFileName = parser.get<std::string>("result");
	size_t dimensionality = parser.get<size_t>("dimensionality");

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

	std::cout << "Output: " << resultFileName << std::endl << std::endl;

	// Load all input samples into one sample.
	std::vector<Sample> inputSamples;

	for each (auto& fileName in inputFiles)
	{
		Sample sample;
		_persistence.loadSample(fileName, sample);
		inputSamples.push_back(sample);
	}

	Sample material = Sample::mergeSamples(std::initializer_list<const Sample>(inputSamples.data(), inputSamples.data() + inputSamples.size()));

	// The "exemplar" now contains all descriptive channels that are used to calculate the appearance space.
	std::unique_ptr<AppearanceSpace> dscr;
	std::cout << "Computing appearance space descriptors...";
	auto start = std::chrono::high_resolution_clock::now();
	AppearanceSpace::calculate(material, dscr, dimensionality);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Save the asset.
	AppearanceSpaceAsset asset;
	std::shared_ptr<AppearanceSpace> descriptor{ std::move(dscr) };
	asset.write(resultFileName, descriptor);

	// Print some statistics.
	// NOTE: Only works on CV_32F currently.
	std::shared_ptr<const cv::PCA> projector;
	descriptor->getProjector(projector);

	std::vector<float> variances;
	::calculateRetainedVariance(projector->eigenvalues, variances);

	std::cout << "Retained variances in " << variances.size() << " dimensions:" << std::endl;

	for (std::vector<float>::size_type s(0); s < variances.size(); ++s)
		std::cout << s << ": " << variances[s] << std::endl;
}