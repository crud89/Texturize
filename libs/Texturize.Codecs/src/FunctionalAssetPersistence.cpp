#include "stdafx.h"

#include <codecs.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Asset Persistence implementation                                                        /////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TAsset>
FunctionalAssetPersistence<TAsset>::FunctionalAssetPersistence(ASSET_WRITER writer, ASSET_READER reader) :
	_writerFunc(writer), _readerFunc(reader)
{
}

template <typename TAsset>
void FunctionalAssetPersistence<TAsset>::store(std::shared_ptr<IFileStorage> storage, std::shared_ptr<const AppearanceSpace> asset) const
{
	TEXTURIZE_ASSERT(storage != nullptr);
	TEXTURIZE_ASSERT(asset != nullptr);

	_writerFunc(storage, asset);
}

template <typename TAsset>
void FunctionalAssetPersistence<TAsset>::restore(std::shared_ptr<const IFileStorage> storage, std::unique_ptr<AppearanceSpace>& asset) const
{
	TEXTURIZE_ASSERT(storage != nullptr);
	TEXTURIZE_ASSERT(asset != nullptr);

	_readerFunc(storage, asset);
}