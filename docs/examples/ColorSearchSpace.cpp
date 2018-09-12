#include <analysis.hpp>

using namespace Texturize;

// ------------------------------------------------------------------------------------------------
// 
// The example demonstrates how to implement a custom search space, that matches pixels
// based on color values.
// 
// ------------------------------------------------------------------------------------------------

class ColorSearchSpace : public ISearchSpace {
private:
    // A pointer to the search-space transformed exemplar.
    const std::unique_ptr<Sample> _exemplar;

    // The kernel size of the neighborhood window.
    const int _kernelSize;

    // Constructor
public:
    ColorSearchSpace(Sample* exemplar, const int kernelSize) :
        _exemplar(exemplar), _kernelSize(kernelSize) {
    }

    // Factory method
public:
    static void calculate(const Sample& exemplar, ColorSearchSpace** desc, int kernelSize = 5);

    // ISearchSpace interface
public:
    void transform(const std::vector<float>& pixelNeighborhood, std::vector<float>& desc) const override;
    void transform(const Sample& sample, const int x, const int y, std::vector<float>& desc) const override;
    void transform(const Sample& sample, const cv::Point& texelCoords, std::vector<float>& desc) const override;
    void sample(Sample& sample) const override;
    void sample(const Sample** const sample) const override;
    void kernel(int& kernel) const override;
    void sampleSize(cv::Size& size) const override;
    void sampleSize(int& width, int& height) const override;
};

// ------------------------------------------------------------------------------------------------
// 
// Construction and factory method
// 
// ------------------------------------------------------------------------------------------------
//
// NOTE: It is recommended to use a factory method to create search space instances. The factory
// should calculate the projection, transform the exemplar and initialize the search space instances
// using the transformed exemplar. Since this example does not need to transform the exemplar,
// the search space can be initialized directly using a copy of the exemplar.
// 
// The constructor should only be called to restore a search space instance from an persistent 
// source, for example from an asset persistence.

void ColorSearchSpace::calculate(const Sample& exemplar, ColorSearchSpace** desc, int kernelSize = 5) {
	*desc = new AppearanceSpace(exemplar.clone(), kernelSize);
}

// ------------------------------------------------------------------------------------------------
// 
// Search space transformations
// 
// ------------------------------------------------------------------------------------------------
//
// NOTE: Advanced search spaces project neighborhoods into descriptors, however, this naive example
// matches neighborhoods based on raw color values, thus no transformation is required.

void ColorSearchSpace::transform(const std::vector<float>& pixelNeighborhood, std::vector<float>& desc) const {
    desc = std::vector<float>(pixelNeighborhood);
}

void ColorSearchSpace::transform(const Sample& sample, const int x, const int y, std::vector<float>& desc) const {
	// Calculate the number of components.
	int components = _kernelSize * _kernelSize * sample.channels();

	// Get the weighted pixel neighborhood from the sample.
	std::vector<float> kernel(components);
	sample.getNeighborhood(x, y, _kernelSize, kernel, true);

    // Return the transformed neighborhood.
	this->transform(kernel, desc);
}

void ColorSearchSpace::transform(const Sample& sample, const cv::Point& texelCoords, std::vector<float>& desc) const {
	// Calculate the number of components.
	int components = _kernelSize * _kernelSize * sample.channels();
    
	// Get the weighted pixel neighborhood from the sample.
	std::vector<float> kernel(components);
	sample.getNeighborhood(texelCoords, _kernelSize, kernel, true);

    // Return the transformed neighborhood.
	this->transform(kernel, desc);
}

// ------------------------------------------------------------------------------------------------
// 
// Search space properties
// 
// ------------------------------------------------------------------------------------------------

void ColorSearchSpace::sample(Sample& sample) const {
    _exemplar->clone(sample);
}

void ColorSearchSpace::sample(const Sample** const sample) const {
	*sample = _exemplar.get();
}

void ColorSearchSpace::kernel(int& kernel) const {
	kernel = _kernelSize;
}

void ColorSearchSpace::sampleSize(cv::Size& size) const {
	size = _exemplar->size();
}

void ColorSearchSpace::sampleSize(int& width, int& height) const {
	width = _exemplar->width();
	height = _exemplar->height();
}