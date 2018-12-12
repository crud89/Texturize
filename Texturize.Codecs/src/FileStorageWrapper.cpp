#include "stdafx.h"

#include <codecs.hpp>
#include <filestorage.hpp>

#include <opencv2\opencv.hpp>

using namespace Texturize;
using namespace Texturize::Persistence;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// OpenCV File Storage (XML, YAML, JSON) implementation                                    /////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TStorage>
FileStorageWrapper<TStorage>::FileStorageWrapper() :
	_storage()
{
	TEXTURIZE_ASSERT(_storage.isOpened());
}

template <typename TStorage>
FileStorageWrapper<TStorage>::FileStorageWrapper(const std::string &fileName, int flags, const std::string &encoding) :
	_storage(fileName, flags, encoding)
{
	TEXTURIZE_ASSERT(_storage.isOpened());
}

template <typename TStorage>
FileStorageWrapper<TStorage>::~FileStorageWrapper()
{
	_storage.release();
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, int _data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, float _data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, double _data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::string &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const cv::Mat &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const cv::SparseMat &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<int> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<float> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<double> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<std::string> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<cv::KeyPoint> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const std::vector<cv::DMatch> &_data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::write(const std::string &name, const cv::Range& _data)
{
	_storage << name << _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, int &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, float &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, double &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::string &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, cv::Mat &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, cv::SparseMat &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<int> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<float> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<double> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<std::string> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<cv::KeyPoint> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, std::vector<cv::DMatch> &_data) const
{
	_storage[name] >> _data;
}

template <typename TStorage>
void FileStorageWrapper<TStorage>::read(const std::string &name, cv::Range& _data) const
{
	_storage[name] >> _data;
}