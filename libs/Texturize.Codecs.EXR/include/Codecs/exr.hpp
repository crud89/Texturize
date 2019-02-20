#pragma once

#include <texturize.hpp>
#include <analysis.hpp>
#include <codecs.hpp>

#include <string>
#include <iostream>

namespace Texturize {

	/// \brief 
	class TEXTURIZE_API EXRCodec :
		public ISampleCodec
	{
	public:
		virtual void load(const std::string& fileName, Sample& sample) const override;
		virtual void load(std::istream& stream, Sample& sample) const override;
		virtual void save(const std::string& fileName, const Sample& sample, const int depth = CV_8U) const override;
		virtual void save(std::ostream& stream, const Sample& sample, const int depth = CV_8U) const override;
	};
}