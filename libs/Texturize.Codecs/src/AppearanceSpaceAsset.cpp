#include "stdafx.h"

#include <codecs.hpp>
#include <iostream>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Appearance Space Descriptor persistence implementation                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void AppearanceSpaceAsset::store(std::shared_ptr<IFileStorage> storage, std::shared_ptr<const AppearanceSpace> asset) const
{
	TEXTURIZE_ASSERT(asset != nullptr);
	TEXTURIZE_ASSERT(storage != nullptr);

	// Write asset metadata.
	storage->write("META", "{");
	storage->write("TYPE", "ASD");
	storage->write("VERSION", "1.0");
	storage->write("META", "}");

	// Get the current asset state.
	std::shared_ptr<const cv::PCA> projection;
	std::shared_ptr<const Sample> exemplar;
	int kernel;

	asset->getProjector(projection);
	asset->getExemplar(exemplar);
	asset->getKernel(kernel);

	// Serialize the asset state.
	storage->write("EIGEN", "{");
	storage->write("VALUES", projection->eigenvalues);
	storage->write("VECTORS", projection->eigenvectors);
	storage->write("MEAN", projection->mean);
	storage->write("KERNEL", kernel);
	storage->write("EIGEN", "}");

	storage->write("EXEMPLAR", "{");
	storage->write("WIDTH", exemplar->width());
	storage->write("HEIGHT", exemplar->height());
	storage->write("SAMPLE", (cv::Mat)*exemplar);
	storage->write("EXEMPLAR", "}");
}

void AppearanceSpaceAsset::restore(std::shared_ptr<const IFileStorage> storage, std::unique_ptr<AppearanceSpace>& asset) const
{
	TEXTURIZE_ASSERT(asset == nullptr);
	TEXTURIZE_ASSERT(storage != nullptr);

	// Read asset metadata.
	std::vector<std::string> meta(2);
	storage->read("META/TYPE", meta[0]);
	storage->read("META/VERSION", meta[1]);
	
	// Validate the header.
	TEXTURIZE_ASSERT(meta.size() >= 2);
	TEXTURIZE_ASSERT(_strcmpi(meta[0].c_str(), "ASD") == 0);
	TEXTURIZE_ASSERT(_strcmpi(meta[1].c_str(), "1.0") == 0);

	// Read the asset state.
	std::unique_ptr<cv::PCA> projector = std::make_unique<cv::PCA>();
	cv::Mat ex;
	cv::Size size;
	int kernel;

	storage->read("EIGEN/VALUES", projector->eigenvalues);
	storage->read("EIGEN/VECTORS", projector->eigenvectors);
	storage->read("EIGEN/MEAN", projector->mean);
	storage->read("EIGEN/KERNEL", kernel);
	storage->read("EXEMPLAR/SAMPLE", ex);
	storage->read("EXEMPLAR/WIDTH", size.width);
	storage->read("EXEMPLAR/HEIGHT", size.height);

	// Validate the exemplar size.
	TEXTURIZE_ASSERT(size == ex.size());

	// Initialize a new descriptor instance.
	std::unique_ptr<Sample> exemplar = std::make_unique<Sample>(ex);
	asset = std::make_unique<AppearanceSpace>(std::move(projector), std::move(exemplar), kernel);
}

void AppearanceSpaceAsset::write(const std::string& fileName, std::shared_ptr<const AppearanceSpace> descriptor, const StorageFactory& storages) const
{
	// Create a new file storage instance.
	std::unique_ptr<IFileStorage> storage;
	storages.createStorage(fileName, storage, StorageFactory::FSM_WRITE, StorageFactory::FSF_HDF5);

	try
	{
		// Store the asset.
		this->store(std::move(storage), descriptor);
	}
	catch (const H5::Exception& ex)
	{
		std::cout << "Error storing asset: " << ex.getDetailMsg() << std::endl;
		throw;
	}
}

void AppearanceSpaceAsset::read(const std::string& fileName, std::unique_ptr<AppearanceSpace>& descriptor, const StorageFactory& storages) const
{
	// Create a new file storage instance.
	std::unique_ptr<IFileStorage> storage;
	storages.createStorage(fileName, storage, StorageFactory::FSM_READ, StorageFactory::FSF_HDF5);

	try
	{
		// Restore the asset.
		this->restore(std::move(storage), descriptor);
	}
	catch (const H5::Exception& ex)
	{
		std::cout << "Error restoring asset: " << ex.getCDetailMsg() << " (" << ex.getCFuncName() << ")" << std::endl;
		throw;
	}
}