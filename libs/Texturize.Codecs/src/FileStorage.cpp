///////////////////////////////////////////////////////////////////////////////////////////////////
///// HDF5 File Storage implementation.                                                       /////
///// Based on sources by Andrés Solís Montero (https://github.com/asolis/binaryStorage)      /////
/////                                                                                         /////
///// Original license note:                                                                  /////
/////                                                                                         /////
///// BSD 3-Clause License (https://www.tldrlegal.com/l/bsd3)                                 /////
/////                                                                                         /////
///// Copyright(c) 2016 Andrés Solís Montero <http://www.solism.ca>, All rights reserved.     /////
/////                                                                                         /////
///// Redistribution and use in source and binary forms, with or without modification, are    /////
///// permitted provided that the following conditions are met:                               /////
/////                                                                                         /////
///// 1. Redistributions of source code must retain the above copyright notice, this list of  /////
/////    conditions and the following disclaimer.                                             /////
///// 2. Redistributions in binary form must reproduce the above copyright notice, this list  /////
/////    of conditions and the following disclaimer in the documentation and/or other         /////
/////    materials provided with the distribution.                                            /////
///// 3. Neither the name of the copyright holder nor the names of its contributors may be    /////
/////    used to endorse or promote products derived from this software without specific      /////
/////    prior written permission.                                                            /////
/////                                                                                         /////
///// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY     /////
///// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF /////
///// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL   /////
///// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    /////
///// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT /////
///// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS             /////
///// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,       /////
///// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF /////
///// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.            /////
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <codecs.hpp>
#include <filestorage.hpp>

using namespace Texturize;
using namespace Texturize::Persistence;
using namespace Texturize::Persistence::Util;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// File Storage implementation.                                                            /////
///////////////////////////////////////////////////////////////////////////////////////////////////

const  std::string FileNode::TYPE	= "type";
const  std::string FileNode::ROWS	= "rows";
const  std::string FileNode::COLS	= "cols";
const  std::string FileNode::MTYPE	= "mTyp";

FileNode FileNodeIterator::operator*() const 
{
	return _fri->operator*();
}

FileNode FileNodeIterator::operator->() const
{
	return _fri->operator->();
}

FileNodeIterator& FileNodeIterator::operator++()
{
	_fri->increment(1);
	return *this;
}

FileNodeIterator FileNodeIterator::operator++(int)
{
	FileNodeIterator tmp(_fri);
	_fri->increment(1);
	return tmp;
}

FileNodeIterator& FileNodeIterator::operator--()
{
	_fri->decrement(1);
	return *this;
}

FileNodeIterator FileNodeIterator::operator--(int)
{
	FileNodeIterator tmp(_fri);
	_fri->decrement(1);
	return tmp;
}

FileNodeIterator& FileNodeIterator::operator+=(int ofs)
{
	_fri->increment(ofs);
	return *this;
}

FileNodeIterator& FileNodeIterator::operator-=(int ofs)
{
	_fri->decrement(ofs);
	return *this;
}

bool FileNodeIterator::operator==(const FileNodeIterator& rhs)
{
	return _fri->operator==(*rhs._fri);
}

bool FileNodeIterator::operator!=(const FileNodeIterator& rhs)
{
	return _fri->operator!=(*rhs._fri);
};

ptrdiff_t FileNodeIterator::operator-(const FileNodeIterator& rhs)
{
	return _fri->operator-(*rhs._fri);
};

bool FileNodeIterator::operator<(const FileNodeIterator& rhs)
{
	return _fri->operator<(*rhs._fri);
};

FileNode FileNode::operator[](const std::string &nodename) const
{
	return _fr->operator[](nodename);
};

FileNode FileNode::operator[](const char *nodename) const
{
	return _fr->operator[](std::string(nodename));
};

FileNode FileNode::operator[](int i)const
{
	return _fr->operator[](i);
}

int FileNode::type() const
{
	return _fr->type();
}

bool FileNode::empty() const
{
	return _fr->empty();
}

bool FileNode::isNone() const
{
	return _fr->isNone();
}

bool FileNode::isSeq() const
{
	return _fr->isSeq();
}

bool FileNode::isMap() const
{
	return _fr->isMap();
}

bool FileNode::isInt() const
{
	return _fr->isInt();
}

bool FileNode::isReal() const
{
	return _fr->isReal();
}

bool FileNode::isString() const
{
	return _fr->isString();
}

std::string FileNode::name() const
{
	return _fr->name();
}

size_t FileNode::size() const
{
	return _fr->size();
}

FileNode::operator int() const
{
	return _fr->operator int();
}

FileNode::operator float() const
{
	return _fr->operator float();
}

FileNode::operator double() const
{
	return _fr->operator double();
}

FileNode::operator std::string() const
{
	return _fr->operator std::string();
}

FileNodeIterator FileNode::begin() const
{
	return _fr->begin();
}

FileNodeIterator FileNode::end() const
{
	return _fr->end();
}

FileStorage::FileStorage()
{
}

FileStorage::FileStorage(const std::string &filename, int flags, const std::string &encoding) :
	_fw(((flags & FileStorage::FORMAT_MASK) == FileStorage::FORMAT_H5) ? new H5FileWriter(filename, flags) : new H5FileWriter(filename, flags))
{
}

bool FileStorage::open(const std::string &filename, int flags, const std::string &encoding)
{
	if ((flags & FileStorage::FORMAT_MASK) == FileStorage::FORMAT_H5)
		_fw = new H5FileWriter(filename, flags);
	else
		_fw = new H5FileWriter(filename, flags); //xml

	return isOpened();
}

bool FileStorage::isOpened() const
{
	return _fw && _fw->isOpened();
}

void FileStorage::release()
{
	if (_fw)
		_fw->release();
}

FileNode FileStorage::getFirstTopLevelNode() const
{
	return _fw->getFirstTopLevelNode();
}

FileNode FileStorage::root(int streamidx) const
{
	return _fw->root(streamidx);
}

FileNode FileStorage::operator[](const std::string& nodename) const
{
	return _fw->operator[](nodename);
}

FileNode FileStorage::operator[](const char* nodename) const
{
	return _fw->operator[](nodename);
}

std::string FileStorage::fullName(const std::string &node) const
{
	return _fw->fullName(node);
}

void FileStorage::nodeWritten()
{
	_fw->nodeWritten();
}

void FileStorage::startScope(const std::string &name, int type)
{
	_fw->startScope(name, type);
}

void FileStorage::endScope()
{
	_fw->endScope();
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, int _data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, float _data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, double _data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::string &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const cv::Mat &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const cv::SparseMat &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<int> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<float> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<double> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<std::string> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<cv::KeyPoint> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const std::vector<cv::DMatch> &_data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::write(FileStorage &fs, const std::string &name, const cv::Range& _data)
{
	fs._fw->write(name, _data);
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, int &_data, int default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, float &_data, float default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, double &_data, double default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::string &_data, const std::string &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, cv::Mat &_data, const cv::Mat &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, cv::SparseMat &_data, const cv::SparseMat &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<int> &_data, const std::vector<int> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<float> &_data, const std::vector<float> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<double> &_data, const std::vector<double> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<std::string> &_data, const std::vector<std::string> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<cv::KeyPoint> &_data, const std::vector<cv::KeyPoint> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, std::vector<cv::DMatch> &_data, const std::vector<cv::DMatch> &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

TEXTURIZE_API void Texturize::Persistence::read(const FileNode &fn, cv::Range& _data, const cv::Range &default_value)
{
	if (!fn._fr->read(fn._fr->name(), _data))
		_data = default_value;
}

bool H5Utils::getAttribute(H5::H5Object &node, const H5::DataType &type, const std::string &name, void *value)
{
	hsize_t dims[] = { 1 };
	H5::DataSpace attSpace(1, dims);

	if (!node.attrExists(name.c_str()))
		return false;
	
	H5::Attribute attribute = node.openAttribute(name.c_str());
	attribute.read(type, value);
	
	return true;
}

void H5Utils::setAttribute(H5::H5Object &node, const H5::DataType &type, const std::string &name, const void *value)
{
	hsize_t dims[] = { 1 };
	H5::DataSpace attSpace(1, dims);
	H5::Attribute attribute = (!node.attrExists(name.c_str())) ?
		node.createAttribute(name.c_str(), type, attSpace) :
		node.openAttribute(name.c_str());

	attribute.write(type, value);
}

void H5Utils::setStrAttribute(H5::H5Object &node, const std::string &name, const char *value)
{
	std::vector<const char*> arr_c_str;
	arr_c_str.push_back(value);
	setAttribute(node, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), name, arr_c_str.data());
}

void H5Utils::setStrAttribute(H5::H5Object &node, const std::string &name, const std::string &value)
{
	setAttribute(node, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), name, value.c_str());
}

void H5Utils::setIntAttribute(H5::H5Object &node, const std::string &name, const int &num)
{
	setAttribute(node, H5::PredType::NATIVE_INT, name, &num);
}

bool H5Utils::getNodeType(H5::H5Object &node, int &value)
{
	return getAttribute(node, H5::PredType::NATIVE_INT, FileNode::TYPE, &value);
}

void H5Utils::setNodeType(H5::H5Object &node, int type)
{
	setIntAttribute(node, FileNode::TYPE, type);
}

void H5Utils::setGroupAttribute(H5::H5File &file, const std::string &gname, const std::string &attr, int value)
{
	H5::Group group(file.openGroup(gname.c_str()));
	setIntAttribute(group, attr, value);
}

void H5Utils::createGroup(H5::H5File &file, const std::string &name, int type)
{
	H5::Group group(file.createGroup(name.c_str()));
	setNodeType(group, type);
}

bool H5Utils::sortSequenceDatasets(const std::string &a, const std::string &b)
{
	size_t _ia, _ib;
	std::istringstream(a) >> _ia;
	std::istringstream(b) >> _ib;

	return _ia < _ib;
}

herr_t H5Utils::readH5NodeInfo(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata)
{
	std::vector<std::string> *d = reinterpret_cast<std::vector<std::string> *>(opdata);
	d->push_back(name);

	return 0;
}

void H5Utils::listSubnodes(H5::H5Object &node, std::vector<std::string> &subnodes)
{
	H5Literate(node.getId(), H5_INDEX_NAME, H5_ITER_INC, nullptr, readH5NodeInfo, &subnodes);

	int type;
	bool read = getNodeType(node, type);

	if (read && type == FileNode::SEQ)
		std::sort(subnodes.begin(), subnodes.end(), H5Utils::sortSequenceDatasets);
}

void H5Utils::writeDataset1D(H5::H5File &file, const H5::DataType &type, const std::string &name, const void *data, const std::map<std::string, int> &attr, size_t count)
{
	hsize_t dims[] = { count };
	H5::DataSpace dataspace(1, dims);
	H5::DataSet dataset(file.createDataSet(name.c_str(), type, dataspace));

	for (std::map<std::string, int>::const_iterator it = attr.begin(); it != attr.end(); it++)
		setIntAttribute(dataset, it->first, it->second);

	dataset.write(data, type);
}

bool H5Utils::readDataset(const H5::H5File &file, const std::string &name, int type, void *data)
{
	H5::DataSet dataset = file.openDataSet(name.c_str());
	int _type;
	bool read = getNodeType(dataset, _type);

	if (read & (_type == type))
	{
		dataset.read(data, dataset.getDataType());
		return true;
	}

	return false;
}

H5FileWriter::H5FileWriter() :
	opened(false)
{
	state = FileStorage::UNDEFINED;
}

H5FileWriter::H5FileWriter(const std::string &filename, int flags) :
	opened(false)
{
	state = FileStorage::UNDEFINED;
	open(filename, flags, "");
}

bool H5FileWriter::open(const std::string &filename, int flags, const std::string &encoding)
{
	opened = false;
	
	try
	{
		if (flags & FileStorage::WRITE)
			_bfs = new H5::H5File(filename.c_str(), H5F_ACC_TRUNC);
		else if (flags & FileStorage::APPEND)
			_bfs = new H5::H5File(filename.c_str(), H5F_ACC_RDWR);
		else //if (flags & FileStorage::READ)
			_bfs = new H5::H5File(filename.c_str(), H5F_ACC_RDONLY);

		opened = true;
	}
	catch (H5::FileIException error)
	{
		opened = false;
	}

	state = (opened) ? FileStorage::NAME_EXPECTED + FileStorage::INSIDE_MAP : FileStorage::UNDEFINED;
	counter.push_back(0);

	return opened;
}

H5FileWriter::~H5FileWriter()
{
	release();
}

bool H5FileWriter::isOpened() const
{
	return opened;
}

void H5FileWriter::release()
{
	if (!isOpened())
		return;

	state = FileStorage::UNDEFINED;
}

FileNode H5FileWriter::getFirstTopLevelNode() const
{
	return FileNode(new H5FileReader(_bfs, "/"));
}

FileNode H5FileWriter::root(int streamidx) const
{
	return FileNode(new H5FileReader(_bfs, "/"));
}

FileNode H5FileWriter::operator[](const std::string& nodename) const
{
	return FileNode(new H5FileReader(_bfs, "/" + nodename));
}

FileNode H5FileWriter::operator[](const char* nodename) const
{
	return this->operator[](std::string(nodename));
}

std::string H5FileWriter::fullName(const std::string &node) const
{
	std::stringstream ss;
	ss << "/";

	for (size_t i = 0; i < groups.size(); i++)
		ss << groups[i] << "/";

	ss << ((node.empty()) ? std::to_string(counter.back()) : node);

	return ss.str();
}
void H5FileWriter::nodeWritten()
{
	if (state == FileStorage::VALUE_EXPECTED && elname == "" && groups.size() > 0)
	{
		counter.back()++;
	}
}

void H5FileWriter::startScope(const std::string &name, int type)
{
	H5Utils::createGroup(*_bfs, fullName(name).c_str(), type);
	groups.push_back(((name.empty()) ? std::to_string(counter.back()) : name));
	counter.push_back(0);
}

void H5FileWriter::endScope()
{
	groups.pop_back();
	counter.pop_back();
}

void H5FileWriter::write(const std::string &name, int data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::INT));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_INT, name.c_str(), &data, attr, 1);
}

void H5FileWriter::write(const std::string &name, float data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::REAL));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_FLOAT, name.c_str(), &data, attr, 1);
}

void H5FileWriter::write(const std::string &name, double data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::REAL));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_DOUBLE, name.c_str(), &data, attr, 1);
}

void H5FileWriter::write(const std::string &name, const std::string &data)
{
	std::vector<const char*> arr_c_str;
	arr_c_str.push_back(data.c_str());
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::STRING));
	H5Utils::writeDataset1D(*_bfs, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), name.c_str(), arr_c_str.data(), attr, 1);
}

void H5FileWriter::write(const std::string &name, const cv::Mat &data)
{
	hsize_t fdims[2], mdims[1];
	fdims[0] = static_cast<hsize_t>(data.rows);
	fdims[1] = data.cols * data.channels();
	H5::DataType type;

	switch (data.depth())
	{
	case CV_8U:  type = H5::PredType::NATIVE_UCHAR; break;
	case CV_8S:  type = H5::PredType::NATIVE_CHAR; break;
	case CV_16U: type = H5::PredType::NATIVE_UINT16; break;
	case CV_16S: type = H5::PredType::NATIVE_INT16; break;
	case CV_32S: type = H5::PredType::NATIVE_UINT; break;
	case CV_32F: type = H5::PredType::NATIVE_FLOAT; break;
	case CV_64F: type = H5::PredType::NATIVE_DOUBLE; break;
	default:
		type = H5::PredType::NATIVE_UCHAR;
		fdims[0] = data.cols * data.elemSize();
		break;
	}

	mdims[0] = fdims[1];

	H5::DataSpace fspace(2, fdims);
	H5::DataSpace mspace(1, mdims);

	H5::DataSet dataset(_bfs->createDataSet(name.c_str(), type, fspace));
	H5Utils::setIntAttribute(dataset, FileNode::ROWS, data.rows);
	H5Utils::setIntAttribute(dataset, FileNode::COLS, data.cols);
	H5Utils::setIntAttribute(dataset, FileNode::MTYPE, data.type());
	H5Utils::setNodeType(dataset, FileNode::USER);

	hsize_t offset[2], count[2], block[2];
	for (size_t i = 0; i < data.rows; i++)
	{
		offset[0] = i;
		offset[1] = 0;
		count[0] = 1;
		count[1] = 1;
		block[0] = 1;
		block[1] = fdims[1];
		fspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);

		offset[0] = 0;
		count[0] = mdims[0];
		block[0] = 1;
		mspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);

		dataset.write(data.ptr(i, 0), type, mspace, fspace);
	}
}

void H5FileWriter::write(const std::string &name, const cv::SparseMat &data)
{
	cv::Mat _tmp;
	data.copyTo(_tmp);
	write(name, _tmp);
}

void H5FileWriter::write(const std::string &name, const std::vector<int> &data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::SEQ));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_INT, name.c_str(), &data[0], attr, data.size());
}

void H5FileWriter::write(const std::string &name, const std::vector<float> &data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::SEQ));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_FLOAT, name.c_str(), &data[0], attr, data.size());
}

void H5FileWriter::write(const std::string &name, const std::vector<double> &data)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::SEQ));
	H5Utils::writeDataset1D(*_bfs, H5::PredType::NATIVE_DOUBLE, name.c_str(), &data[0], attr, data.size());
}

void H5FileWriter::write(const std::string &name, const std::vector<std::string> &vec)
{
	std::map<std::string, int> attr;
	attr.insert(std::make_pair(FileNode::TYPE, FileNode::SEQ));
	std::vector<const char*> arr_c_str;
	for (unsigned ii = 0; ii < vec.size(); ++ii)
		arr_c_str.push_back(vec[ii].c_str());
	H5Utils::writeDataset1D(*_bfs, H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), name.c_str(), arr_c_str.data(), attr, vec.size());
}

void H5FileWriter::write(const std::string &name, const std::vector<cv::KeyPoint> &ks)
{
	struct type_opencv_kp
	{
		float x;
		float y;
		float size;
		float angle;
		float response;
		int octave;
		int class_id;
	};

	hsize_t fdims[1], mdims[1];
	fdims[0] = ks.size();
	mdims[0] = FileNode::BUFFER; //buffer size of StorageNode::BUFFER keypoints

	H5::DataSpace fspace(1, fdims);
	H5::DataSpace mspace(1, mdims);

	H5::CompType mtype1(sizeof(type_opencv_kp));
	mtype1.insertMember("x", HOFFSET(type_opencv_kp, x), H5::PredType::NATIVE_FLOAT);
	mtype1.insertMember("y", HOFFSET(type_opencv_kp, y), H5::PredType::NATIVE_FLOAT);
	mtype1.insertMember("size", HOFFSET(type_opencv_kp, size), H5::PredType::NATIVE_FLOAT);
	mtype1.insertMember("angle", HOFFSET(type_opencv_kp, angle), H5::PredType::NATIVE_FLOAT);
	mtype1.insertMember("response", HOFFSET(type_opencv_kp, response), H5::PredType::NATIVE_FLOAT);
	mtype1.insertMember("octave", HOFFSET(type_opencv_kp, octave), H5::PredType::NATIVE_INT);
	mtype1.insertMember("class_i", HOFFSET(type_opencv_kp, class_id), H5::PredType::NATIVE_INT);
	H5::DataSet dataset(_bfs->createDataSet(name.c_str(), mtype1, fspace));
	H5Utils::setNodeType(dataset, FileNode::SEQ);

	hsize_t offset[1], count[1], block[1];

	size_t step = mdims[0];
	size_t chunks = (fdims[0] / mdims[0]) + ((fdims[0] % mdims[0]) ? 1 : 0);
	size_t extra = (fdims[0] % mdims[0]) ? fdims[0] % mdims[0] : mdims[0];

	for (size_t i = 0, cursor = 0; i < chunks; i++, cursor += step)
	{
		if (i == chunks - 1)
			step = extra;

		std::vector<type_opencv_kp> _kpts(step);
		for (size_t idx = 0; idx < step; idx++)
		{
			_kpts[idx].x = ks[cursor + idx].pt.x;
			_kpts[idx].y = ks[cursor + idx].pt.y;
			_kpts[idx].size = ks[cursor + idx].size;
			_kpts[idx].angle = ks[cursor + idx].angle;
			_kpts[idx].response = ks[cursor + idx].response;
			_kpts[idx].octave = ks[cursor + idx].octave;
			_kpts[idx].class_id = ks[cursor + idx].class_id;
		}

		offset[0] = cursor;
		count[0] = step;
		block[0] = 1;
		fspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		offset[0] = 0;
		block[0] = 1;
		mspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		dataset.write(_kpts.data(), mtype1, mspace, fspace);
	}
}

void H5FileWriter::write(const std::string &name, const std::vector<cv::DMatch> &ks)
{
	struct type_opencv_dm
	{
		int queryIdx;
		int trainIdx;
		int imgIdx;
		float distance;
	};


	hsize_t fdims[1], mdims[1];
	fdims[0] = ks.size();
	mdims[0] = FileNode::BUFFER; //buffer size of StorageNode::BUFFER keypoints

	H5::DataSpace fspace(1, fdims);
	H5::DataSpace mspace(1, mdims);

	H5::CompType mtype1(sizeof(type_opencv_dm));
	mtype1.insertMember("queryIdx", HOFFSET(type_opencv_dm, queryIdx), H5::PredType::NATIVE_INT);
	mtype1.insertMember("trainIdx", HOFFSET(type_opencv_dm, trainIdx), H5::PredType::NATIVE_INT);
	mtype1.insertMember("imgIdx", HOFFSET(type_opencv_dm, imgIdx), H5::PredType::NATIVE_INT);
	mtype1.insertMember("distance", HOFFSET(type_opencv_dm, distance), H5::PredType::NATIVE_FLOAT);
	H5::DataSet dataset(_bfs->createDataSet(name.c_str(), mtype1, fspace));
	H5Utils::setNodeType(dataset, FileNode::SEQ);


	hsize_t offset[1], count[1], block[1];

	size_t step = mdims[0];
	size_t chunks = (fdims[0] / mdims[0]) + ((fdims[0] % mdims[0]) ? 1 : 0);
	size_t extra = (fdims[0] % mdims[0]) ? fdims[0] % mdims[0] : mdims[0];

	for (size_t i = 0, cursor = 0; i < chunks; i++, cursor += step)
	{
		if (i == chunks - 1)
			step = extra;

		std::vector<type_opencv_dm> _kpts(step);
		for (size_t idx = 0; idx < step; idx++)
		{
			_kpts[idx].queryIdx = ks[cursor + idx].queryIdx;
			_kpts[idx].trainIdx = ks[cursor + idx].trainIdx;
			_kpts[idx].imgIdx = ks[cursor + idx].imgIdx;
			_kpts[idx].distance = ks[cursor + idx].distance;
		}

		offset[0] = cursor;
		count[0] = step;
		block[0] = 1;
		fspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		offset[0] = 0;
		block[0] = 1;
		mspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		dataset.write(_kpts.data(), mtype1, mspace, fspace);
	}
}

void H5FileWriter::write(const std::string &name, const cv::Range& _data)
{
	std::vector<int> _tmp;
	_tmp.push_back(_data.start);
	_tmp.push_back(_data.end);
	write(name, _tmp);
}

int FileReader::type() const
{
	return _type;
}

bool FileReader::empty() const
{
	return type() == FileNode::NONE;
}

bool FileReader::isNone() const
{
	return type() == FileNode::NONE;
}

bool FileReader::isSeq() const
{
	return type() == FileNode::SEQ;
}

bool FileReader::isMap() const
{
	return type() == FileNode::MAP;
}

bool FileReader::isInt() const
{
	return type() == FileNode::INT;
}

bool FileReader::isReal() const
{
	return type() == FileNode::REAL;
}

bool FileReader::isString() const
{
	return type() == FileNode::STRING;
}

std::string FileReader::name() const
{
	return _name;
}

FileReader::operator int() const
{
	int integer;

	if (!read(_name, integer))
		return 0;

	return integer;
}

FileReader::operator float() const
{
	float _float;

	if (!read(_name, _float))
		return 0.f;

	return _float;
}

FileReader::operator double() const
{
	double _double;

	if (!read(_name, _double))
		return 0.;

	return _double;
}

FileReader::operator std::string() const
{
	std::string tmp;

	if (!read(_name, tmp))
		return "";

	return tmp;
}

H5FileReader::H5FileReader() :
	_fn(cv::Ptr<H5::H5File>()), _sub(0)
{
	_type = FileNode::NONE;
	_name = "";
}

H5FileReader::H5FileReader(const cv::Ptr<H5::H5File> &f, const std::string &name) :
	_fn(f)
{
	_type = FileNode::NONE;
	_name = name;
	bool _read = false;

	try
	{
		H5::Exception::dontPrint();
		H5::DataSet *loc = new H5::DataSet(f->openDataSet(name.c_str()));
		bool read = H5Utils::getNodeType(*loc, _type);
		if (!read)
			_type = FileNode::NONE;
		H5Utils::listSubnodes(*loc, _sub);
		_read = true;
		delete loc;
	}
	catch (H5::FileIException not_found_error)
	{
		try
		{
			H5::Exception::dontPrint();
			H5::Group *loc = new H5::Group(f->openGroup(name.c_str()));
			bool read = H5Utils::getNodeType(*loc, _type);
			if (!read)
				_type = FileNode::NONE;
			H5Utils::listSubnodes(*loc, _sub);
			_read = true;
			delete loc;
		}
		catch (H5::FileIException not_found_error)
		{

		}
	}

	if (!_read)
		_type = FileNode::NONE;
}

FileNode H5FileReader::operator[](const std::string &nodename) const
{
	std::vector<std::string>::const_iterator it;
	it = std::find(_sub.begin(), _sub.end(), nodename);
	if (it != _sub.end())
	{
		std::string fName = (_name.back() != '/') ?
			(_name + "/" + nodename) : (_name + nodename);
		return FileNode(new H5FileReader(_fn, fName));
	}
	else
		return FileNode();
}

FileNode H5FileReader::operator[](const char *nodename) const
{
	return this->operator[](std::string(nodename));
}

FileNode H5FileReader::operator[](int i) const
{
	if (i >= 0 && i < _sub.size())
	{
		std::string fName = (_name.back() != '/') ?
			(_name + "/" + _sub[i]) : (_name + _sub[i]);
		return FileNode(new H5FileReader(_fn, fName));
	}
	else
		return FileNode();
}

size_t H5FileReader::size() const
{
	return _sub.size();
}

FileNodeIterator H5FileReader::begin() const
{
	cv::Ptr<H5FileReaderIterator> it = new H5FileReaderIterator(*this, 0);
	return FileNodeIterator(it);
}

FileNodeIterator H5FileReader::end() const
{
	cv::Ptr<H5FileReaderIterator> it = new H5FileReaderIterator(*this, _sub.size());
	return FileNodeIterator(it);
}

bool H5FileReader::read(const std::string &name, int &data) const
{
	return H5Utils::readDataset(*_fn, name.c_str(), FileNode::INT, &data);
}

bool H5FileReader::read(const std::string &name, float &data) const
{
	return H5Utils::readDataset(*_fn, name.c_str(), FileNode::REAL, &data);
}

bool H5FileReader::read(const std::string &name, double &data) const
{
	return H5Utils::readDataset(*_fn, name.c_str(), FileNode::REAL, &data);
}

bool H5FileReader::read(const std::string &name, std::string &data) const
{
	H5::DataSet dataset = _fn->openDataSet(name.c_str());
	int _type;
	bool read = H5Utils::getNodeType(dataset, _type);
	if (read & (_type == FileNode::STRING))
	{
		dataset.read(data, dataset.getDataType());
		return true;
	}
	return false;
}

bool H5FileReader::read(const std::string &name, cv::Mat &data) const
{
	H5::DataSet dataset = _fn->openDataSet(name.c_str());
	int row, col, type, mtype;

	bool read = true;
	read &= H5Utils::getNodeType(dataset, type);
	read &= H5Utils::getAttribute(dataset, H5::PredType::NATIVE_INT, FileNode::ROWS, &row);
	read &= H5Utils::getAttribute(dataset, H5::PredType::NATIVE_INT, FileNode::COLS, &col);
	read &= H5Utils::getAttribute(dataset, H5::PredType::NATIVE_INT, FileNode::MTYPE, &mtype);
	read &= (type == FileNode::USER);

	if (!read)
		return read;

	data.create(row, col, mtype);
	dataset.read(data.data, dataset.getDataType());
	return true;
}

bool H5FileReader::read(const std::string &name, cv::SparseMat &data) const
{
	cv::Mat _tmp;
	bool _read = read(name, _tmp);
	if (!_read)
		return _read;
	data = _tmp.clone();
	return true;
}

bool H5FileReader::read(const std::string &name, std::vector<int> &data) const
{
	return H5Utils::readDataset1D<int>(*_fn, name.c_str(), data);
}

bool H5FileReader::read(const std::string &name, std::vector<float> &data) const
{
	return H5Utils::readDataset1D<float>(*_fn, name.c_str(), data);
}

bool H5FileReader::read(const std::string &name, std::vector<double> &data) const
{
	return H5Utils::readDataset1D<double>(*_fn, name.c_str(), data);
}

bool H5FileReader::read(const std::string &name, std::vector<std::string> &data) const
{
	return H5Utils::readDataset1D<std::string>(*_fn, name.c_str(), data);
}

bool H5FileReader::read(const std::string &name, std::vector<cv::KeyPoint> &ks) const
{
	struct type_opencv_kp
	{
		float x;
		float y;
		float size;
		float angle;
		float response;
		int octave;
		int class_id;
	};

	H5::DataSet dataset = _fn->openDataSet(name.c_str());
	H5::DataSpace fspace = dataset.getSpace();
	hsize_t fdims[1], mdims[1];
	int rank = fspace.getSimpleExtentDims(fdims, NULL);

	mdims[0] = FileNode::BUFFER;
	H5::DataSpace mspace(rank, mdims);


	int type(FileNode::NONE);
	bool read = H5Utils::getNodeType(dataset, type) &
		(type == FileNode::SEQ) &
		(rank == 1);

	if (!read)
		return read;

	hsize_t offset[1], count[1], block[1];

	size_t step = mdims[0];
	size_t chunks = (fdims[0] / mdims[0]) + ((fdims[0] % mdims[0]) ? 1 : 0);
	size_t extra = (fdims[0] % mdims[0]) ? fdims[0] % mdims[0] : mdims[0];

	for (size_t i = 0, cursor = 0; i < chunks; i++, cursor += step)
	{
		if (i == chunks - 1)
			step = extra;

		std::vector<type_opencv_kp> _kpts(step);

		offset[0] = cursor;
		count[0] = step;
		block[0] = 1;
		fspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		offset[0] = 0;
		block[0] = 1;
		mspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		dataset.read(_kpts.data(), dataset.getDataType(), mspace, fspace);


		for (size_t i = 0; i< step; i++)
		{
			cv::KeyPoint p;
			p.pt.x = _kpts[i].x;
			p.pt.y = _kpts[i].y;
			p.size = _kpts[i].size;
			p.angle = _kpts[i].angle;
			p.response = _kpts[i].response;
			p.octave = _kpts[i].octave;
			p.class_id = _kpts[i].class_id;
			ks.push_back(p);
		}
	}
	return true;
}

bool H5FileReader::read(const std::string &name, std::vector<cv::DMatch> &dm) const
{
	struct type_opencv_dm
	{
		int queryIdx;
		int trainIdx;
		int imgIdx;
		float distance;
	};

	H5::DataSet dataset = _fn->openDataSet(name.c_str());
	H5::DataSpace fspace = dataset.getSpace();
	hsize_t fdims[2], mdims[2];
	int rank = fspace.getSimpleExtentDims(fdims, NULL);

	mdims[0] = FileNode::BUFFER;
	H5::DataSpace mspace(rank, mdims);

	int type(FileNode::NONE);
	bool read = H5Utils::getNodeType(dataset, type) &
		(type == FileNode::SEQ) &
		(rank == 1);

	if (!read)
		return read;
	hsize_t offset[1], count[1], block[1];

	size_t step = mdims[0];
	size_t chunks = (fdims[0] / mdims[0]) + ((fdims[0] % mdims[0]) ? 1 : 0);
	size_t extra = (fdims[0] % mdims[0]) ? fdims[0] % mdims[0] : mdims[0];

	for (size_t i = 0, cursor = 0; i < chunks; i++, cursor += step)
	{
		if (i == chunks - 1)
			step = extra;

		std::vector<type_opencv_dm> _dm(step);

		offset[0] = cursor;
		count[0] = step;
		block[0] = 1;
		fspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		offset[0] = 0;
		block[0] = 1;
		mspace.selectHyperslab(H5S_SELECT_SET, count, offset, NULL, block);
		dataset.read(_dm.data(), dataset.getDataType(), mspace, fspace);


		for (size_t i = 0; i< step; i++)
		{
			cv::DMatch p;
			p.queryIdx = _dm[i].queryIdx;
			p.trainIdx = _dm[i].trainIdx;
			p.imgIdx = _dm[i].imgIdx;
			p.distance = _dm[i].distance;
			dm.push_back(p);
		}
	}
	return true;
}

bool H5FileReader::read(const std::string &name, cv::Range& _data) const
{
	std::vector<int> _tmp;
	bool _read = H5Utils::readDataset1D<int>(*_fn, name.c_str(), _tmp);
	if (!_read)
		return _read;
	_data.start = _tmp[0];
	_data.end = _tmp[1];

	return true;
}

H5FileReaderIterator::H5FileReaderIterator() :
	_pos(0), _node()
{
}

H5FileReaderIterator::H5FileReaderIterator(const H5FileReader &node, size_t pos) :
	_pos(pos), _node(node)
{
}

FileNode H5FileReaderIterator::operator *() const
{
	if (_pos < _node._sub.size())
	{
		std::string fName = (_node._name.back() != '/') ?
			(_node._name + "/" + _node._sub[_pos]) :
			(_node._name + _node._sub[_pos]);
		return FileNode(new H5FileReader(_node._fn, fName));
	}
	else
		return FileNode();
}

FileNode H5FileReaderIterator::operator ->() const
{
	return this->operator*();
}

void H5FileReaderIterator::increment(size_t i)
{
	_pos += i;

	if (_pos > _node._sub.size())
		_pos = _node._sub.size();
}

void H5FileReaderIterator::decrement(size_t i)
{
	_pos -= i;

	if (_pos > _node._sub.size())
		_pos = _node._sub.size();
}

bool H5FileReaderIterator::operator == (const FileReaderIterator& rhs)
{
	const H5FileReaderIterator *_rhs = dynamic_cast<const H5FileReaderIterator*>(&rhs);

	return this->_pos == _rhs->_pos && this->_node._fn == _rhs->_node._fn && this->_node._name == _rhs->_node._name;
}

bool H5FileReaderIterator::operator != (const FileReaderIterator& rhs)
{
	return !(*this == rhs);
}

ptrdiff_t H5FileReaderIterator::operator- (const FileReaderIterator& rhs)
{
	const H5FileReaderIterator *_rhs = dynamic_cast<const H5FileReaderIterator*>(&rhs);

	if (this->_node._fn == _rhs->_node._fn && this->_node._name == _rhs->_node._name)
		return this->_pos - _rhs->_pos;

	return this->_pos;
}

bool H5FileReaderIterator::operator < (const FileReaderIterator& rhs)
{
	const H5FileReaderIterator *_rhs = dynamic_cast<const H5FileReaderIterator*>(&rhs);

	return this->_pos < _rhs->_pos && this->_node._fn == _rhs->_node._fn && this->_node._name == _rhs->_node._name;
}