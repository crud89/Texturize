#include "stdafx.h"

#include <sampling.hpp>

#include <algorithm>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SearchIndex base interface                                                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SearchIndex::SearchIndex(const std::shared_ptr<ISearchSpace> searchSpace, const std::shared_ptr<IDescriptorExtractor> descriptorExtractor, cv::NormTypes normType) :
	_searchSpace(std::move(searchSpace)), _descriptorExtractor(std::move(descriptorExtractor)), _normType(normType)
{
	TEXTURIZE_ASSERT(searchSpace != nullptr);
	TEXTURIZE_ASSERT(descriptorExtractor != nullptr);
}

std::shared_ptr<ISearchSpace> SearchIndex::getSearchSpace() const
{
	return _searchSpace;
}

std::shared_ptr<IDescriptorExtractor> SearchIndex::getDescriptorExtractor() const
{
	return _descriptorExtractor;
}