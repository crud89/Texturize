#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////
///// API Exports/Imports                                                                     /////
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TEXTURIZE_EXPORTS
#define TEXTURIZE_API __declspec(dllexport) 
#else
#define TEXTURIZE_API /*__declspec(dllimport)*/
#endif

// Prototype to be used as follows: class TEXTURIZE_API(CORE) Foo { void Bar(void); };
// Would export if CORE is defined, otherwise it would import.
//#define TEXTURIZE_API(api) 
//	#if defined(api) __declspec(dllexport) 
//	#else __declspec(dllimport)
//	#endif
//#else
//#define TEXTURIZE_API /*__declspec(dllimport)*/
//#endif

// TODO: Implement pImpl based architecture to get rid of those warnings.
#pragma warning(disable:4275)	// Exporting STL types as base interface.
#pragma warning(disable:4251)	// Usage of STL types in class interfaces.

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Include all important headers                                                           /////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "version.hpp"
#include "error.hpp"
#include "traits.hpp"
#include "events.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Error reporting macros                                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TEXTURIZE_ERROR(code, msg)		Texturize::raiseError(__FUNCTION__, __FILE__, __LINE__, code, msg)

#define TEXTURIZE_ASSERT(e) \
	if(!!!(e)) TEXTURIZE_ERROR(TEXTURIZE_ERROR_ASSERT, #e);

#ifndef _DEBUG
#define TEXTURIZE_ASSERT_DBG(e) 
#else
#define TEXTURIZE_ASSERT_DBG(e) TEXTURIZE_ASSERT(e)
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Platform types		                                                                  /////
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned char			TX_BYTE;
typedef unsigned short			TX_WORD;
typedef unsigned long			TX_DWORD;
typedef unsigned long long		TX_QWORD;

#ifndef BYTE
#define BYTE TX_BYTE
#endif

#ifndef WORD
#define WORD TX_WORD
#endif

#ifndef DWORD
#define DWORD TX_DWORD
#endif

#ifndef QWORD
#define QWORD TX_QWORD
#endif