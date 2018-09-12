#include "Texturize.Analysis.h"

#include <sstream>

using namespace Texturize;

AnalysisException::AnalysisException() :
	_msg("An unknown error occured."), _code(0), _ex(_msg.c_str(), _code)
{

}

AnalysisException::AnalysisException(int code, const char* msg) :
	_msg(msg), _code(code), _ex(_msg.c_str(), _code)
{

}

AnalysisException::~AnalysisException() throw()
{

}

const char* AnalysisException::what() const throw()
{
	std::stringstream str;
	str << "Error " << this->_code << ": " << this->_msg;
	std::string s = str.str();
	return s.c_str();
}