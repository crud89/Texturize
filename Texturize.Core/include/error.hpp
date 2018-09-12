#pragma once

#include "texturize.hpp"

#include "../errors.h"

#include <string>
#include <sstream>
#include <iostream>
#include <exception>

/// \namespace Texturize
/// \brief The root namespace, that contains all framework classes.
namespace Texturize {

	/// \brief An exception object that gets thrown by the `raiseError` methods.
	class TEXTURIZE_API Exception : public std::exception
	{
	public:
		/// \brief Creates a new exception instance.
		Exception();

		/// \brief Creates a new exception instance
		/// \param code An error code.
		/// \param msg A description of the error.
		Exception(int code, const char* msg);

		virtual ~Exception() throw();
		virtual const char* what() const throw();

		std::string _msg;
		int _code;
	};

	/// \brief Raises an error based on an `Exception` object.
	/// \param ex The exception to throw.
	inline void TEXTURIZE_API raiseError(const Exception& ex)
	{
		throw ex;
	}

	/// \brief Creates a new error and raises it.
	/// \param func The name of the function that reported the error.
	/// \param file The name of the source file, that contains the function, reporting the error.
	/// \param line The line number, the error was raised at.
	/// \param code An error code.
	/// \param msg A description of the error.
	inline void TEXTURIZE_API raiseError(const char* func, const char* file, int line, int code, const char* msg)
	{
		std::cout << file << " [line " << line << "] - Error " << code << ": " << msg << " (" << func << ")" << std::endl;

		raiseError(Exception(code, msg));
	}
};