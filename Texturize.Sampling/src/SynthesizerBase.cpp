#include "stdafx.h"

#include <sampling.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Synthesizer base implementation		                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

SynthesizerBase::SynthesizerBase(const SearchIndex* catalog) :
	_catalog(catalog)
{
	TEXTURIZE_ASSERT(catalog != nullptr);							// The search space must be initialized.
}

const SearchIndex* SynthesizerBase::getIndex() const
{
	return _catalog;
}