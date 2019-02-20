#include "stdafx.h"

#include <codecs.hpp>
#include <H5File.h>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// File Storage Factory implementation                                                     /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void StorageFactory::createStorage(const std::string& fileName, IFileStorage** storage, StorageMode mode, StorageFormat format, const std::string& encoding) const
{
	TEXTURIZE_ASSERT(storage != nullptr);

	// Create an HDF5 file storage instance, depending on the provided format.
	std::unique_ptr<IFileStorage> s;

	if (format & FSF_HDF5)
	{
		// Also take into account the provided file mode.
		if (mode & FSM_READ)
			s = std::make_unique<Hdf5FileStorage>(fileName, FileStorage::READ | FileStorage::FORMAT_H5, encoding);
		else if (mode & FSM_WRITE)
			s = std::make_unique<Hdf5FileStorage>(fileName, FileStorage::WRITE | FileStorage::FORMAT_H5, encoding);
		else
			TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Unsupported file storage mode provided. Only read or write are allowed.");
	}
	else if (format & FSF_XML || format & FSF_YAML || format & FSF_JSON)
	{
		// Same as above.
		if (mode & FSM_READ)
			s = std::make_unique<OpenCvFileStorage>(fileName, format | cv::FileStorage::READ, encoding);
		else if (mode & FSM_WRITE)
			s = std::make_unique<OpenCvFileStorage>(fileName, format | cv::FileStorage::WRITE, encoding);
		else
			TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Unsupported file storage mode provided. Only read or write are allowed.");
	}
	else
	{
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Unsupported file storage format.");
	}

	// Return the storage pointer.
	*storage = s.release();
}