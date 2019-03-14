#include "stdafx.h"

#include <iostream>
#include <chrono>

#include <texturize.hpp>
#include <analysis.hpp>
#include <sampling.hpp>
#include <codecs.hpp>
#include <Codecs/exr.hpp>

#include <opencv2/highgui.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message.}"
	"{input in          |    | The name of an appearance space asset.}"
	"{result r          |    | The name of the image file, the result uv map stored to.}"
	"{seed s            | 0  | The seed to initialize random number generators with.}"
	"{srcprog           |    | The name of a greyscale image, containing the source texture homogeneity progression.}"
	"{trgprog           |    | The name of a greyscale image, containing the control homogeneity progression for the target texture.}"
	"{homogeneity h     | 0.9| The importance factor of the source and target homgeneneity on the result.}"
	/*"{anisotropy ai     | 0.5| The importance of local anisotropy, i.e. how much weight goes into homogeneity progression and how much into orientation differences.}"*/
};

// Persistence providers.
DefaultPersistence _persistence;

int main(int argc, const char** argv) {
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize %d.%d -- Synthesizer", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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
	std::string sourceProgressionFileName = parser.get<std::string>("srcprog");
	std::string targetProgressionFileName = parser.get<std::string>("trgprog");
	uint64_t seed = parser.get<uint64_t>("seed");
	float homogeneity = parser.get<float>("homogeneity");

	std::cout << "Input: " << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Seed: " << seed << std::endl;

	// Non-stationary synthesis can only be done, if a source homogeneity map is provided.
	if (sourceProgressionFileName.empty()) {
		std::cout << "Source progression channel: " << sourceProgressionFileName << std::endl;

		if (!targetProgressionFileName.empty())
			std::cout << "Target progression channel: " << targetProgressionFileName << std::endl;
		else
			std::cout << "Target progression channel: not provided" << std::endl;
	} else {
		std::cout << "Source progression channel: ignored" << std::endl <<
			"Target progression channel: ignored" << std::endl;

		// Homogenious synthesis does not need weights for stationarity maps.
		homogeneity = 0.f;
	}

	std::cout << "Homogeneity: " << homogeneity << std::endl << std::endl;

	// Declare all samples and variables required.
	Sample sample, exemplar, result, srcProgression, trgProgression;
	int kernel, width, height;

	// Load the input samples, if they are provided.
	_persistence.loadSample(inputFileName, sample);
	srcProgression = Sample(cv::Mat::zeros(sample.size(), CV_32FC1));
	trgProgression = Sample(cv::Mat::zeros(sample.size(), CV_32FC1));

	if (!sourceProgressionFileName.empty()) {
		_persistence.loadSample(sourceProgressionFileName, srcProgression);

		if (!targetProgressionFileName.empty())
			_persistence.loadSample(targetProgressionFileName, trgProgression);


		// TODO: Weight the progression maps, based on the homogeneity.
		// TODO: Pass target progression map during synthesis.
		// TODO: Automatic jitter amplitude, based on variances.
	}

	TEXTURIZE_ASSERT(srcProgression.channels() == 1);
	TEXTURIZE_ASSERT(trgProgression.channels() == 1);

	// Load the appearance space descriptor.
	std::unique_ptr<AppearanceSpace> descriptor;
	AppearanceSpaceAsset asset;
	asset.read(inputFileName, descriptor);
		
	// Get the exemplar and define the synthesis result.
	descriptor->sample(exemplar);
	descriptor->getKernel(kernel);

	// Validate dimensions.
	height = exemplar.height();
	width = exemplar.width();
	
	//TEXTURIZE_ASSERT(height == width);

	// Build up the search index.
	//std::unique_ptr<IDescriptorExtractor> descriptorExtractor = std::make_unique<Tapkee::PCADescriptorExtractor>();
	//std::shared_ptr<ISearchIndex> index = std::make_shared<KNNIndex>(std::move(descriptor), std::move(descriptorExtractor));
	std::shared_ptr<ISearchIndex> index = std::make_shared<KNNIndex>(std::move(descriptor));

#ifdef _DEBUG
	auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	//auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#else
	//auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#endif

	// Randomness Selector Function
	int depth = log2(width);
	std::vector<float> randomness(0);

	for (size_t n = randomness.size(); n < depth; ++n)
		randomness.push_back(0.f);

	PyramidSynthesisSettings config(exemplar.width(), cv::Point2f(0.f, 0.f), randomness, kernel, seed);
	
	config._progressHandler.add([depth](int level, int pass, const cv::Mat& uv) -> void {
		if (pass == -1)
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass skipped)" << std::endl;
		else
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass " << pass + 1 << ")" << std::endl;
	});

	// Perform the synthesis.
	std::cout << "Performing synthesis...";
	auto start = std::chrono::high_resolution_clock::now();
	synthesizer->synthesize(width, height, result, config);
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << " Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Store the result uv map.
	_persistence.saveSample(resultFileName, result);
}