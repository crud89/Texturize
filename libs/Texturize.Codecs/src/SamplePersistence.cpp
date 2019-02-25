#include "stdafx.h"

#include <codecs.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Sample Persistence implementation                                                       /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SamplePersistence::SamplePersistence(std::unique_ptr<ISampleCodec> defaultCodec) :
	_defaultCodec(std::move(defaultCodec))
{
	// NOTE: It does not matter if the codec is initialized or not. We have to check this everytime.
}

void SamplePersistence::registerCodec(const std::string& extension, std::unique_ptr<ISampleCodec> codec)
{
	TEXTURIZE_ASSERT_DBG(extension.find('.') == std::string::npos);				// Extensions are not expected to contain the "." character.
	TEXTURIZE_ASSERT(_codecs.find(extension) == _codecs.end());					// An extension can only be registered with one codec.
	TEXTURIZE_ASSERT(codec != nullptr);											// The codec must be initialized.

	_codecs.insert(std::make_pair(extension, std::move(codec)));
}

void SamplePersistence::loadSample(const std::string& fileName, Sample& sample) const
{
	// Get the extension of the file name.
	std::string extension = fileName.substr(fileName.find_last_of('.') + 1);

	// Lookup if there's a certain codec for the extension.
	auto codec = _codecs.find(extension);

	// If the codec exists, use it. If it doesn't, fall back to the default codec.
	// In case the default codec is not initialized, return an error.
	if (codec != _codecs.end())
		codec->second->load(fileName, sample);
	else if (_defaultCodec != nullptr)
		_defaultCodec->load(fileName, sample);
	else
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "No codec has been found for the provided file.");
}

void SamplePersistence::loadSample(std::istream& stream, const std::string& extension, Sample& sample) const
{
	// Lookup if there's a certain codec for the extension.
	auto codec = _codecs.find(extension);

	// If the codec exists, use it. If it doesn't, fall back to the default codec.
	// In case the default codec is not initialized, return an error.
	if (codec != _codecs.end())
		codec->second->load(stream, sample);
	else if (_defaultCodec != nullptr)
		_defaultCodec->load(stream, sample);
	else
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "No codec has been found for the provided file.");
}

void SamplePersistence::saveSample(const std::string& fileName, const Sample& sample, const int depth) const
{
	// Get the extension of the file name.
	std::string extension = fileName.substr(fileName.find_last_of('.') + 1);

	// Lookup if there's a certain codec for the extension.
	auto codec = _codecs.find(extension);

	// If the codec exists, use it. If it doesn't, fall back to the default codec.
	// In case the default codec is not initialized, return an error.
	if (codec != _codecs.end())
		codec->second->save(fileName, sample, depth);
	else if (_defaultCodec != nullptr)
		_defaultCodec->save(fileName, sample, depth);
	else
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "No codec has been found for the provided file.");
}

void SamplePersistence::saveSample(std::ostream& stream, const std::string& extension, const Sample& sample, const int depth) const
{
	// Lookup if there's a certain codec for the extension.
	auto codec = _codecs.find(extension);

	// If the codec exists, use it. If it doesn't, fall back to the default codec.
	// In case the default codec is not initialized, return an error.
	if (codec != _codecs.end())
		codec->second->save(stream, sample, depth);
	else if (_defaultCodec != nullptr)
		_defaultCodec->save(stream, sample, depth);
	else
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "No codec has been found for the provided file.");
}