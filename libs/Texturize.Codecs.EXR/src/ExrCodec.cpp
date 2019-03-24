#include "stdafx.h"

#include <codecs\exr.hpp>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>

#include "Stream.hpp"

using namespace Texturize;
using namespace Imf;
using namespace Imath;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// OpenEXR codec implementation                                                            /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void EXRCodec::load(const std::string& fileName, Sample& sample) const
{
	std::ifstream stream(fileName, std::ios::in | std::ios::binary);
	this->load(stream, sample);
}

void EXRCodec::load(std::istream& stream, Sample& sample) const
{
	// Read the file header.
	IStreamImpl s(&stream);
	InputFile file(s);
	Header header = file.header();

	TEXTURIZE_ASSERT(file.isComplete());

	// Get the image size.
	Box2i dw = header.dataWindow();
	int width = dw.max.x - dw.min.x + 1;
	int height = dw.max.y - dw.min.y + 1;
	
	// Setup a frame buffer.
	// TODO: Remove raw pointer here.
	const ChannelList& channels = header.channels();
	std::vector<char*> frames;
	FrameBuffer buffer;

	for (ChannelList::ConstIterator c = channels.begin(); c != channels.end(); ++c)
	{
		// std::cout << "Extracting channel \"" << c.name() << "\"" << std::endl;
		const Channel& channel = c.channel();
		Slice slice;

		// Handle the different channel types.
		// NOTE: Currently only 32 bit floating point values are supported. Other formats require explicit conversion while restoring the channels into the sample later on.
		switch (channel.type)
		{
		case PixelType::FLOAT:
		{
			auto frame = new char[width * height * sizeof(float)];
			slice = Slice(PixelType::FLOAT, frame, sizeof(float), sizeof(float) * width, channel.xSampling, channel.ySampling);

			frames.push_back(frame);
			break;
		}
		case PixelType::HALF:
		//{
		//	auto frame = new char[width * height * sizeof(half)];
		//	slice = Slice(PixelType::HALF, frame, sizeof(half), sizeof(half) * width, channel.xSampling, channel.ySampling);

		//	frames.push_back(frame);
		//	break;
		//}
		case PixelType::UINT:
		//{
		//	auto frame = new char[width * height * sizeof(unsigned int)];
		//	slice = Slice(PixelType::UINT, frame, sizeof(unsigned int), sizeof(unsigned int) * width, channel.xSampling, channel.ySampling);

		//	frames.push_back(frame);
		//	break;
		//}
		default:
			TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "The codec could not load the requested sample due to an unsupported channel format.");
		}
		
		// Store the channel within the frame buffer.
		buffer.insert(c.name(), slice);
	}

	// Load the frame buffer.
	file.setFrameBuffer(buffer);
	file.readPixels(dw.min.y, dw.max.y);

	// Setup the sample.
	Sample result = Sample(frames.size(), width, height);

	// Setup each channel individually.
	for (size_t f(0); f < frames.size(); ++f)
	{
		// NOTE: The channel is assumed to be of 32 bit floating point precision.
		result.setChannel(static_cast<int>(f), cv::Mat(height, width, CV_32F, reinterpret_cast<void*>(frames[f])));
				
		// Delete the frame buffer.
		delete[] frames[f];
	}

	// Return the result.
	sample = result;
}

void EXRCodec::save(const std::string& fileName, const Sample& sample, const int depth) const
{
	std::ofstream stream(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
	this->save(stream, sample);
}

void EXRCodec::save(std::ostream& stream, const Sample& sample, const int depth) const
{
	// Setup the image header.
	Header header(sample.width(), sample.height());
	ChannelList& channels = header.channels();

	for (size_t c(0); c < sample.channels(); ++c)
		channels.insert(std::to_string(c), Channel(PixelType::FLOAT));

	// Setup the file stream.
	OStreamImpl s(&stream);
	OutputFile file(s, header);

	// Setup the frame buffer and pass each channel individually.
	std::vector<cv::Mat> channelBuffer(sample.channels());
	FrameBuffer buffer;

	for (size_t c(0); c < sample.channels(); ++c)
	{
		// Get the current channel.
		cv::Mat channel = sample.getChannel(static_cast<int>(c));

		TEXTURIZE_ASSERT(channel.isContinuous());							// Required in order to interpret the data directly.

		// Describe the channels memory layout: Each pixel is stored as 32-bit floating point value. 
		// Therefore size of each pixel along the x-axis equals 32 bit (xStride) and the size of an individual line equals 
		// the size of a pixel, multiplied by the number of columns (e.g. image width).
		Slice slice = Slice(PixelType::FLOAT, reinterpret_cast<char*>(channel.data), sizeof(float), sizeof(float) * sample.width());
				
		// Store the channel in a buffer.
		buffer.insert(std::to_string(c), slice);
		channelBuffer[c] = channel;
	}
	
	// Save the image (by writing each scanline).
	file.setFrameBuffer(buffer);
	file.writePixels(sample.height());
}