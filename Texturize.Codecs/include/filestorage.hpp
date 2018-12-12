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

#pragma once

#include <texturize.hpp>

#include <map>
#include <iostream>
#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "H5Cpp.h"

namespace Texturize {
	namespace Persistence {

		namespace Util {
			class TEXTURIZE_API H5Utils {
			public:
				static bool getAttribute(H5::H5Object &node, const H5::DataType &type, const std::string &name, void *value);
				static void setAttribute(H5::H5Object &node, const H5::DataType &type, const std::string &name, const void *value);
				static void setStrAttribute(H5::H5Object &node, const std::string &name, const char *value);
				static void setStrAttribute(H5::H5Object &node, const std::string &name, const std::string &value);
				static void setIntAttribute(H5::H5Object &node, const std::string &name, const int &num);
				static bool getNodeType(H5::H5Object &node, int &value);
				static void setNodeType(H5::H5Object &node, int type);
				static void setGroupAttribute(H5::H5File &file, const std::string &gname, const std::string &attr, int value);
				static void createGroup(H5::H5File &file, const std::string &name, int type);
				static bool sortSequenceDatasets(const std::string &a, const std::string &b);
				static herr_t readH5NodeInfo(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata);
				static void listSubnodes(H5::H5Object &node, std::vector<std::string> &subnodes);
				static void getNode(H5::H5File &file, const std::string &name, H5::H5Object &node);
				static void writeDataset1D(H5::H5File &file, const H5::DataType &type, const std::string &name, const void *data, const std::map<std::string, int> &attr, size_t count);

				template<typename _Tp>
				static bool readDataset1D(const H5::H5File &file, const std::string &name, std::vector<_Tp> &data)
				{
					H5::DataSet dataset = file.openDataSet(name);
					H5::DataSpace dataspace = dataset.getSpace();
					hsize_t dims_out[1];
					int rank = dataspace.getSimpleExtentDims(dims_out, NULL);

					int _type;
					bool read = getNodeType(dataset, _type);
					read &= (_type == FileNode::SEQ);
					read &= (rank == 1);

					if (!read)
						return read;
					data.resize(dims_out[0]);
					dataset.read(data.data(), dataset.getDataType());
					return true;
				}

				static bool readDataset(const H5::H5File &file, const std::string &name, int type, void *data);
			};
		}

		using namespace Texturize::Persistence::Util;

		class FileNode;
		class FileNodeIterator;
		class FileStorage;
		
		class TEXTURIZE_API FileWriter {
		protected:
			friend TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const std::string &str);

			template<typename _Tp>
			friend TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const _Tp& value);

			int							state;
			std::string					elname;
			std::vector<char>			structs;

		public:
			virtual bool open(const std::string &filename, int flags, const std::string &encoding = std::string()) = 0;
			virtual bool isOpened() const = 0;
			virtual void release() = 0;

			virtual FileNode getFirstTopLevelNode() const = 0;
			virtual FileNode root(int streamidx = 0) const = 0;
			virtual FileNode operator[](const std::string& nodename) const = 0;
			virtual FileNode operator[](const char* nodename) const = 0;

			virtual std::string fullName(const std::string &node) const = 0;
			virtual void nodeWritten() = 0;
			virtual void startScope(const std::string &name, int type) = 0;
			virtual void endScope() = 0;

			virtual void write(const std::string &name, int _data) = 0;
			virtual void write(const std::string &name, float _data) = 0;
			virtual void write(const std::string &name, double _data) = 0;
			virtual void write(const std::string &name, const std::string &_data) = 0;
			virtual void write(const std::string &name, const cv::Mat &_data) = 0;
			virtual void write(const std::string &name, const cv::SparseMat &_data) = 0;
			virtual void write(const std::string &name, const std::vector<int> &_data) = 0;
			virtual void write(const std::string &name, const std::vector<float> &_data) = 0;
			virtual void write(const std::string &name, const std::vector<double> &_data) = 0;
			virtual void write(const std::string &name, const std::vector<std::string> &_data) = 0;
			virtual void write(const std::string &name, const std::vector<cv::KeyPoint> &_data) = 0;
			virtual void write(const std::string &name, const std::vector<cv::DMatch> &_data) = 0;
			virtual void write(const std::string &name, const cv::Range& _data) = 0;

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Point_<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.x);
				_tmp.push_back(_data.y);
				write(name, _tmp);
			}

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Point3_<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.x);
				_tmp.push_back(_data.y);
				_tmp.push_back(_data.z);
				write(name, _tmp);
			}

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Size_<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.width);
				_tmp.push_back(_data.height);
				write(name, _tmp);
			}

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Complex<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.re);
				_tmp.push_back(_data.im);
				write(name, _tmp);
			}

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Rect_<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.x);
				_tmp.push_back(_data.y);
				_tmp.push_back(_data.width);
				_tmp.push_back(_data.height);
				write(name, _tmp);
			}

			template<typename _Tp>
			inline void write(const std::string &name, const cv::Scalar_<_Tp>& _data)
			{
				std::vector<_Tp> _tmp;
				_tmp.push_back(_data.val[0]);
				_tmp.push_back(_data.val[1]);
				_tmp.push_back(_data.val[2]);
				_tmp.push_back(_data.val[3]);
				write(name, _tmp);
			}
		};

		class TEXTURIZE_API H5FileWriter : public FileWriter {
		protected:
			bool						opened;

		private:
			cv::Ptr<H5::H5File>			_bfs;
			std::vector<std::string>	groups;
			std::vector<size_t>			counter;

		public:
			H5FileWriter();
			H5FileWriter(const std::string &filename, int flags);
			virtual ~H5FileWriter();

		public:
			bool open(const std::string &filename, int flags, const std::string &encoding = std::string()) override;
			bool isOpened() const override;
			void release() override;

			FileNode getFirstTopLevelNode() const override;
			FileNode root(int streamidx = 0) const override;
			FileNode operator[](const std::string& nodename) const override;
			FileNode operator[](const char* nodename) const override;

			std::string fullName(const std::string &node) const override;
			void nodeWritten() override;
			void startScope(const std::string &name, int type) override;
			void endScope() override;

			void write(const std::string &name, int _data) override;
			void write(const std::string &name, float _data) override;
			void write(const std::string &name, double _data) override;
			void write(const std::string &name, const std::string &_data) override;
			void write(const std::string &name, const cv::Mat &_data) override;
			void write(const std::string &name, const cv::SparseMat &_data) override;
			void write(const std::string &name, const std::vector<int> &_data) override;
			void write(const std::string &name, const std::vector<float> &_data) override;
			void write(const std::string &name, const std::vector<double> &_data) override;
			void write(const std::string &name, const std::vector<std::string> &_data) override;
			void write(const std::string &name, const std::vector<cv::KeyPoint> &_data) override;
			void write(const std::string &name, const std::vector<cv::DMatch> &_data) override;
			void write(const std::string &name, const cv::Range& _data) override;
		};

		class TEXTURIZE_API FileReader {
		protected:
			int							_type;
			std::string					_name;

		public:
			virtual FileNode operator[](const std::string &nodename) const = 0;
			virtual FileNode operator[](const char *nodename) const = 0;
			virtual FileNode operator[](int i)const = 0;

			virtual int  type()   const;
			virtual bool empty()  const;
			virtual bool isNone() const;
			virtual bool isSeq()  const;
			virtual bool isMap()  const;
			virtual bool isInt()  const;
			virtual bool isReal() const;
			virtual bool isString() const;

			virtual std::string name() const;
			virtual size_t size() const = 0;

			virtual operator int() const;
			virtual operator float() const;
			virtual operator double() const;
			virtual operator std::string() const;

			virtual FileNodeIterator begin() const = 0;
			virtual FileNodeIterator end() const = 0;

			virtual bool read(const std::string &name, int &data) const = 0;
			virtual bool read(const std::string &name, float &data) const = 0;
			virtual bool read(const std::string &name, double &data) const = 0;
			virtual bool read(const std::string &name, std::string &data) const = 0;
			virtual bool read(const std::string &name, cv::Mat &data) const = 0;
			virtual bool read(const std::string &name, cv::SparseMat &data)const = 0;
			virtual bool read(const std::string &name, std::vector<int> &data) const = 0;
			virtual bool read(const std::string &name, std::vector<float> &data)const = 0;
			virtual bool read(const std::string &name, std::vector<double> &data)const = 0;
			virtual bool read(const std::string &name, std::vector<std::string> &data) const = 0;
			virtual bool read(const std::string &name, std::vector<cv::KeyPoint> &ks)const = 0;
			virtual bool read(const std::string &name, std::vector<cv::DMatch> &dm)const = 0;
			virtual bool read(const std::string &name, cv::Range& _data)const = 0;

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Point_<_Tp>& _data) const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.x = _tmp[0];
				_data.y = _tmp[1];
				return true;
			}

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Point3_<_Tp>& _data) const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.x = _tmp[0];
				_data.y = _tmp[1];
				_data.y = _tmp[2];
				return true;
			}

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Size_<_Tp>& _data) const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.width = _tmp[0];
				_data.height = _tmp[1];
				return true;
			}

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Complex<_Tp>& _data)const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.re = _tmp[0];
				_data.im = _tmp[1];
				return true;
			}

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Rect_<_Tp>& _data)const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.x = _tmp[0];
				_data.y = _tmp[1];
				_data.width = _tmp[2];
				_data.height = _tmp[3];
				return true;
			}

			template<typename _Tp>
			inline bool read(const std::string &name, cv::Scalar_<_Tp>& _data) const
			{
				std::vector<_Tp> _tmp;
				bool read = read(name, _tmp);
				if (!read)
					return read;
				_data.val[0] = _tmp[0];
				_data.val[1] = _tmp[1];
				_data.val[2] = _tmp[2];
				_data.val[3] = _tmp[3];
				return true;
			}
		};

		class TEXTURIZE_API H5FileReader : public FileReader {
		private:
			friend class H5FileReaderIterator;
			cv::Ptr<H5::H5File>			_fn;
			std::vector<std::string>	_sub;

		public:
			H5FileReader();
			H5FileReader(const cv::Ptr<H5::H5File> &file, const std::string &name);

		public:
			cv::Ptr<H5::H5File>& fn(void) {
				return _fn;
			}

			std::vector<std::string>& sub(void) {
				return _sub;
			}

		public:
			FileNode operator[](const std::string &nodename) const override;
			FileNode operator[](const char *nodename) const override;
			FileNode operator[](int i) const override;

			size_t size() const override;

			FileNodeIterator begin() const override;
			FileNodeIterator end() const override;

			bool read(const std::string &name, int &data) const override;
			bool read(const std::string &name, float &data) const override;
			bool read(const std::string &name, double &data) const override;
			bool read(const std::string &name, std::string &data) const override;
			bool read(const std::string &name, cv::Mat &data) const override;
			bool read(const std::string &name, cv::SparseMat &data) const override;
			bool read(const std::string &name, std::vector<int> &data) const override;
			bool read(const std::string &name, std::vector<float> &data) const override;
			bool read(const std::string &name, std::vector<double> &data) const override;
			bool read(const std::string &name, std::vector<std::string> &data) const override;
			bool read(const std::string &name, std::vector<cv::KeyPoint> &ks) const override;
			bool read(const std::string &name, std::vector<cv::DMatch> &dm) const override;
			bool read(const std::string &name, cv::Range& _data) const override;
		};

		class TEXTURIZE_API FileReaderIterator {
		public:
			virtual void increment(size_t i = 1) = 0;
			virtual void decrement(size_t i = 1) = 0;

			virtual FileNode operator* () const = 0;
			virtual FileNode operator-> () const = 0;
			virtual bool operator== (const FileReaderIterator& rhs) = 0;
			virtual bool operator!= (const FileReaderIterator& rhs) = 0;
			virtual bool operator< (const FileReaderIterator& rhs) = 0;
			virtual ptrdiff_t operator- (const FileReaderIterator& rhs) = 0;
		};

		class TEXTURIZE_API H5FileReaderIterator : public FileReaderIterator {
		private:
			size_t					_pos;
			H5FileReader			_node;

		public:
			H5FileReaderIterator();
			H5FileReaderIterator(const H5FileReader &node, size_t pos);

		public:
			void increment(size_t i = 1) override;
			void decrement(size_t i = 1) override;
			bool operator== (const FileReaderIterator& rhs) override;
			bool operator!= (const FileReaderIterator& rhs) override;
			bool operator< (const FileReaderIterator& rhs) override;
			ptrdiff_t operator- (const FileReaderIterator& rhs) override;
			FileNode operator* () const override;
			FileNode operator-> () const override;
		};
		
		class TEXTURIZE_API FileNode {
		public:
			const static std::string TYPE;
			const static std::string ROWS;
			const static std::string COLS;
			const static std::string MTYPE;
			const static size_t BUFFER = 100;

		public:
			enum Type
			{
				NONE		= 0x00,
				INT			= 0x01,
				REAL		= 0x02,
				FLOAT		= REAL,
				STR			= 0x03,
				STRING		= STR,
				REF			= 0x04,
				SEQ			= 0x05,
				MAP			= 0x06,
				TYPE_MASK	= 0x07,
				FLOW		= 0x08,
				USER		= 0x10,
				EMPTY		= 0x20,
				NAMED		= 0x40
			};

		protected:
			friend TEXTURIZE_API void read(const FileNode &fn, int &_data, int default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, float &_data, float default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, double &_data, double default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::string &_data, const std::string &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Mat &_data, const cv::Mat &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, cv::SparseMat &_data, const cv::SparseMat &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<int> &_data, const std::vector<int> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<float> &_data, const std::vector<float> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<double> &_data, const std::vector<double> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<std::string> &_data, const std::vector<std::string> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<cv::KeyPoint> &_data, const std::vector<cv::KeyPoint> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, std::vector<cv::DMatch> &_data, const std::vector<cv::DMatch> &default_value);
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Range& _data, const cv::Range &default_value);

			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Point_<_Tp>& _data, const cv::Point_<_Tp>& default_value);
			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Point3_<_Tp>& _data, const cv::Point3_<_Tp>& default_value);
			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Size_<_Tp>& _data, const cv::Size_<_Tp>& default_value);
			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Complex<_Tp>& _data, const cv::Complex<_Tp>& default_value);
			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Rect_<_Tp>& _data, const cv::Rect_<_Tp>& default_value);
			template<typename _Tp>
			friend TEXTURIZE_API void read(const FileNode &fn, cv::Scalar_<_Tp>& _data, const cv::Scalar_<_Tp>& default_value);

			cv::Ptr<FileReader>		_fr;

		public:
			FileNode() : 
				_fr(cv::Ptr<FileReader>()) {
			}

			FileNode(const cv::Ptr<FileReader> &fr) : 
				_fr(fr) {
			}

		public:
			virtual FileNode operator[](const std::string &nodename) const;
			virtual FileNode operator[](const char *nodename) const;
			virtual FileNode operator[](int i) const;

			virtual int  type() const;
			virtual bool empty() const;
			virtual bool isNone() const;
			virtual bool isSeq() const;
			virtual bool isMap() const;
			virtual bool isInt() const;
			virtual bool isReal() const;
			virtual bool isString() const;

			virtual std::string name() const;
			virtual size_t size() const;
			virtual operator int() const;
			virtual operator float() const;
			virtual operator double() const;
			virtual operator std::string() const;
			virtual FileNodeIterator begin() const;
			virtual FileNodeIterator end() const;
		};

		class TEXTURIZE_API FileNodeIterator {
		private:
			cv::Ptr<FileReaderIterator> _fri;

		public:
			FileNodeIterator() {
			}

			FileNodeIterator(const cv::Ptr<FileReaderIterator> &it) : 
				_fri(it) {
			}

		public:
			virtual FileNode operator *() const;
			virtual FileNode operator ->() const;
			virtual FileNodeIterator& operator++ ();
			virtual FileNodeIterator operator++ (int);
			virtual FileNodeIterator& operator-- ();
			virtual FileNodeIterator operator-- (int);
			virtual FileNodeIterator& operator+= (int ofs);
			virtual FileNodeIterator& operator-= (int ofs);

			virtual bool operator== (const FileNodeIterator& rhs);
			virtual bool operator!= (const FileNodeIterator& rhs);
			virtual ptrdiff_t operator- (const FileNodeIterator& rhs);
			virtual bool operator< (const FileNodeIterator& rhs);
		};

		class TEXTURIZE_API FileStorage {
		public:
			enum Mode {
				READ = 0,
				WRITE = 1,
				APPEND = 2,
				MEMORY = 4,
				FORMAT_MASK = (7 << 3),
				FORMAT_AUTO = 0,
				FORMAT_XML = (1 << 3),
				FORMAT_YAML = (2 << 3),
				FORMAT_H5 = (3 << 3) 
			};

			enum State {
				UNDEFINED = 0,
				VALUE_EXPECTED = 1,
				NAME_EXPECTED = 2,
				INSIDE_MAP = 4
			};

		protected:
			friend TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const std::string &str);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, int _data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, float _data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, double _data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::string &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Mat &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::SparseMat &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<int> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<float> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<double> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<std::string> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<cv::KeyPoint> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<cv::DMatch> &_data);
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Range& _data);

			template<typename _Tp>
			friend TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const _Tp& value);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Point_<_Tp>& _data);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Point3_<_Tp>& _data);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Size_<_Tp>& _data);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Complex<_Tp>& _data);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Rect_<_Tp>& _data);
			template<typename _Tp>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Scalar_<_Tp>& _data);
			template<typename T>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<T> &data);
			template<typename T>
			friend TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::map<std::string, T> &data);
			
			cv::Ptr<FileWriter>		_fw;

		public:
			FileStorage();
			FileStorage(const std::string &filename, int flags, const std::string &encoding = std::string());

		public:
			virtual bool open(const std::string &filename, int flags, const std::string &encoding = std::string());
			virtual bool isOpened() const;
			virtual void release();
			virtual FileNode getFirstTopLevelNode() const;
			virtual FileNode root(int streamidx = 0) const;
			virtual FileNode operator[](const std::string& nodename) const;
			virtual FileNode operator[](const char* nodename) const;
			virtual std::string fullName(const std::string &node) const;
			virtual void nodeWritten();
			virtual void startScope(const std::string &name, int type);
			virtual void endScope();

			static inline bool cv_isalpha(char c) {
				return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
			}
		};

		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, int _data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, float _data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, double _data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::string &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Mat &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::SparseMat &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<int> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<float> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<double> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<std::string> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<cv::KeyPoint> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<cv::DMatch> &_data);
		TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Range& _data);
		
		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Point_<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Point3_<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Size_<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Complex<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Rect_<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename _Tp>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const cv::Scalar_<_Tp>& _data)
		{
			fs._fw->write<_Tp>(name, _data);
		}

		template<typename T>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::vector<T> &data)
		{
			fs << "[";

			for (size_t i = 0; i < data.size(); i++)
				fs << data[i];

			fs << "]";
		}

		template<typename T>
		inline TEXTURIZE_API void write(FileStorage &fs, const std::string &name, const std::map<std::string, T> &data)
		{
			fs << "{";

			for (typename std::map<std::string, T>::const_iterator it = data.begin(); it != data.end(); it++)
				fs << it->first << it->second;

			fs << "}";
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Point_<_Tp>& _data, const cv::Point_<_Tp>& default_value = cv::Point_<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Point3_<_Tp>& _data, const cv::Point3_<_Tp>& default_value = cv::Point3_<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Size_<_Tp>& _data, const cv::Size_<_Tp>& default_value = cv::Size_<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Complex<_Tp>& _data, const cv::Complex<_Tp>& default_value = cv::Complex<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Rect_<_Tp>& _data, const cv::Rect_<_Tp>& default_value = cv::Rect_<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void read(const FileNode &fn, cv::Scalar_<_Tp>& _data, const cv::Scalar_<_Tp>& default_value = cv::Scalar_<_Tp>())
		{
			if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
				_data = default_value;
		}

		inline TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const std::string &str)
		{
			const char* _str = str.c_str();

			if (!fs.isOpened() || !_str)
				return fs;

			if (*_str == '}' || *_str == ']')
			{
				if (fs._fw->structs.empty())
					TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Unexpected end of struct or vector detected.");

				if ((*_str == ']' ? '[' : '{') != fs._fw->structs.back())
					TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Wrong closing bracket detected. Use \'[\'/\']\' to delimit vectors and \'{\'/\'}\' to delimit structs.");

				fs._fw->structs.pop_back();
				fs._fw->state = (fs._fw->structs.empty() || fs._fw->structs.back() == '{') ?
					FileStorage::INSIDE_MAP + FileStorage::NAME_EXPECTED :
					FileStorage::VALUE_EXPECTED;

				fs.endScope();
				fs._fw->elname = "";
			}
			else if (fs._fw->state == FileStorage::NAME_EXPECTED + FileStorage::INSIDE_MAP)
			{
				if (!FileStorage::cv_isalpha(*_str))
					TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Element names must only contain alphabetical characters.");

				fs._fw->elname = str;
				fs._fw->state = FileStorage::VALUE_EXPECTED + FileStorage::INSIDE_MAP;
			}
			else if ((fs._fw->state & 3) == FileStorage::VALUE_EXPECTED)
			{
				if (*_str == '{' || *_str == '[')
				{
					fs._fw->structs.push_back(*_str);
					int flags = (*_str++ == '{') ? cv::FileNode::MAP : cv::FileNode::SEQ;

					fs._fw->state = (flags == cv::FileNode::MAP) ?
						FileStorage::INSIDE_MAP + FileStorage::NAME_EXPECTED :
						FileStorage::VALUE_EXPECTED;

					if (*_str == ':') {
						flags |= cv::FileNode::FLOW;
						_str++;
					}

					fs.startScope(fs._fw->elname, (flags == cv::FileNode::MAP) ? cv::FileNode::MAP : cv::FileNode::SEQ);
					fs._fw->elname = "";
				}
				else
				{
					std::string _tmp_ = (_str[0] == '\\' && (_str[1] == '{' || _str[1] == '}' || _str[1] == '[' || _str[1] == ']')) ? std::string(_str + 1) : str;
					write(fs, fs.fullName(fs._fw->elname), _tmp_);

					fs.nodeWritten();

					if (fs._fw->state == FileStorage::INSIDE_MAP + FileStorage::VALUE_EXPECTED)
						fs._fw->state = FileStorage::INSIDE_MAP + FileStorage::NAME_EXPECTED;
				}
			}
			else
				TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "Invalid storage state. Make sure the storage is opened.");

			return fs;
		}

		inline TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const char* str)
		{
			return (fs << std::string(str));
		}

		inline TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, char* value)
		{
			return (fs << std::string(value));
		}

		template<typename _Tp>
		inline TEXTURIZE_API FileStorage& operator<< (FileStorage& fs, const _Tp& value)
		{
			if (!fs.isOpened())
				return fs;

			if (fs._fw->state == FileStorage::NAME_EXPECTED + FileStorage::INSIDE_MAP)
				TEXTURIZE_ERROR(TEXTURIZE_ERROR_IO, "No element name has been set.");

			write(fs, fs.fullName(fs._fw->elname), value);
			fs.nodeWritten();

			if (fs._fw->state & FileStorage::INSIDE_MAP)
				fs._fw->state = FileStorage::NAME_EXPECTED + FileStorage::INSIDE_MAP;

			return fs;
		}

		template<typename _Tp>
		inline TEXTURIZE_API FileNodeIterator& operator>> (FileNodeIterator& it, _Tp& value)
		{
			read(*it, value, _Tp());
			return ++it;
		}

		template<typename _Tp>
		inline TEXTURIZE_API void operator>> (const FileNode& n, _Tp& value)
		{
			read(n, value, _Tp());
		}
	}
}