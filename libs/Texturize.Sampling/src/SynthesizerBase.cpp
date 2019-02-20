#include "stdafx.h"

#include <sampling.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Synthesizer base implementation		                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SynthesizerBase::SynthesizerBase(std::shared_ptr<ISearchIndex> catalog) :
	_catalog(std::move(catalog))
{
	TEXTURIZE_ASSERT(_catalog != nullptr);							// The search space must be initialized.
}

std::shared_ptr<ISearchIndex> SynthesizerBase::getIndex() const
{
	return _catalog;
}