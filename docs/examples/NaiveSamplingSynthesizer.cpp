#include <sampling.hpp>

using namespace Texturize;

// ------------------------------------------------------------------------------------------------
// 
// The example demonstrates how to implement a custom synthesizer, that generates a new texture
// by naively sampling an exemplar.
// 
// ------------------------------------------------------------------------------------------------

class NaiveSamplingSynthesizer : public SynthesizerBase {
    // Constructor and factory method
protected:
    NaiveSamplingSynthesizer(const SearchIndex* catalog);
    
public:
    static std::unique_ptr<ISynthesizer> createSynthesizer(const SearchIndex* catalog);

    // Pixel sampling logic
private:
    cv::Point2i createSample(Sample& result, const cv::Size& size,  const cv::Point2f& coords, const int kernel, cv::Mat& mask) const;
    bool synthesizeNextPixel(Sample& sample, cv::Mat& mask, cv::Point2i& at, cv::Vec2i& offset) const;

    // ISynthesizer interface
public:
    void synthesize(int width, int height, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const override;
    void synthesize(const cv::Size& size, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const override;
};

class NaiveSamplerSettings : public SynthesisSettings {
public:
    typedef EventDispatcher<void, const Sample&> ProgressHandler;

public:
    // A callback, that can be used to inspect the currently synthesized sample.
    ProgressHandler _progressHandler;
};

// ------------------------------------------------------------------------------------------------
// 
// Construction and factory method
// 
// ------------------------------------------------------------------------------------------------

NaiveSamplingSynthesizer::NaiveSamplingSynthesizer(const SearchIndex* catalog) :
    SynthesizerBase(catalog) 
{
}

std::unique_ptr<ISynthesizer> NaiveSamplingSynthesizer::createSynthesizer(const SearchIndex* catalog) {
	return std::make_unique<NaiveSamplingSynthesizer>(catalog);
}

// ------------------------------------------------------------------------------------------------
// 
// Sampling
// 
// ------------------------------------------------------------------------------------------------

cv::Point2i NaiveSamplingSynthesizer::createSample(Sample& result, const cv::Size& size, const cv::Point2f& coords, const int kernel, cv::Mat& mask) const {
    // Create a new sample with two channels, one for u and one for v coordinates.
	cv::Mat uv(size, CV_32FC2);

    // Initialize the coordinates.
	uv.forEach<cv::Vec2f>([&size](cv::Vec2f& uv, const int* idx) -> void {
        // The u and v coordinates are offset by the seed.
		uv[0] = static_cast<float>(idx[1]) / static_cast<float>(size.width) + coords.x;
		uv[1] = static_cast<float>(idx[0]) / static_cast<float>(size.height) + coords.y;
	});
    
    // Create a binary mask, that masks out the kernel in the center of the sample.
    mask = cv::Mat::zeros(size, CV_8UC1);
    int centerX = size.width / 2;
    int centerY = size.height / 2;
    int kernelHalf = kernel / 2;

    for (int x(-kernelHalf); x <= kernelHalf; ++x)
    for (int y(-kernelHalf); y <= kernelHalf; ++y)
    {
        mask.at<unsigned char>(centerX + x, centerY + y) = UCHAR_MAX;
    }

    // Mask out non-seed coordinates.
    cv::Mat maskedUv;
    uv.copyTo(maskedUv, mask);

    // Store the result.
    result = Sample(maskedUv);

    // Return the point, where the synthesis should be started, i.e. the lower right point of the
    // window with an offset of 1 pixel to the right. For example:
    // o o o
    // o o o
    // o o o x
    return cv::Point2i(centerX + kernelHalf + 1, centerY + kernelHalf);
}

bool NaiveSamplingSynthesizer::synthesizeNextPixel(Sample& sample, cv::Mat& mask, cv::Point2i& at, cv::Vec2i& offset) const {
    // Create a rotation matrix, that rotates a vector by 90 degrees counter-clockwise.
    static const cv::Mat rotation = cv::getRotationMatrix2D(cv::Point2f(0, 0), 90, 1);

    // Calculate the runtime neighborhood descriptors for the current sample.
	const SearchIndex* index = this->getIndex();
	const Sample* exemplar;
	index->getSearchSpace()->sample(&exemplar);
    const cv::Mat descriptors = index->calculateNeighborhoodDescriptors(*exemplar, (cv::Mat)sample);

    // Find the nearest neighbor, that it at least 5% units away of the exemplar's width/height.
    cv::Vec2f match;
    index->findNearestNeighbor(descriptors, (cv::Mat)sample, at, match, 0.05);

    // Set the coordinates.
    sample.setTexel(at, std::vector<float>({ match.x, match.y }));

    // Mask the current pixel.
    mask.at<unsigned char>(at.x, at.y) = UCHAR_MAX;

    // Rotate the offset vector to see if we can go left.
    cv::Mat rotated = rotation * cv::Mat(offset);
    cv::Vec2i next(static_cast<int>(rotated.at<float>(0, 0)), static_cast<int>(rotated.at<float>(0, 1)));
    
    if (mask.at<unsigned char>(at.x + next.x, at.y + next.y) == 0) {
        // Continue with the left pixel.
        at = cv::Point2i(at.x + next.x, at.y + next.y);
        offset = next;
    } else if (mask.at<unsigned char>(at.x + offset.x, at.y + offset.y) == 0) {
        // Continue by moving forward.
        at = cv::Point2i(at.x + offset.x, at.y + offset.y);
    } else {
        // If neither of both cases is possible, the algorithm has finished.
        return false;
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
// 
// ISynthesizer interface
// 
// ------------------------------------------------------------------------------------------------

void NaiveSamplingSynthesizer::synthesize(int width, int height, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const {
    this->synthesize(cv::Size(width, height), result, config);
}

void NaiveSamplingSynthesizer::synthesize(const cv::Size& size, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const {
	// The configuration must contain arguments for naive sampling synthesis.
	const NaiveSamplerSettings* settings = dynamic_cast<const NaiveSamplerSettings*>(&config);

    if (settings == nullptr || !settings->validate())
        throw;      // Incompatible or invalid settings.

    // Create a new sample for the result uv map.
    Sample sample;
    cv::Mat mask;
    cv::Point2i start = this->createSample(sample, size, config._seedCoords, config._seedKernel, mask);
    cv::Vec2i next(0, 1);

    // The sample now contains an area in the center, that contains a copied window from the
    // exemplar. The algorithm starts with the center pixel right to this window and calculates
    // the nearest neighbor for it. It then checks, if it can "go left", which it is not permitted,
    // if the pixel there has already been synthesized. In that case, it moves forward. This 
    // process is repeated until there are no pixels left, i.e. it can only move forward, but by
    // doing so, it would get out of bounds.
    // Note that this algorithm only works on images that have equal dimensions.
    while (this->synthesizeNextPixel(sample, mask, start, next))
        settings->_progressHandler.execute(sample);
}
