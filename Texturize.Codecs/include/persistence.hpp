#pragma once

#include "codecs.hpp"

using namespace Texturize;

#ifndef __cplusplus
#error The persistence.hpp header can only be compiled using C++.
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Sample persistence implementation                                                       /////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TDefaultCodec>
SamplePersistence_<TDefaultCodec>::SamplePersistence_() :
	SamplePersistence(new TDefaultCodec())
{
}

template <typename TDefaultCodec>
SamplePersistence_<TDefaultCodec>::~SamplePersistence_()
{
	delete this->_defaultCodec;
}