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
	"{inhomogeneity ih  | 0.9| The importance factor of the source and target inhomgeneneity on the result.}"
	"{anisotropy ai     | 0.5| The importance of local anisotropy, i.e. how much weight goes into homogeneity progression and how much into orientation differences.}"
	"{chaos c ji        | 1.0| A factor to scale the jitter amplitudes. Increasing this value will produce more random results.}"
	"{albedo            |    | The name of an image file. If provided, the synthesizer displays feedback after each operation. Only usefull for debugging purposes.}"
};

// Persistence providers.
DefaultPersistence _persistence;
std::optional<Sample> _albedo;

void displayProgress(const float& progress) {
	const int numChars = 100;
	const int currentPos = static_cast<int>(progress * static_cast<float>(numChars));

	std::cout << "[";

	for (int i(0); i < numChars; ++i)
		if (i <= currentPos) std::cout << "="; else std::cout << " ";

	std::cout << "] " << int(progress * 100.0) << " %\r";
	std::cout.flush();
}

void giveFeedback(const std::string& name, const cv::Mat& map) {

	Sample image(cv::Mat::zeros(map.size(), CV_32FC3));
	cv::Mat rgb = cv::Mat::zeros(map.size(), CV_32FC3);

	if (map.channels() == 2) {
		Sample s(map);
		s.extract({ 0, 2, 1, 1 }, image);	// Since CV uses BGR representation.
	} else {
		image = Sample(map);
	}

	cv::imshow(name, (cv::Mat)image);

	if (_albedo.has_value()) {
		cv::Mat ex = (cv::Mat)_albedo.value();
		rgb.forEach<cv::Vec3f>([&map, &ex](cv::Vec3f& val, const int* idx) -> void {
			cv::Vec2f coords = map.at<cv::Vec2f>(idx);
			cv::Point2i exc(static_cast<int>(coords[0] * ex.cols), static_cast<int>(coords[1] * ex.rows));
			val = ex.at<cv::Vec3f>(exc);
		});

		cv::imshow("RGB", rgb);
	}

	cv::waitKey(0);
}

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
	std::string albedoFileName = parser.get<std::string>("albedo");
	uint64_t seed = parser.get<uint64_t>("seed");
	float inhomogeneity = parser.get<float>("inhomogeneity");
	float jitterIntensity = parser.get<float>("chaos");

	std::cout << "Input: " << inputFileName << std::endl <<
		"Output: " << resultFileName << std::endl <<
		"Seed: " << seed << std::endl;

	// Non-stationary synthesis can only be done, if a source homogeneity map is provided.
	if (!sourceProgressionFileName.empty()) {
		std::cout << "Source progression channel: " << sourceProgressionFileName << std::endl;

		if (!targetProgressionFileName.empty())
			std::cout << "Target progression channel: " << targetProgressionFileName << std::endl;
		else
			std::cout << "Target progression channel: not provided" << std::endl;
	} else {
		std::cout << "Source progression channel: ignored" << std::endl <<
			"Target progression channel: ignored" << std::endl;

		// Homogenious synthesis does not need weights for stationarity maps.
		inhomogeneity = 0.f;
	}

	std::cout << "Inhomogeneity: " << inhomogeneity << std::endl << std::endl;

	// Declare all samples and variables required.
	Sample exemplar, result, srcProgression, trgProgression;
	int kernel, width, height;
	std::shared_ptr<ISearchIndex> index;
	std::unique_ptr<AppearanceSpace> descriptor;

	// Load the appearance space descriptor.
	AppearanceSpaceAsset asset;
	asset.read(inputFileName, descriptor);

	// Get the exemplar and define the synthesis result.
	descriptor->sample(exemplar);
	descriptor->getKernel(kernel);

	// Validate dimensions.
	height = exemplar.height();
	width = exemplar.width();

	//TEXTURIZE_ASSERT(height == width);

	// Load the input samples, if they are provided.
	srcProgression = Sample(cv::Mat::zeros(exemplar.size(), CV_32FC1));
	trgProgression = Sample(cv::Mat::zeros(exemplar.size(), CV_32FC1));

	std::cout << "Initializing search index... ";
	auto start = std::chrono::high_resolution_clock::now();
	if (!sourceProgressionFileName.empty()) {
		_persistence.loadSample(sourceProgressionFileName, srcProgression);
		srcProgression.weight(inhomogeneity);

		if (!targetProgressionFileName.empty()) {
			_persistence.loadSample(targetProgressionFileName, trgProgression);
			trgProgression.weight(inhomogeneity);
		} else {
			// TODO: In case no control map is provided, generate one based on hierarchical perlin noise.
			// For now, just print an error.
			std::cout << "Error: No progression control map has been provided." << std::endl;
			return EXIT_FAILURE;
		}

		TEXTURIZE_ASSERT(srcProgression.channels() == 1);
		TEXTURIZE_ASSERT(trgProgression.channels() == 1);

		// Build up the search index.
		//std::unique_ptr<IDescriptorExtractor> descriptorExtractor = std::make_unique<Tapkee::PCADescriptorExtractor>();
		//std::shared_ptr<ISearchIndex> index = std::make_shared<KNNIndex>(std::move(descriptor), std::move(descriptorExtractor));
		//index = std::make_shared<ANNIndex>(std::move(descriptor), srcProgression);
		//index = std::make_shared<KNNIndex>(std::move(descriptor), srcProgression);
		index = std::make_shared<CoherentIndex>(std::move(descriptor), srcProgression);
		//index = std::make_shared<RandomWalkIndex>(std::move(descriptor), srcProgression);
	} else {
		//index = std::make_shared<ANNIndex>(std::move(descriptor));
		//index = std::make_shared<KNNIndex>(std::move(descriptor));
		index = std::make_shared<CoherentIndex>(std::move(descriptor));
		//index = std::make_shared<RandomWalkIndex>(std::move(descriptor));
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << std::endl << "Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

#ifdef _DEBUG
	auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	//auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#else
	//auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#endif

	// Randomness Selector Function
	// TODO: This should go into the framework.
	int depth = log2(width);
	PyramidSynthesisSettings::RandomnessSelectorFunction randmonessSelector([&exemplar, &jitterIntensity](int level, const cv::Mat& uv) -> float {
		// Sample the exemplar.
		Sample currentSample;
		exemplar.sample(uv, currentSample);

		// Get a resized copy of the exemplar.
		cv::Mat downsampled;
		cv::resize((cv::Mat)exemplar, downsampled, uv.size());

		// Calculate variances from both: current sample and exemplar at current scale.
		cv::Mat meanSample, devSample, meanEx, devEx;
		cv::meanStdDev((cv::Mat)currentSample, meanSample, devSample);
		cv::meanStdDev(downsampled, meanEx, devEx);

		// Calculate average variances.
		cv::multiply(devSample, devSample, devSample);
		cv::multiply(devEx, devEx, devEx);
		float avgVarSample = cv::sum(devSample).val[0];
		float avgVarEx = cv::sum(devEx).val[0];

		// Set jitter amplitude to difference of variances.
		return jitterIntensity * std::abs(avgVarSample - avgVarEx);
	});

	PyramidSynthesisSettings config(1.f, cv::Point2f(0.f, 0.f), randmonessSelector, kernel, seed);

	// Toggle target guidance map.
	if (!sourceProgressionFileName.empty())
		config._guidanceMap = trgProgression;

	// Toggle feedback provider.
	if (!albedoFileName.empty()) {
		Sample albedo;
		_persistence.loadSample(albedoFileName, albedo);
		_albedo = albedo;
		config._feedbackHandler.add(giveFeedback);
	}

	// Setup progress handler.
	const int passesPerLevel = config._correctionPasses;
	
	config._progressHandler.add([depth, passesPerLevel](int level, int pass, const cv::Mat& uv) -> void {
		float progressPerCallback = (static_cast<float>(1.f) / static_cast<float>(depth)) / static_cast<float>(passesPerLevel);
		float currentProgress = static_cast<float>(level) / static_cast<float>(depth);

		if (pass != -1)
			currentProgress += static_cast<float>(pass + 1) * progressPerCallback;
			
		displayProgress(currentProgress);
	});

	// Perform the synthesis.
	std::cout << "Performing synthesis..." << std::endl;
	start = std::chrono::high_resolution_clock::now();
	synthesizer->synthesize(width, height, result, config);
	end = std::chrono::high_resolution_clock::now();
	std::cout << std::endl << "Done! (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms)" << std::endl;

	// Store the result uv map.
	_persistence.saveSample(resultFileName, result);
}