#include "stdafx.h"

#include <analysis.hpp>

#include <tbb\blocked_range2d.h>
#include <tbb\parallel_for_each.h>

#include <iostream>
#include <chrono>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Appearance Space implementation                                                         /////
///////////////////////////////////////////////////////////////////////////////////////////////////

AppearanceSpace::AppearanceSpace(const cv::PCA* projection, Sample* exemplar, const int kernelSize) :
	_projection(projection), _exemplar(exemplar), _kernelSize(kernelSize)
{
	TEXTURIZE_ASSERT(projection != nullptr);							// The PCA for dimensionality reduction must be initialized.
	TEXTURIZE_ASSERT(exemplar != nullptr);								// The exemplar must be initialized.
	TEXTURIZE_ASSERT(kernelSize > 0);									// The kernel size must not be negative.
	TEXTURIZE_ASSERT(kernelSize % 2 == 1);								// The kernel must have an uneven size.
}

cv::Mat AppearanceSpace::getComponents(const Sample& exemplar, int ks)
{
	// Initialize high-dimensional appearance space descriptors.
	int dimensionality = ks * ks * static_cast<int>(exemplar.channels());
	std::vector<cv::Mat> appearanceSpace(dimensionality);

	for (size_t d(0); d < dimensionality; ++d)
		appearanceSpace[d] = cv::Mat(exemplar.size(), CV_32FC1);

	// Extract all neighborhoods into individual texton descriptors.
	tbb::parallel_for(tbb::blocked_range2d<size_t>(0, exemplar.height(), 0, exemplar.width()),
		[&exemplar, &appearanceSpace, dimensionality, ks](const tbb::blocked_range2d<size_t>& range) {
		for (size_t x = range.cols().begin(); x < range.cols().end(); ++x) {
			for (size_t y = range.rows().begin(); y < range.rows().end(); ++y) {
				// Get the neighborhood texton.
				std::vector<float> neighborhood(dimensionality);
				exemplar.getNeighborhood(x, y, ks, neighborhood, true);

				// Store each component into a separate channel.
				for (size_t d(0); d < dimensionality; ++d)
					appearanceSpace[d].at<float>(y, x) = neighborhood[d];
			}
		}
	});

	// Reorganize the channels into rows of a single matrix.
	cv::Mat components(dimensionality, exemplar.height() * exemplar.width(), CV_32FC1);

	for (int d(0); d < dimensionality; ++d)
	{
		cv::Mat row = components.row(d);
		appearanceSpace[d].reshape(1, 1).convertTo(row, CV_32F);
	}

	return components;
}

Sample AppearanceSpace::mergeSamples(std::initializer_list<const Sample>& samples)
{
	// Calculate the number of channels in the result exemplar.
	int cn(0), targetChannel(0);
	int width = samples.begin()->width();
	int height = samples.begin()->height();

	for (auto ex : samples)
	{
		// The dimensions must be equal for all sample layers.
		TEXTURIZE_ASSERT(width == ex.width());
		TEXTURIZE_ASSERT(height == ex.height());

		// Accumulate the number of channels of each sample to get the number of channels within the target.
		cn += static_cast<int>(ex.channels());
	}

	// Create a new exemplar by merging the provided channels.
	Sample target(cn, width, height);

	for (auto ex : samples)
	{
		// Create a mapping between the source and target sample, where each channel is copied individually.
		std::vector<int> fromTo;

		for (int c(0); c < ex.channels(); ++c)
		{
			fromTo.push_back(c);										// Extract from channel 0..n in source exemplar
			fromTo.push_back(targetChannel++);							// To channel m..m+n in target exemplar; m represents the increment of all prevously mapped channels.
		}

		// Map the channels to the new exemplar.
		ex.extract(fromTo, target);
	}
	
	return target;
}

void AppearanceSpace::calculate(const Sample& exemplar, AppearanceSpace** desc, size_t rd, int ks)
{
	TEXTURIZE_ASSERT(desc != nullptr);
	TEXTURIZE_ASSERT(ks % 2 == 1);										// The kernel size must be odd (extent in two opposite directions - i.e. right/left or top/down - plus 1 for the center row/column)
	//TEXTURIZE_ASSERT(rd > 0 && rd <= exemplar.channels());			// The exemplar is required to be reduceable to a dimensionality larger than 0 and smaller than the original one.
	TEXTURIZE_ASSERT(rd > 0);											// The exemplar is required to be reduceable to a dimensionality larger than 0.

	// Get the exemplar components.
	cv::Mat components = AppearanceSpace::getComponents(exemplar, ks);
		
	// Perform the principal component analysis to reduce dimensionality and extract the coefficients.
	std::unique_ptr<cv::PCA> projector = std::make_unique<cv::PCA>(components, cv::Mat(), cv::PCA::DATA_AS_COL, static_cast<int>(rd));
	cv::Mat projected = projector->project(components);
	
	// The projected component space now should have one row for each channel.
	// In order to retain coordinate mappings between the original and the transformed exemplar, change it's shape.
	//TEXTURIZE_ASSERT(projected.rows == exemplar.channels());
	std::vector<cv::Mat> channels;

	for (int c(0); c < projected.rows; ++c)
		channels.push_back(projected.row(c));

	cv::merge(channels, projected);
	projected = projected.reshape(channels.size(), exemplar.height());

	// Setup and return a new descriptor.
	TEXTURIZE_ASSERT(projected.channels() == channels.size());
	TEXTURIZE_ASSERT(projected.rows == exemplar.height());
	std::unique_ptr<Sample> transformedExemplar = std::make_unique<Sample>(projected);
	*desc = new AppearanceSpace(projector.release(), transformedExemplar.release(), ks);
}

void AppearanceSpace::calculate(const Sample& exemplar, AppearanceSpace** desc, float tv, int ks)
{
	TEXTURIZE_ASSERT(desc != nullptr);
	TEXTURIZE_ASSERT(ks % 2 == 1);										// The kernel size must be odd (extent in two opposite directions - i.e. right/left or top/down - plus 1 for the center row/column)
	TEXTURIZE_ASSERT(tv > 0 && tv <= 1);								// Variance must be a value between 0.0 and 1.0.

	// Get the exemplar components.
	cv::Mat components = AppearanceSpace::getComponents(exemplar, ks);

	// Perform the principal component analysis to reduce dimensionality and extract the coefficients.
	std::unique_ptr<cv::PCA> projector = std::make_unique<cv::PCA>(components, cv::Mat(), cv::PCA::DATA_AS_COL, static_cast<double>(tv));
	cv::Mat projected = projector->project(components);

	// The projected component space now should have one row for each channel.
	// In order to retain coordinate mappings between the original and the transformed exemplar, change it's shape.
	//TEXTURIZE_ASSERT(projected.rows == exemplar.channels());
	std::vector<cv::Mat> channels;

	for (int c(0); c < projected.rows; ++c)
		channels.push_back(projected.row(c));

	cv::merge(channels, projected);
	projected = projected.reshape(channels.size(), exemplar.height());

	// Setup and return a new descriptor.
	TEXTURIZE_ASSERT(projected.channels() == channels.size());
	TEXTURIZE_ASSERT(projected.rows == exemplar.height());
	std::unique_ptr<Sample> transformedExemplar = std::make_unique<Sample>(projected);
	*desc = new AppearanceSpace(projector.release(), transformedExemplar.release(), ks);
}

void AppearanceSpace::calculate(std::initializer_list<const Sample> exemplarMaps, AppearanceSpace** desc, size_t rd, int ks)
{
	AppearanceSpace::calculate(mergeSamples(exemplarMaps), desc, rd, ks);
}

void AppearanceSpace::calculate(std::initializer_list<const Sample> exemplarMaps, AppearanceSpace** desc, float tv, int ks)
{
	AppearanceSpace::calculate(mergeSamples(exemplarMaps), desc, tv, ks);
}

void AppearanceSpace::getProjector(const cv::PCA** projection) const
{
	std::unique_ptr<cv::PCA> projector = std::make_unique<cv::PCA>();
	projector->eigenvectors = _projection->eigenvectors.clone();
	projector->eigenvalues = _projection->eigenvalues.clone();
	projector->mean = _projection->mean.clone();

	*projection = projector.release();
}

void AppearanceSpace::getExemplar(const Sample** exemplar) const
{
	Sample* copy;
	_exemplar->clone(&copy);
	*exemplar = copy;
}

void AppearanceSpace::getKernel(int& kernel) const
{
	kernel = _kernelSize;
}

void AppearanceSpace::transform(const std::vector<float>& texel, std::vector<float>& desc) const
{
	TEXTURIZE_ASSERT(texel.size() == _projection->mean.rows);				// The number of texel components must equal the number of components used to calculate the projector.

	// Transpose the input array to fit the DATA_AS_COL initialization.
	cv::Mat data = cv::Mat(texel.size(), 1, CV_32FC1, const_cast<float*>(texel.data()));
	desc = _projection->project(data);
}

void AppearanceSpace::transform(const Sample& sample, const int x, const int y, std::vector<float>& desc) const
{
	// Calculate the number of components.
	int components = _kernelSize * _kernelSize * sample.channels();
	
	TEXTURIZE_ASSERT_DBG(components == _projection->mean.rows);				// The number of channels of the indexed sample must match the number of channels of the original exemplar.

	// Get the pixel neighborhood.
	std::vector<float> kernel(components);
	sample.getNeighborhood(x, y, _kernelSize, kernel, true);

	this->transform(kernel, desc);
}

void AppearanceSpace::transform(const Sample& sample, const cv::Point& texelCoords, std::vector<float>& desc) const
{
	// Calculate the number of components.
	int components = _kernelSize * _kernelSize * sample.channels();

	TEXTURIZE_ASSERT_DBG(components == _projection->mean.rows);				// The number of channels of the indexed sample must match the number of channels of the original exemplar.

	// Get the pixel neighborhood.
	std::vector<float> kernel(components);
	sample.getNeighborhood(texelCoords, _kernelSize, kernel, true);

	this->transform(kernel, desc);
}

void AppearanceSpace::sample(Sample& sample) const
{
	std::vector<int> channelMap(_exemplar->channels() * 2);
	sample = Sample(_exemplar->channels(), _exemplar->size());
	
	for (int c(0); c < sample.channels(); ++c)
		channelMap[c * 2] = channelMap[c * 2 + 1] = c;

	sample.map(channelMap, *_exemplar);
}

void AppearanceSpace::sample(const Sample** const sample) const
{
	TEXTURIZE_ASSERT(sample != nullptr);

	*sample = _exemplar.get();
}

void AppearanceSpace::kernel(int& kernel) const
{
	this->getKernel(kernel);
}

void AppearanceSpace::sampleSize(cv::Size& size) const
{
	size = _exemplar->size();
}

void AppearanceSpace::sampleSize(int& width, int& height) const
{
	width = _exemplar->width();
	height = _exemplar->height();
}