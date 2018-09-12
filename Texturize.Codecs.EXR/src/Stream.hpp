#include "stdafx.h"

#include <codecs/exr.hpp>
#include <ImfIO.h>
#include <ImathInt64.h>
#include <stdio.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

class IStreamImpl :
	public IStream
{
private:
	std::istream* _stream;

public:
	IStreamImpl(std::istream* stream) : 
		IStream(""), _stream(stream)
	{
		TEXTURIZE_ASSERT(_stream != nullptr);
	}

	virtual ~IStreamImpl()
	{
		//delete _stream;
	}

	virtual bool read(char c[], int n) override;
	virtual Int64 tellg() override;
	virtual void seekg(Int64 pos) override;
	virtual void clear() override;
};

class OStreamImpl :
	public OStream
{
private:
	std::ostream* _stream;

public:
	OStreamImpl(std::ostream* stream) :
		OStream(""), _stream(stream)
	{
		TEXTURIZE_ASSERT(_stream != nullptr);
	}

	virtual ~OStreamImpl()
	{
		//delete _stream;
	}

	virtual void write(const char c[], int n) override;
	virtual Int64 tellp() override;
	virtual void seekp(Int64 p) override;
};

bool IStreamImpl::read(char c[], int n)
{
	_stream->read(c, static_cast<std::streamsize>(n));
	return true;
}

Int64 IStreamImpl::tellg()
{
	return static_cast<Int64>(_stream->tellg());
}

void IStreamImpl::seekg(Int64 pos)
{
	_stream->seekg(static_cast<std::streampos>(pos));
}

void IStreamImpl::clear()
{
	_stream->clear();
}

void OStreamImpl::write(const char c[], int n)
{
	_stream->write(c, static_cast<std::streamsize>(n));
}

Int64 OStreamImpl::tellp()
{
	return static_cast<Int64>(_stream->tellp());
}

void OStreamImpl::seekp(Int64 p)
{
	_stream->seekp(static_cast<std::streampos>(p));
}