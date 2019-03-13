#include "stdafx.h"
#include "log2.h"

#include <analysis.hpp>

#include <opencv2/highgui.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Image pyramid base implementation 				                                      /////
///////////////////////////////////////////////////////////////////////////////////////////////////

Sample ImagePyramid::getLevel(const unsigned int level) const
{
	TEXTURIZE_ASSERT(level < _levels.size());                   // level must be a valid index
	
	return _levels[level];
}

void ImagePyramid::filterLevel(std::unique_ptr<IFilter>& filter, const unsigned int level)
{
    TEXTURIZE_ASSERT(level < _levels.size());                   // level must be a valid index
    
	Sample sample;
	//this->reconstruct(sample, level);
    filter->apply(sample, _levels[level]);
	_levels[level] = sample;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Gaussian image pyramid base implementation 		                                      /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void GaussianImagePyramid::construct(const Sample& sample, const unsigned int toLevel)
{
    auto level = log2(static_cast<uint32_t>(sample.width()));

    TEXTURIZE_ASSERT(level > 0);                                // There must be at least one level.
    TEXTURIZE_ASSERT(toLevel <= level);                         // Image must be large enough.
    //TEXTURIZE_ASSERT(sample.channels() == 1);

    // Each level is a successively downsampled and blurred copy of the previous sample, starting
    // with the finest level, that represents a copy of the original sample.
    cv::Mat s = (cv::Mat)sample;
    _levels.push_back(sample.clone());

    for (unsigned int l = 1; l < toLevel; ++l)
    {
        cv::Mat finerLevel = (cv::Mat)_levels[l - 1], downsampledLevel;
        cv::pyrDown(finerLevel, downsampledLevel);
        _levels.push_back(Sample(downsampledLevel));
    }

	// Store the levels in reverse order, so that the coarsest level is the first one in the array.
	// This allows for it to be easily skipped by letting loops begin at array index 1, which is
	// handy when building laplacian pyramids.
	std::reverse(_levels.begin(), _levels.end());
}

void GaussianImagePyramid::reconstruct(Sample& to, const unsigned int toLevel)
{
    TEXTURIZE_ASSERT(toLevel < _levels.size());                 // toLevel must be a valid level index.

    // Reconstruction is done by starting at the coarsest level, iterating upwards.
	cv::Mat reconstruction = (cv::Mat)_levels.front();

    for (unsigned int i = 0; i < toLevel; ++i)
        cv::pyrUp(reconstruction, reconstruction);

    to = Sample(reconstruction);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Gaussian image pyramid base implementation 		                                      /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void LaplacianImagePyramid::construct(const Sample& sample, const unsigned int toLevel)
{
    TEXTURIZE_ASSERT(toLevel > 1);                                // There must be at least two levels, in order upsample high frequencies.
	
    // Generate a gaussian pyramid first.
    GaussianImagePyramid::construct(sample, toLevel);
	std::vector<Sample> gaussianLevels = _levels;

    // The coarsest (index 0) level will be stored. The successive levels will be high-pass filtered. 
	// Start at the second-coarsest level and traverse all towards the finest.
    for (int lvl(1); lvl < toLevel; ++lvl) {
        cv::Mat coarserLevel = (cv::Mat)gaussianLevels[lvl - 1], upsampledLevel, currentLevel;

        // Take the next coarser level; upsample and blur it.
        cv::pyrUp(coarserLevel, upsampledLevel);

        // The difference between the current level and the upsampled one are the high frequencies 
        // at this scale, which would be lost by downsampling. During reconstruction they will be
        // added back again to the image.
        cv::subtract((cv::Mat)gaussianLevels[lvl], upsampledLevel, currentLevel);

        // Store the filter response.
        _levels[lvl] = Sample(currentLevel);
    }
}

void LaplacianImagePyramid::reconstruct(Sample& to, const unsigned int toLevel)
{
    TEXTURIZE_ASSERT(toLevel < _levels.size());                 // toLevel must be a valid level index.

    // Reconstruction is done by starting at the coarsest level, iterating upwards.
    cv::Mat reconstruction = (cv::Mat)_levels.front();
	auto depthType = reconstruction.depth();

	for (int lvl(1); lvl < toLevel; ++lvl)
	{
		// Upsample and blur.
		cv::pyrUp(reconstruction, reconstruction);

		// Add high frequencies back in.
		cv::add(reconstruction, (cv::Mat)_levels[lvl], reconstruction, cv::noArray(), depthType);
    }

    to = Sample(reconstruction);
}