#include "stdafx.h"

#include <analysis.hpp>

#include <tbb\blocked_range2d.h>
#include <tbb\parallel_for_each.h>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Sample implementation                                                                   /////
///////////////////////////////////////////////////////////////////////////////////////////////////

Sample::Sample(const cv::Mat& raw) :
	_channels(raw.channels())
{
	auto cn = raw.channels();
	std::vector<cv::Mat> channels(cn);
	cv::split(raw, channels);

	// Convert the channels, if required.
	for (int c(0); c < cn; ++c)
		channels[c].depth() != CV_32F && channels[c].depth() != CV_64F ?
			channels[c].convertTo(_channels[c], CV_32F, 1 / 255.f) :
			channels[c].convertTo(_channels[c], CV_32F);
}

Sample::Sample(const size_t channels) : 
	_channels(channels)
{
	TEXTURIZE_ASSERT(channels > 0);										// There must be at least one channel in the sample.

	for (int c(0); c < channels; c++)
		_channels[c] = cv::Mat(cv::Size(), CV_32FC1);
}

Sample::Sample(const size_t channels, const cv::Size& size) :
	_channels(channels)
{
	TEXTURIZE_ASSERT(channels > 0);										// There must be at least one channel in the sample.

	for (int c(0); c < channels; c++)
		_channels[c] = cv::Mat(size, CV_32FC1);
}

Sample::Sample(const size_t channels, const int width, const int height) :
	_channels(channels)
{
	TEXTURIZE_ASSERT(channels > 0);										// There must be at least one channel in the sample.

	for (int c(0); c < channels; c++)
		_channels[c] = cv::Mat(height, width, CV_32FC1);
}

void Sample::wrapCoords(int& x, int& y) const
{
	Sample::wrapCoords(_channels[0].cols, _channels[0].rows, x, y);
}

void Sample::wrapCoords(int width, int height, int& x, int& y)
{
	while (x < 0) x += width;
	while (x >= width) x -= width;
	while (y < 0) y += height;
	while (y >= height) y -= height;
}

void Sample::wrapCoords(int width, int height, cv::Point2i& coords)
{
	while (coords.x < 0) coords.x += width;
	while (coords.x >= width) coords.x -= width;
	while (coords.y < 0) coords.y += height;
	while (coords.y >= height) coords.y -= height;
}

void Sample::wrapCoords(cv::Vec2f& coords)
{
	coords[0] = coords[0] < 0.f ? 1.f - (coords[0] - static_cast<long>(coords[0])) : coords[0];
	coords[0] = coords[0] >= 1.f ? coords[0] - static_cast<long>(coords[0]) : coords[0];
	coords[1] = coords[1] < 0.f ? 1.f - (coords[1] - static_cast<long>(coords[1])) : coords[1];
	coords[1] = coords[1] >= 1.f ? coords[1] - static_cast<long>(coords[1]) : coords[1];
}

size_t Sample::channels() const
{
	return _channels.size();
}

cv::Size Sample::size() const
{
	cv::Size s;
	this->getSize(s);
	return s;
}

int Sample::width() const
{
	int x, y;
	this->getSize(x, y);
	return x;
}

int Sample::height() const
{
	int x, y;
	this->getSize(x, y);
	return y;
}

void Sample::setChannel(const int index, const cv::Mat& channel)
{
	TEXTURIZE_ASSERT(index >= 0 && index < _channels.size());			// The channel index must address an existing channel.
	_channels[index] = channel.clone();
}

void Sample::setTexel(const cv::Point2i& at, const std::vector<float>& texel)
{
	TEXTURIZE_ASSERT(texel.size() == this->channels());					// A texel must provide a value for each channel.

	for (size_t cn(0); cn < _channels.size(); ++cn)
		_channels[cn].at<float>(at) = texel[cn];
}

void Sample::setTexel(const int x, const int y, const std::vector<float>& texel)
{
	TEXTURIZE_ASSERT(texel.size() == this->channels());					// A texel must provide a value for each channel.

	for (size_t cn(0); cn < _channels.size(); ++cn)
		_channels[cn].at<float>(cv::Point2i(x, y)) = texel[cn];
}

void Sample::copyChannel(const int index, cv::Mat& channel) const
{
	channel = this->getChannel(index).clone();
}

cv::Mat Sample::getChannel(const int index) const
{
	TEXTURIZE_ASSERT(index >= 0 && index < _channels.size());			// The channel index must address an existing channel.
	return _channels[index].clone();
}

void Sample::getSize(cv::Size& size) const
{
	size = _channels[0].size();
}

void Sample::getSize(int& width, int& height) const
{
	cv::Size s = _channels[0].size();
	width = s.width;
	height = s.height;
}

void Sample::at(int& x, int& y, Texel& texel) const
{
	// Get the wrapped coords.
	this->wrapCoords(x, y);

	// Setup the texel.
	texel = Texel(_channels.size());

	// NOTE: OpenCV accesses pixels in row-major order.
	for (int i(0); i < _channels.size(); ++i)
		texel[i] = _channels[i].at<float>(y, x);
}

void Sample::at(cv::Point& p, Texel& texel) const
{
	// Get the wrapped coords.
	int _x(p.x), _y(p.y);
	this->at(_x, _y, texel);

	p.x = _x;
	p.y = _y;
}

void Sample::at(const float& u, const float& v, Texel& texel) const
{
	int x = static_cast<int>(this->width() * u);
	int y = static_cast<int>(this->height() * v);

	this->at(x, y, texel);
}

void Sample::at(const cv::Vec2f& uv, Texel& texel) const
{
	this->at(uv[0], uv[1], texel);
}

void Sample::extract(const int* fromTo, const size_t pairs, Sample& sample) const
{
	TEXTURIZE_ASSERT(fromTo != nullptr);								// The mapping array must be initialized.
	TEXTURIZE_ASSERT(pairs > 0);										// The number of pairs in the mapping array must be positive.

	std::vector<int> map(fromTo, fromTo + pairs * 2);
	this->extract(map, sample);
}

void Sample::extract(const std::vector<int>& fromTo, Sample& sample) const
{
	TEXTURIZE_ASSERT(fromTo.size() > 0 && fromTo.size() % 2 == 0);		// The vector contains pairs and there must be at least one pair and no value without counter-part.
	TEXTURIZE_ASSERT(sample.size() == this->size());					// The target sample should have the same size as the current sample.

	for (int i(0); i < fromTo.size(); i += 2)
	{
		int from = fromTo[i], to = fromTo[i + 1];
		TEXTURIZE_ASSERT(from >= 0 && from < _channels.size());			// The source index must address a valid channel within the current sample.
		TEXTURIZE_ASSERT(to >= 0 && to < sample.channels());			// The target index must address a valid channel within the target sample.

		sample.setChannel(to, _channels[from]);
	}
}

void Sample::extract(std::initializer_list<int> fromTo, Sample& sample) const
{
	std::vector<int> channelMap(fromTo);
	this->extract(channelMap, sample);
}

void Sample::map(const int* fromTo, const size_t pairs, const Sample& sample)
{
	TEXTURIZE_ASSERT(fromTo != nullptr);								// The mapping array must be initialized.
	TEXTURIZE_ASSERT(pairs > 0);										// The number of pairs in the mapping array must be positive.

	std::vector<int> map(fromTo, fromTo + pairs * 2);
	this->map(map, sample);
}

void Sample::map(const std::vector<int>& fromTo, const Sample& sample)
{
	TEXTURIZE_ASSERT(fromTo.size() > 0 && fromTo.size() % 2 == 0);		// The vector contains pairs and there must be at least one pair and no value without counter-part.

	for (int i(0); i < fromTo.size(); i += 2)
	{
		int from = fromTo[i], to = fromTo[i + 1];
		TEXTURIZE_ASSERT(from >= 0 && from < sample.channels());		// The source index must address a valid channel within the target sample.
		TEXTURIZE_ASSERT(to >= 0 && to < _channels.size());				// The target index must address a valid channel within the current sample.

		sample.copyChannel(from, _channels[to]);
	}
}

void Sample::map(std::initializer_list<int> fromTo, const Sample& sample)
{
	std::vector<int> channelMap(fromTo);
	this->map(channelMap, sample);
}

void Sample::merge(const Sample& with, Sample& to) const
{
	TEXTURIZE_ASSERT(with.size() == this->size());

	// Create a new sample that can contain all channels.
	Sample result = Sample(with.channels() + _channels.size(), with.width(), with.height());

	// Copy the channels of the current sample first.
	std::vector<int> channelMap(_channels.size() * 2);

	for (int c(0); c < _channels.size(); ++c)
		channelMap[c * 2] = channelMap[c * 2 + 1] = c;

	// Map the channels to the result.
	this->extract(channelMap, result);

	// Do the same with the channels of the other sample.
	channelMap.resize(with.channels() * 2);

	for (int c(0); c < with.channels(); ++c)
	{
		channelMap[c * 2] = c;
		channelMap[c * 2 + 1] = _channels.size() + c;
	}

	with.extract(channelMap, result);

	// Return the merged sample.
	to = result;
}

void Sample::getNeighborhood(const int x, const int y, const int k, Texel& neighborhood, const bool weight) const
{
	TEXTURIZE_ASSERT(k % 2 == 1);									// Only allow odd kernel sizes (even steps into each direction, plus the current pixel row/col)
	int extent = (k - 1) / 2, w(0), wx(0), wy(0);
	neighborhood = Texel(k * k * _channels.size());
	cv::Mat kernel = weight ? cv::getGaussianKernel(k, 0, CV_32F) : cv::Mat::ones(k, k, CV_32F);

	for (int _x(-extent); _x <= extent; ++_x)
	{
		for (int _y(-extent); _y <= extent; ++_y)
		{
			// Get the actual pixel coordinates.
			wx = x + _x; wy = y + _y;
			this->wrapCoords(wx, wy);

			// Calculate the weight of the pixel values.
			float g = weight ? kernel.at<float>(_x + extent) * kernel.at<float>(_y + extent) : 1.f;

			// Extract the value from each channel individually.
			// NOTE: OpenCV accesses pixels in row-major order.
			for (int c(0); c < _channels.size(); ++c)
				neighborhood[w++] = g * _channels[c].at<float>(wy, wx);
		}
	}
}

void Sample::getNeighborhood(const cv::Point& p, const int k, Texel& v, const bool weight) const
{
	return this->getNeighborhood(p.x, p.y, k, v, weight);
}

void Sample::clone(Sample& s) const
{
	s = Sample(_channels.size(), this->size());

	for (size_t cn(0); cn < _channels.size(); ++cn)
		s._channels[cn] = _channels[cn].clone();
}

void Sample::clone(Sample** const s) const
{
	std::unique_ptr<Sample> copy = std::make_unique<Sample>(_channels.size(), this->size());

	for (size_t cn(0); cn < _channels.size(); ++cn)
		copy->_channels[cn] = _channels[cn].clone();

	*s = copy.release();
}

Sample Sample::clone() const
{
	Sample copy;
	this->clone(copy);
	return copy;
}

inline Sample::operator cv::Mat() const
{
	cv::Mat result(this->size(), CV_32FC(static_cast<int>(this->channels())));
	cv::merge(_channels, result);

	//std::vector<int> channelMap(this->channels() * 2);

	//for (int c(0); c < this->channels(); ++c)
	//	channelMap[c * 2] = channelMap[c * 2 + 1] = c;

	//cv::mixChannels(_channels, result, channelMap);

	return result;
}

void Sample::weight(const float weight)
{
	for each(auto& channel in _channels)
		channel *= weight;
}

void Sample::sample(const Sample& sample, const cv::Mat& uv, Sample& to)
{
	Sample::sample(sample, uv, {}, to);
}

void Sample::sample(const Sample& sample, const cv::Mat& uv, const std::vector<int>& fromTo, Sample& to)
{
	Sample::sample(sample, uv, std::initializer_list<int>(fromTo.data(), fromTo.data() + fromTo.size()), to);
}

void Sample::sample(const Sample& sample, const cv::Mat& uv, std::initializer_list<int> fromTo, Sample& to)
{
	TEXTURIZE_ASSERT(fromTo.size() % 2 == 0);						// The channel mapping vector does provide pairs, thus the number of entry must be even.

	// If there is a channel mapping provided, use it, otherwise copy all channels.
	std::vector<int> channelMap;

	if (fromTo.size() == 0)
		for (int cn(0); cn < sample.channels(); ++cn)
			for (int i(0); i < 2; ++i)
				channelMap.push_back(cn);
	else
		channelMap = std::vector<int>(fromTo);
			
	to = Sample(channelMap.size() / 2, uv.cols, uv.rows);

	// Sample each pixel of the uv map.
	tbb::parallel_for(tbb::blocked_range2d<size_t>(0, uv.rows, 0, uv.cols),
		[&sample, &uv, &channelMap, &to](const tbb::blocked_range2d<size_t>& range) {
		for (size_t x = range.cols().begin(); x < range.cols().end(); ++x) {
			for (size_t y = range.rows().begin(); y < range.rows().end(); ++y) {
				// Get the coordinates and the texel.
				cv::Point2i pos = cv::Point2i(x, y);
				cv::Vec2f coords = uv.at<cv::Vec2f>(pos);
				std::vector<float> texel;
				sample.at(coords, texel);
				to.setTexel(pos, texel);
			}
		}
	});
}

void Sample::sample(const cv::Mat& uv, Sample& to) const
{
	Sample::sample(*this, uv, to);
}

void Sample::sample(const cv::Mat& uv, const std::vector<int>& fromTo, Sample& to) const
{
	Sample::sample(*this, uv, fromTo, to);
}

void Sample::sample(const cv::Mat& uv, std::initializer_list<int> fromTo, Sample& to) const
{
	Sample::sample(*this, uv, fromTo, to);
}

Sample Sample::mergeSamples(std::initializer_list<const Sample>& samples)
{
	// Calculate the number of channels in the result exemplar.
	int cn(0), targetChannel(0);
	int width = samples.begin()->width();
	int height = samples.begin()->height();

	for (auto ex : samples)
	{
		// The dimensions must be equal for all sample layers.
		TEXTURIZE_ASSERT(width == ex.width());
		TEXTURIZE_ASSERT(height == ex.height());

		// Accumulate the number of channels of each sample to get the number of channels within the target.
		cn += static_cast<int>(ex.channels());
	}

	// Create a new exemplar by merging the provided channels.
	Sample target(cn, width, height);

	for (auto ex : samples)
	{
		// Create a mapping between the source and target sample, where each channel is copied individually.
		std::vector<int> fromTo;

		for (int c(0); c < ex.channels(); ++c)
		{
			fromTo.push_back(c);										// Extract from channel 0..n in source exemplar
			fromTo.push_back(targetChannel++);							// To channel m..m+n in target exemplar; m represents the increment of all prevously mapped channels.
		}

		// Map the channels to the new exemplar.
		ex.extract(fromTo, target);
	}

	return target;
}