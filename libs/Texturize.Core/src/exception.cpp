#include "stdafx.h"
#include "texturize.hpp"

using namespace Texturize;

Exception::Exception() :
	_msg("An unknown error occured."), _code(0), std::exception()
{
}

Exception::Exception(int code, const char* msg) :
	_msg(msg), _code(code), std::exception(msg, code)
{
}

const char* Exception::what() const throw()
{
	// The C-style cast that violates Core Guideline Type.4 (prefer named casts or explicit conversions) is intented.
	[[gsl::suppress(type.4)]]
	{
		std::stringstream str;
		str << "Error " << this->_code << ": " << this->_msg;
		std::string s = str.str();
		return s.c_str();
	}
}