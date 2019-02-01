#include "stdafx.h"

#include "gaussianpdf.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>
#include <chrono>

#include <opencv2\core\base.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\core\utility.hpp>
#include <opencv2\highgui.hpp>

#include <texturize.hpp>
#include <analysis.hpp>
#include <sampling.hpp>
#include <codecs.hpp>
#include <Codecs\exr.hpp>
#include <Adapters\tapkee.hpp>

using namespace Texturize;

// Command line parameter meta data.
// NOTE: It's important to use spaces instead of tabstops here!
const char* parameters =
{
	"{h help usage ?    |    | Displays this help message}"
	"{proc p            |    | Specifies the program(s) to run (fm: Detect Features, fd: Calculate feature distances, as: Appearance space transform, s: Perform synthesis, st: Style transfer). Different programs can be seperated using the \'|\' character and will be executed in the order they appear.}"
	"{exemplar ex       |    | Exemplar file name}"
	"{target t          |    | Style transfer target map}"
	"{featureMap fm     |    | Feature map file name}"
	"{distanceMap fd    |    | Feature distance file name}"
	"{descriptor ds     |    | Descriptor asset name}"
	"{model em          |    | Edge detector model name}"
	"{result r          |    | Result file name}"
	"{uv                |    | Result UV map name}"
	"{width rw          |0   | Result width}"
	"{height rh         |0   | Result height}"
	"{displayResult dr  |0   | Displays the result of the currently executed program, after it has finished.}"
	"{mat m             |0   | Flag: Exemplar and result are treated as material (1) or texture (0).}"
	"{weight w          |    | Specifies weights for individual exemplar material maps. Only applies to material maps.}"
	"{dontWait          |0   | Do not wait for the user before exiting the application.}"
	"{jitter j          |    | Semicolon-separated list of floats between 0 and 1, that contain per-level randomness.}"
	"{rnd               |0   | A randomness value that is used to initialize constant jitter over all levels.}"
	"{g gauss           |0   | Defines a randomness distribution, set to a pyramid level where a gaussian distribution has its peak. Overrides j and rnd settings.}"
	"{gs                |0   | Lets the synthesizer use a symmetrical normal jitter distribution function.}"
	"{s seed            |0   | The seed to initialize the RNG.}"
	"{d dim             |8   | Dimensionality of the search space.}"
};

// NOTE: In case the -m flag is specified the -ex and the -r syntax changes.
// TODO: Weights for feature distance.

// Persistence providers.
DefaultPersistence _persistence;
StorageFactory _storage;

std::string buildFileName(const std::string& baseFileName, const std::string& suffix)
{
	std::string outputFileName(baseFileName);
	outputFileName = outputFileName.substr(0, outputFileName.find_last_of('.'));
	outputFileName += "_" + suffix;
	outputFileName += baseFileName.substr(baseFileName.find_last_of('.'));

	return outputFileName;
}

int detectEdges(const std::string& exemplarName, const std::string& featureMapName, const std::string& edgeDetectorModel, const bool showResult = false)
{
	// Load the exemplar and check if it's RGB.
	Sample exemplar;
	_persistence.loadSample(exemplarName, exemplar);

	TEXTURIZE_ASSERT(exemplar.channels() == 3);

	// Initialize a new edge detector by loading a defined model.
	std::unique_ptr<EdgeDetector> detector = std::make_unique<StructuredEdgeDetector>(edgeDetectorModel);

	// Extract the edges.
	Sample result;
	detector->apply(result, exemplar);

	// Store the result.
	_persistence.saveSample(featureMapName, result);

	// Display the result if requested.
	if (showResult)
	{
		cv::imshow("Edge Response", (cv::Mat)result);
		//cv::waitKey(0);
		cv::waitKey(1);
	}

	return 0;
}

int featureDistance(const std::string& featureMapName, const std::string& distanceMapName, const bool showResult = false)
{
	// Load the exemplar and check if it's grayscale.
	Sample featureMap;
	_persistence.loadSample(featureMapName, featureMap);

	if (featureMap.channels() != 1)
	{
		std::unique_ptr<GrayscaleFilter> filter = std::make_unique<GrayscaleFilter>();
		filter->apply(featureMap, featureMap);
	}

	TEXTURIZE_ASSERT(featureMap.channels() == 1);

	// Initialize a new edge detector by loading a defined model.
	std::unique_ptr<FeatureExtractor> filter = std::make_unique<FeatureExtractor>();

	// Extract the edges.
	Sample result;
	filter->apply(result, featureMap);

	// Store the result.
	_persistence.saveSample(distanceMapName, result);

	// Display the result if requested.
	if (showResult)
	{
		cv::imshow("Feature Map", (cv::Mat)result);
		//cv::waitKey(0);
		cv::waitKey(1);
	}

	return 0;
}

int appearanceSpace(const std::unordered_map<std::string, std::string>& exemplarMaps, const std::unordered_map<std::string, float>& mapWeights, const std::string& distanceMapName, const std::string& descriptorAssetName, const size_t dimensionality = 8, bool showResult = false)
{
	// Load the provided exemplar samples.
	std::vector<Sample> samples;

	for each (auto map in exemplarMaps)
	{
		Sample sample;
		_persistence.loadSample(map.second, sample);

		// Weight the map, if a weight is provided.
		if (mapWeights.find(map.first) != mapWeights.end())
			sample.weight(mapWeights.at(map.first));

		samples.push_back(sample);
	}

	// If a distance map is provided, load it too.
	if (!distanceMapName.empty())
	{
		Sample distanceMap;
		_persistence.loadSample(distanceMapName, distanceMap);

		TEXTURIZE_ASSERT_DBG(distanceMap.channels() == 1);

		samples.push_back(distanceMap);
	}

	// The "exemplar" now contains all descriptive channels that are used to calculate the appearance space.
	AppearanceSpace* rawDescriptor;
	AppearanceSpace::calculate(std::initializer_list<const Sample>(samples.data(), samples.data() + samples.size()), &rawDescriptor, dimensionality);
	std::unique_ptr<AppearanceSpace> descriptor(rawDescriptor);
	
	// Save the asset.
	AppearanceSpaceAsset asset;
	asset.write(descriptorAssetName, descriptor.get(), _storage);

	// Display the result, if requested.
	if (showResult)
	{
		Sample sample;
		descriptor->sample(sample);

		if (sample.channels() > 2 && sample.channels() <= 4)
		{
			cv::imshow("Appearance Space", (cv::Mat)sample);
			//cv::waitKey(0);
			cv::waitKey(1);
		}
		else
		{
			std::cout << "Appearance space has " << sample.channels() << " channels and thus cannot be displayed." << std::endl;
			//std::cout << "Appearance space has " << sample.channels() << " channels and thus cannot be displayed completely. Only the first 4 channels are shown" << std::endl;
			// TODO: Implement me!
		}
	}

	return 0;
}

int synthesize(const std::unordered_map<std::string, std::string>& exemplarMaps, const std::unordered_map<std::string, std::string>& resultMaps, const std::string& uvMap, const std::string& descriptorAssetName, std::vector<float>& jitter, int width, int height, const uint64_t seed, const cv::Point2f seedCoords = cv::Point2f(-1, -1), const int seedKernel = 5, bool showResult = false)
{
	// Load the exemplar albedo map.
	Sample albedoMap;
	bool albedoProvided = false;

	if (albedoProvided = (exemplarMaps.find("albedo") != exemplarMaps.end()))
		_persistence.loadSample(exemplarMaps.at("albedo"), albedoMap);

	// Load the appearance space descriptor.
	std::unique_ptr<AppearanceSpace> descriptor;

	try
	{
		AppearanceSpaceAsset asset;
		AppearanceSpace* desc;
		asset.read(descriptorAssetName, &desc, _storage);
		descriptor = std::unique_ptr<AppearanceSpace>(desc);
	}
	catch (...)
	{
		return -1;
	}
		
	// Get the exemplar and define the synthesis result.
	Sample exemplar, result;
	int kernel;
	descriptor->sample(exemplar);
	descriptor->getKernel(kernel);

	// Build up the search index.
	//std::shared_ptr<ISearchIndex> index = std::make_shared<CoherentIndex>(std::move(descriptor));
	std::shared_ptr<ISearchIndex> index = std::make_shared<RandomWalkIndex>(std::move(descriptor));

#ifdef _DEBUG
	auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	//auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#else
	//auto synthesizer = PyramidSynthesizer::createSynthesizer(index);
	auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(index);
#endif

	// Randomness Selector Function
	int depth = log2(width);
	std::vector<float> randomness(jitter);

	for (size_t n = randomness.size(); n < depth; ++n)
		randomness.push_back(0.f);

	PyramidSynthesisSettings config(exemplar.width(), cv::Point2f(0.f, 0.f), randomness, kernel, seed);
	
	config._progressHandler.add([depth](int level, int pass, const cv::Mat& uv) -> void {
		if (pass == -1)
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass skipped)" << std::endl;
		else
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass " << pass + 1 << ")" << std::endl;
	});

	// TODO: This can be removed or altered to use the Sample::sample method for sampling (thanks, Captain Obvious!).
	if (showResult && albedoProvided) {
		config._feedbackHandler.add([&albedoMap](const std::string& windowName, const cv::Mat& img) -> void {
			Sample image(cv::Mat::zeros(img.size(), CV_32FC3));
			cv::Mat rgb = cv::Mat::zeros(img.size(), CV_32FC3);
			cv::Mat ex = (cv::Mat)albedoMap;

			if (img.channels() == 2)
			{
				Sample map(img);
				map.extract({ 0, 2, 1, 1 }, image);	// Since CV uses BGR representation.
			}
			else
			{
				image = Sample(img);
			}

			rgb.forEach<cv::Vec3f>([&img, &ex](cv::Vec3f& val, const int* idx) -> void {
				cv::Vec2f coords = img.at<cv::Vec2f>(idx);
				cv::Point2i exc(static_cast<int>(coords[0] * ex.cols), static_cast<int>(coords[1] * ex.rows));
				val = ex.at<cv::Vec3f>(exc);
			});

			cv::imshow(windowName, (cv::Mat)image);
			cv::imshow("RGB", rgb);
			cv::waitKey(1);
			//cv::waitKey(0);
		});
	}

	// Perform the synthesis.
	synthesizer->synthesize(width, height, result, config);
	cv::Mat uv = (cv::Mat)result;

	if (!uvMap.empty())
		_persistence.saveSample(uvMap, result, CV_16U);
	
	// Sample the result maps.
	for each (auto map in resultMaps)
	{
		if (exemplarMaps.find(map.first) == exemplarMaps.end()) {
			std::cout << "Warning: No exemplar map called \"" << map.first << "\" has been provided. Skipping..." << std::endl;
			continue;
		}

		Sample mapImage, resultSample;
		std::string mapName = exemplarMaps.at(map.first);

		_persistence.loadSample(mapName, mapImage);
		mapImage.sample(uv, resultSample);
		_persistence.saveSample(map.second, resultSample);
	}

	return 0;
}

int transferStyle(const std::unordered_map<std::string, std::string>& exemplarMaps, const std::unordered_map<std::string, std::string>& resultMaps, const std::string& uvMap, const std::string& descriptorAssetName, const std::unordered_map<std::string, std::string>& transferTargets, const uint64_t seed, bool showResult = false)
{
	// Load the exemplar albedo map.
	Sample albedoMap;
	bool albedoProvided = false;

	if (albedoProvided = (exemplarMaps.find("albedo") != exemplarMaps.end()))
		_persistence.loadSample(exemplarMaps.at("albedo"), albedoMap);

	// Load the transfer target.
	std::vector<Sample> samples;

	for (auto& targetName : transferTargets)
	{
		Sample target;
		_persistence.loadSample(targetName.second, target);
		samples.push_back(target);
	}

	Sample transferTarget = Sample::mergeSamples(std::initializer_list<const Sample>(samples.data(), samples.data() + samples.size()));

	// Load the appearance space descriptor.
	std::unique_ptr<AppearanceSpace> descriptor;

	try
	{
		AppearanceSpaceAsset asset;
		AppearanceSpace* desc;
		asset.read(descriptorAssetName, &desc, _storage);
		descriptor = std::unique_ptr<AppearanceSpace>(desc);
	}
	catch (...)
	{
		std::cout << "Error: Invalid descriptor asset." << std::endl;
		return -1;
	}

	// Get the exemplar and define the synthesis result.
	Sample exemplar, result;
	int kernel;
	descriptor->sample(exemplar);
	descriptor->getKernel(kernel);

	// Build up the search index.
	//std::shared_ptr<ISearchIndex> index = std::make_shared<CoherentIndex>(std::move(descriptor));
	//std::shared_ptr<ISearchIndex> index = std::make_shared<RandomWalkIndex>(std::move(descriptor));
	//std::shared_ptr<ISearchIndex> index = std::make_shared<ANNIndex>(std::move(descriptor));
	//std::shared_ptr<ISearchIndex> index = std::make_shared<KNNIndex>(std::move(descriptor));

	std::unique_ptr<IDescriptorExtractor> descriptorExtractor = std::make_unique<Tapkee::PCADescriptorExtractor>();
	std::shared_ptr<ISearchIndex> index = std::make_shared<KNNIndex>(std::move(descriptor), std::move(descriptorExtractor));

#ifdef _DEBUG
	auto synthesizer = PyramidSynthesizer::createSynthesizer(std::move(index));
	//auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(std::move(index));
#else
	//auto synthesizer = PyramidSynthesizer::createSynthesizer(std::move(index));
	auto synthesizer = ParallelPyramidSynthesizer::createSynthesizer(std::move(index));
#endif

	PyramidSynthesisSettings config(1.0f);
	config._seedKernel = kernel;
	config._rngState = seed;

	// TODO: This can be removed or altered to use the Sample::sample method for sampling (thanks, Captain Obvious!).
	if (showResult && albedoProvided) {
		config._feedbackHandler.add([&albedoMap](const std::string& windowName, const cv::Mat& img) -> void {
			Sample image(cv::Mat::zeros(img.size(), CV_32FC3));
			cv::Mat rgb = cv::Mat::zeros(img.size(), CV_32FC3);
			cv::Mat ex = (cv::Mat)albedoMap;

			if (img.channels() == 2)
			{
				Sample map(img);
				map.extract({ 0, 2, 1, 1 }, image);	// Since CV uses BGR representation.
			}
			else
			{
				image = Sample(img);
			}

			rgb.forEach<cv::Vec3f>([&img, &ex](cv::Vec3f& val, const int* idx) -> void {
				cv::Vec2f coords = img.at<cv::Vec2f>(idx);
				cv::Point2i exc(static_cast<int>(coords[0] * ex.cols), static_cast<int>(coords[1] * ex.rows));
				val = ex.at<cv::Vec3f>(exc);
			});

			cv::imshow(windowName, (cv::Mat)image);
			cv::imshow("RGB", rgb);
			cv::waitKey(1);
			//cv::waitKey(0);
		});
	}

	// Perform the synthesis.
	synthesizer->transferStyle(transferTarget, result, config);
	cv::Mat uv = (cv::Mat)result;

	if (!uvMap.empty())
		_persistence.saveSample(uvMap, result, CV_16U);

	// Sample the result maps.
	for each (auto map in resultMaps)
	{
		if (map.second.empty())
			continue;

		if (exemplarMaps.find(map.first) == exemplarMaps.end()) {
			std::cout << "Warning: No exemplar map called \"" << map.first << "\" has been provided. Skipping..." << std::endl;
			continue;
		}

		Sample mapImage, resultSample;
		std::string mapName = exemplarMaps.at(map.first);

		_persistence.loadSample(mapName, mapImage);
		mapImage.sample(uv, resultSample);
		_persistence.saveSample(map.second, resultSample);
	}

	return 0;
}

void parsePrograms(const std::string& cmd, std::queue<std::string>& programs)
{
	// Get the selected programs.
	std::istringstream tokens(cmd);
	std::string token;

	while (std::getline(tokens, token, '|')) {
		// In case there is no token, skip.
		if (token.empty())
			continue;
		
		// Convert the token to lowercase.
		std::transform(token.begin(), token.end(), token.begin(), std::tolower);
		programs.push(token);
	}
}

void parseMaps(const std::string& cmd, std::unordered_map<std::string, std::string>& maps)
{
	// Get the individual map definitions.
	std::istringstream tokens(cmd);
	std::string token;

	while (std::getline(tokens, token, ';'))
	{
		if (token.empty())
			continue;
		
		size_t pos = token.find_first_of(":", 0);
		std::string name = token.substr(0, pos);
		std::string path = token.substr(pos + 1);
		std::transform(name.begin(), name.end(), name.begin(), std::tolower);

		maps[name] = path;
	}
}

void parseWeights(const std::string& cmd, std::unordered_map<std::string, float>& weights)
{
	// Get the map weights.
	std::istringstream tokens(cmd);
	std::string token;

	while (std::getline(tokens, token, ';'))
	{
		if (token.empty())
			continue;

		size_t pos = token.find_first_of(":", 0);
		std::string name = token.substr(0, pos);
		float weight = std::stof(token.substr(pos + 1));
		std::transform(name.begin(), name.end(), name.begin(), std::tolower);

		weights[name] = weight;
	}
}

void createContinuousUVMap(const cv::Size& size, const std::string& saveTo)
{
	cv::Mat uv = cv::Mat(size, CV_32FC3);

	for (int x(0); x < size.width; ++x)
		for (int y(0); y < size.height; ++y)
			uv.at<cv::Vec3f>(cv::Point2i(x, y)) = cv::Vec3f(0, static_cast<float>(y) / static_cast<float>(size.height), static_cast<float>(x) / static_cast<float>(size.width));

	uv.convertTo(uv, CV_8UC3, 255.f);
	cv::imwrite(saveTo, uv);
}

void parseJitter(const std::string& cmd, std::vector<float>& jitter)
{
	std::istringstream tokens(cmd);
	std::string token;

	while (std::getline(tokens, token, ';'))
	{
		if (token.empty())
			continue;

		jitter.push_back(std::stof(token));
	}
}

// CMD Presets:
// -proc=fm -ex="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_albedo.tiff" -fm="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fm.txr" -em="$(SolutionDir)\models\forest\modelFinal.yml"
// -proc=fm|fd -ex="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_albedo.tiff" -fm="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fm.txr" -fd="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fd.txr" -em="$(SolutionDir)\models\forest\modelFinal.yml"
// -proc=fm|fd|as -ex="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_albedo.tiff" -fm="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fm.txr" -fd="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fd.txr" -ds="$(SolutionDir)samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml"
//
// -proc=s -ex="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_albedo.tiff" -fd="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_fd.txr" -ds="$(SolutionDir)samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_1024_am.txa" -r="$(SolutionDir)\samples\Mixed Medieval Brick\TexturesCom_MixedMedievalBrick_2048s.png" -rw=2048 -rh=2048 -dr=1
//
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Icon\icon.jpg" -fm="$(SolutionDir)\samples\Icon\Icon_fm.txr" -fd="$(SolutionDir)\samples\Icon\Icon_fd.txr" -ds="$(SolutionDir)samples\Icon\Icon_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Icon\Icon_512.png" -rw=512 -rh=512 -dr=1
// -proc=s -ex="$(SolutionDir)\samples\Icon\icon.jpg" -fd="$(SolutionDir)\samples\Icon\Icon_fd.txr" -ds="$(SolutionDir)samples\Icon\Icon_am.txa" -r="$(SolutionDir)\samples\Icon\Icon_512.png" -rw=512 -rh=512 -dr=1
//
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Hooks\Hooks.png" -fm="$(SolutionDir)\samples\Hooks\Hooks_fm.txr" -fd="$(SolutionDir)\samples\Hooks\Hooks_fd.txr" -ds="$(SolutionDir)samples\Hooks\Hooks_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Hooks\Hooks_512.png" -rw=512 -rh=512 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Icon2\Icon2.png" -fm="$(SolutionDir)\samples\Icon2\Icon2_fm.txr" -fd="$(SolutionDir)\samples\Icon2\Icon2_fd.txr" -ds="$(SolutionDir)samples\Icon2\Icon2_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Icon2\Icon2_512.png" -rw=512 -rh=512 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Skin\Skin.jpg" -fm="$(SolutionDir)\samples\Skin\Skin_fm.txr" -fd="$(SolutionDir)\samples\Skin\Skin_fd.txr" -ds="$(SolutionDir)samples\Skin\Skin_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Skin\Skin_512.png" -rw=512 -rh=512 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Skin\Skin.jpg" -fm="$(SolutionDir)\samples\Skin\Skin_fm.txr" -fd="$(SolutionDir)\samples\Skin\Skin_fd.txr" -ds="$(SolutionDir)samples\Skin\Skin_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Skin\Skin_8192.png" -rw=8192 -rh=8192 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Whitestones\Whitestones.png" -fm="$(SolutionDir)\samples\Whitestones\Whitestones_fm.txr" -fd="$(SolutionDir)\samples\Whitestones\Whitestones_fd.txr" -ds="$(SolutionDir)samples\Whitestones\Whitestones_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Whitestones\Whitestones_2048.png" -rw=2048 -rh=2048 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Grass\Grass.png" -fm="$(SolutionDir)\samples\Grass\Grass_fm.txr" -fd="$(SolutionDir)\samples\Grass\Grass_fd.txr" -ds="$(SolutionDir)samples\Grass\Grass_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Grass\Grass_512.png" -rw=512 -rh=512 -dr=1
//
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Slate Floor\slate_albedo.tif" -fm="$(SolutionDir)\samples\Slate Floor\slate_fm.txr" -fd="$(SolutionDir)\samples\Slate Floor\slate_fd.txr" -ds="$(SolutionDir)samples\Slate Floor\Slate_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Slate Floor\Slate_albedo_512.png" -rw=512 -rh=512 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Rough Medival Wood\wood_albedo.png" -fm="$(SolutionDir)\samples\Rough Medival Wood\wood_fm.txr" -fd="$(SolutionDir)\samples\Rough Medival Wood\wood_fd.txr" -ds="$(SolutionDir)samples\Rough Medival Wood\wood_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Rough Medival Wood\wood_albedo_512.png" -rw=512 -rh=512 -dr=1
// -proc=fm|fd|as|s -ex="$(SolutionDir)\samples\Cliff\cliff_albedo.tif" -fm="$(SolutionDir)\samples\Cliff\cliff_fm.txr" -fd="$(SolutionDir)\samples\Cliff\cliff_fd.txr" -ds="$(SolutionDir)samples\Cliff\cliff_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="$(SolutionDir)\samples\Cliff\cliff_albedo_512.png" -rw=512 -rh=512 -dr=1
//
// PBR Material Presets:
// -proc=fm|fd|as|s -ex="albedo:$(SolutionDir)\samples\Cliff\cliff_albedo.tif;normal:$(SolutionDir)\samples\Cliff\cliff_normal.tif;height:$(SolutionDir)\samples\Cliff\cliff_height.tif;roughness:$(SolutionDir)\samples\Cliff\cliff_roughness.tif;ao:$(SolutionDir)\samples\Cliff\cliff_ao.tif" -fm="$(SolutionDir)\samples\Cliff\cliff_fm.txr" -fd="$(SolutionDir)\samples\Cliff\cliff_fd.txr" -ds="$(SolutionDir)samples\Cliff\cliff_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -r="albedo:$(SolutionDir)\samples\Cliff\cliff_albedo_512.png;normal:$(SolutionDir)\samples\Cliff\cliff_normal_512.png;height:$(SolutionDir)\samples\Cliff\cliff_height_512.png;roughness:$(SolutionDir)\samples\Cliff\cliff_roughness_512.png;ao:$(SolutionDir)\samples\Cliff\cliff_ao_512.png" -rw=512 -rh=512 -dr=1 -m=1 -w="normal:2;albedo:2"
// -proc=fm|fd|as|s -ex="albedo:$(SolutionDir)\samples\Cliff\cliff_albedo.tif;normal:$(SolutionDir)\samples\Cliff\cliff_normal.tif;height:$(SolutionDir)\samples\Cliff\cliff_height.tif;roughness:$(SolutionDir)\samples\Cliff\cliff_roughness.tif;ao:$(SolutionDir)\samples\Cliff\cliff_ao.tif" -fm="$(SolutionDir)\samples\Cliff\cliff_fm.txr" -fd="$(SolutionDir)\samples\Cliff\cliff_fd.txr" -ds="$(SolutionDir)samples\Cliff\cliff_am.txa" -em="$(SolutionDir)\models\forest\modelFinal.yml" -uv="$(SolutionDir)\samples\Cliff\cliff_uv_512_01.png" -rw=512 -rh=512 -dr=1 -m=1 -w="normal:3;albedo:2"
//
// Style Transfer Tests:
// -dontWait=1 -proc="as" -ex="height:C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\height.tif" -ds="C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\searchSpace.txa" -m=1 -fd="C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\guidance.txr"
// -dontWait=1 -proc="st" -ds="C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\searchSpace.txa" -t="C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\target_height.tif" -r="C:\Users\Admin\Documents\Visual Studio 2017\Projects\Texturize\tests\height_transfer\transferred.tif"
int main(int argc, const char** argv)
{
	// Parse the command line.
	cv::CommandLineParser parser(argc, argv, parameters);
	parser.about(cv::format("Texturize Sandbox %d.%d", TEXTURIZE_VER_MAJOR, TEXTURIZE_VER_MINOR));

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

	// Setup EXR codec.
	EXRCodec codec;
	_persistence.registerCodec("exr", &codec);
	_persistence.registerCodec("hdr", &codec);
	_persistence.registerCodec("txr", &codec);

	// Check the program and call it accordingly.
	std::string proc = parser.get<std::string>("proc");
	std::string exemplar = parser.get<std::string>("ex");
	std::string transferTarget = parser.get<std::string>("t");
	std::string featureMap = parser.get<std::string>("fm");
	std::string distanceMap = parser.get<std::string>("fd");
	std::string descriptorAsset = parser.get<std::string>("ds");
	std::string model = parser.get<std::string>("em");
	std::string resultFile = parser.get<std::string>("r");
	std::string weights = parser.get<std::string>("w");
	std::string uvMap = parser.get<std::string>("uv");
	std::string jitters = parser.get<std::string>("j");
	uint64_t seed = parser.get<uint64_t>("s");
	bool isMaterial = parser.get<int>("m");
	bool dontWait = parser.get<int>("dontWait");
	bool symGauss = parser.get<int>("gs");
	int showResult = parser.get<int>("dr");
	int width = parser.get<int>("rw");
	int height = parser.get<int>("rh");
	int gaussian = parser.get<int>("g");
	int dimensionality = parser.get<int>("d");
	float randomness = std::stof(parser.get<std::string>("rnd"));

	// Remove surrounding quote occurencies.
	exemplar			= !exemplar.empty() ? exemplar.substr(exemplar.find_first_not_of('\"'), exemplar.find_last_not_of('\"') + 1) : exemplar;
	transferTarget		= !transferTarget.empty() ? transferTarget.substr(transferTarget.find_first_not_of('\"'), transferTarget.find_last_not_of('\"') + 1) : transferTarget;
	featureMap			= !featureMap.empty() ? featureMap.substr(featureMap.find_first_not_of('\"'), featureMap.find_last_not_of('\"') + 1) : featureMap;
	distanceMap			= !distanceMap.empty() ? distanceMap.substr(distanceMap.find_first_not_of('\"'), distanceMap.find_last_not_of('\"') + 1) : distanceMap;
	descriptorAsset     = !descriptorAsset.empty() ? descriptorAsset.substr(descriptorAsset.find_first_not_of('\"'), descriptorAsset.find_last_not_of('\"') + 1) : descriptorAsset;
	model				= !model.empty() ? model.substr(model.find_first_not_of('\"'), model.find_last_not_of('\"') + 1) : model;
	resultFile			= !resultFile.empty() ? resultFile.substr(resultFile.find_first_not_of('\"'), resultFile.find_last_not_of('\"') + 1) : resultFile;
	uvMap				= !uvMap.empty() ? uvMap.substr(uvMap.find_first_not_of('\"'), uvMap.find_last_not_of('\"') + 1) : uvMap;
	weights				= !weights.empty() ? weights.substr(weights.find_first_not_of('\"'), weights.find_last_not_of('\"') + 1) : weights;

	// Get the programs in the order they should be executed.
	std::queue<std::string> programs;
	parsePrograms(proc, programs);

	// Get the maps, if the material flag is specified.
	std::unordered_map<std::string, std::string> exemplarMaps, resultMaps, transferMaps;

	if (isMaterial != 0)
	{
		dimensionality = dimensionality == 0 ? 8 : dimensionality;
		parseMaps(exemplar, exemplarMaps);
		parseMaps(resultFile, resultMaps);
		parseMaps(transferTarget, transferMaps);
	}
	else if (!exemplar.empty())
	{
		dimensionality = dimensionality == 0 ? 4 : dimensionality;
		exemplarMaps["albedo"] = exemplar;
		resultMaps["albedo"] = resultFile;
		transferMaps["albedo"] = transferTarget;
	}

	// Get the weights.
	std::unordered_map<std::string, float> mapWeights;
	parseWeights(weights, mapWeights);

	// Execute the programs, if provided.
	int result = EXIT_FAILURE;
	std::string program;
	
	while (!programs.empty())
	{
		program = programs.front();
		programs.pop();

		if (program == "fm") {
			std::cout << "Running edge detector..." << std::endl;

			if (exemplarMaps.find("albedo") == exemplarMaps.end()) {
				std::cout << "\tError: No albedo map has been specified." << std::endl;
				result = EXIT_FAILURE;
			}
			else {
				std::cout << "\tExemplar: " << exemplarMaps["albedo"] << std::endl <<
					"\tFeature Map: " << featureMap << std::endl;

				auto start = std::chrono::high_resolution_clock::now();
				result = detectEdges(exemplarMaps["albedo"], featureMap, model, showResult);
				auto end = std::chrono::high_resolution_clock::now();

				std::cout << "\tfm:Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
			}
		}
		else if (program == "fd") {
			std::cout << "Calculating feature distance map..." << std::endl <<
				"\tFeature Map: " << featureMap << std::endl <<
				"\tDistance Map: " << distanceMap << std::endl;

			auto start = std::chrono::high_resolution_clock::now();
			result = featureDistance(featureMap, distanceMap, showResult);
			auto end = std::chrono::high_resolution_clock::now();

			std::cout << "\tfd:Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		}
		else if (program == "as") {
			std::cout << "Calculating appearance map..." << std::endl;

			for each (auto map in exemplarMaps)
				std::cout << "\t" << map.first << ": " << map.second << std::endl;
				
			std::cout << "\tDistance Map: " << distanceMap << std::endl <<
				"\tDescriptor Asset: " << descriptorAsset << std::endl;


			auto start = std::chrono::high_resolution_clock::now();
			result = appearanceSpace(exemplarMaps, mapWeights, distanceMap, descriptorAsset, static_cast<size_t>(dimensionality), showResult);
			auto end = std::chrono::high_resolution_clock::now();

			std::cout << "\tas:Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		}
		else if (program == "s") {
			std::cout << "Performing synthesis stage..." << std::endl
				<< "\tDescriptor Asset: " << descriptorAsset << std::endl;

			for each (auto map in exemplarMaps)
				std::cout << "\tExemplar " << map.first << ": " << map.second << std::endl;
			
			for each (auto map in resultMaps)
				std::cout << "\tResult " << map.first << ": " << map.second << " (" << width << "x" << height << "px)" << std::endl;

			if (width < 0 || height < 0) {
				std::cout << "\tError: Invalid result width or height. Dimensions must be positive." << std::endl;
				result = EXIT_FAILURE;
			} else {			
				// Parse the jitter.
				std::vector<float> jitter;

				if (gaussian != 0) {
					int depth = log2(width);

					for (size_t j(0); j < depth; ++j)
						jitter.push_back(symGauss ?
							normalDistSym<float>(static_cast<float>(j), static_cast<float>(depth), static_cast<float>(gaussian)) :
							normalDistAssym<float>(static_cast<float>(j), static_cast<float>(depth), static_cast<float>(gaussian)));
				} else {
					parseJitter(jitters, jitter);

					// Fill the rest of the jitter from the provided randomness.
					int depth = log2(width);

					for (size_t j = jitter.size(); j < depth; ++j)
						jitter.push_back(randomness);
				}

				auto start = std::chrono::high_resolution_clock::now();
				result = synthesize(exemplarMaps, resultMaps, uvMap, descriptorAsset, jitter, width, height, seed, cv::Point2f(-1, -1), 5, showResult);
				auto end = std::chrono::high_resolution_clock::now();

				std::cout << "\ts:Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
			}
		}
		else if (program == "st") {
			std::cout << "Performing style transfer..." << std::endl
				<< "\tDescriptor Asset: " << descriptorAsset << std::endl;

			for each (auto map in transferMaps)
				std::cout << "\tTarget Map " << map.first << ": " << map.second << std::endl;

			std::cout << "\tResult: " << uvMap << std::endl;
			
			auto start = std::chrono::high_resolution_clock::now();
			result = transferStyle(exemplarMaps, resultMaps, uvMap, descriptorAsset, transferMaps, seed, showResult);
			auto end = std::chrono::high_resolution_clock::now();

			std::cout << "\ts:Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		}

		if (result != 0)
			break;
	}

	// Finished.
	if (!dontWait) {
		std::cout << "Press any key to exit the application!" << std::endl;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

    return result;
}