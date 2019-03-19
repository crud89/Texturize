#include "stdafx.h"

#include <codecs.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Default Sample Codec implementation                                                     /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DefaultCodec::load(const std::string& fileName, Sample& sample) const
{
	// NOTE: Since we do not know the number of channels within the image, they will allways be read
	//       with all color channels and need to be converted to grayscale later, if required.
	cv::Mat s = cv::imread(fileName, cv::IMREAD_UNCHANGED);
	sample = Sample(s);
}

void DefaultCodec::load(std::istream& stream, Sample& sample) const
{
	std::unique_ptr<std::streambuf> buffer(stream.rdbuf());
	std::streamsize size = buffer->pubseekoff(0, stream.end);
	buffer->pubseekoff(0, stream.beg);
	std::vector<uchar> data(size);
	buffer->sgetn(reinterpret_cast<char*>(data.data()), size);

	cv::Mat s = cv::imdecode(data, cv::IMREAD_UNCHANGED);
	sample = Sample(s);
}

void DefaultCodec::save(const std::string& fileName, const Sample& sample, const int depth) const
{
	cv::Mat s;
	float alpha = depth == CV_16U ? static_cast<float>(USHRT_MAX) : static_cast<float>(UCHAR_MAX);

	// If the image is a grayscale image, fill in the RGB channels required for the load-method.
	if (sample.channels() == 3)
	{
		((cv::Mat)sample).convertTo(s, depth, alpha);
	}
	else if (sample.channels() == 2)
	{
		cv::Mat r = sample.getChannel(0);
		cv::Mat g = sample.getChannel(1);
		r.convertTo(r, depth, alpha);
		g.convertTo(g, depth, alpha);
		std::vector<cv::Mat> cn{ cv::Mat::zeros(sample.size(), CV_MAKETYPE(depth, 1)), g, r };
		cv::merge(cn, s);
	}
	else if (sample.channels() == 1)
	{
		cv::Mat c = (cv::Mat)sample;
		c.convertTo(c, depth, alpha);
		std::vector<cv::Mat> cn{ c, c, c };
		cv::merge(cn, s);
	}
	else
	{
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Unsupported number of channels for default codec.");
	}

	if (!cv::imwrite(fileName, s))
		TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Error saving the sample using the specified codec.");
}

void DefaultCodec::save(std::ostream& stream, const Sample& sample, const int depth) const
{
	std::vector<uchar> buffer;
	cv::imencode("png", (cv::Mat)sample, buffer);
	stream.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}