/**************************************************************************************************
 **************************************************************************************************

 BSD 3-Clause License (https://www.tldrlegal.com/l/bsd3)

 Copyright (c) 2016 Andrés Solís Montero <http://www.solism.ca>, All rights reserved.


 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 **************************************************************************************************
 **************************************************************************************************/


#ifndef __OPENCV_CORE_iPERSISTENCE_HPP__
#define __OPENCV_CORE_iPERSISTENCE_HPP__

#include <map>
#include <iostream>
#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>


#pragma warning(push)
#pragma warning(disable: 4251)
#pragma warning(disable: 4267)
#include "H5Cpp.h"
#pragma warning(pop)


namespace cv2
{

    class FileNode;
    class FileNodeIterator;
    /********** Interfaces to Implement for different Storage solutions *******/
	class FileWriter
    {
    public:
        virtual bool open(const std::string &filename,
                          int flags,
                          const std::string &encoding=std::string()) = 0;
        virtual bool isOpened() const = 0;
        virtual void release() = 0;
        virtual FileNode getFirstTopLevelNode() const = 0;
        virtual FileNode root(int streamidx=0) const = 0;
        virtual FileNode operator[](const std::string& nodename) const = 0;
        virtual FileNode operator[](const char* nodename) const =0;

        virtual std::string fullName(const std::string &node) const = 0;
        virtual void nodeWritten()=0;
        virtual void startScope(const std::string &name, int type)=0;
        virtual void endScope()=0;

        /* writing functions for basic and OpenCV types */
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
        inline void write(const std::string &name,  const cv::Rect_<_Tp>& _data)
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

        int  state;
        std::string elname;
        std::vector<char> structs;
    };
    class FileReader
    {
    public:
        int _type;
        std::string     _name;
        virtual ~FileReader(){}
        virtual FileNode operator[](const std::string &nodename) const =0;
        virtual FileNode operator[](const char *nodename) const =0;
        virtual FileNode operator[](int i)const =0;

        virtual int  type()   const;
        virtual bool empty()  const;
        virtual bool isNone() const;
        virtual bool isSeq()  const;
        virtual bool isMap()  const;
        virtual bool isInt()  const;
        virtual bool isReal() const;
        virtual bool isString() const;

        virtual std::string name() const;
        virtual size_t size() const=0;

        virtual operator int() const;
        virtual operator float() const;
        virtual operator double() const;
        virtual operator std::string() const;

        virtual FileNodeIterator begin() const=0;
        virtual FileNodeIterator end() const=0;

        virtual bool read(const std::string &name, int &data) const =0;
        virtual bool read(const std::string &name, float &data) const =0;
        virtual bool read(const std::string &name, double &data) const =0;
        virtual bool read(const std::string &name, std::string &data) const =0;
        virtual bool read(const std::string &name, cv::Mat &data) const =0;
        virtual bool read(const std::string &name, cv::SparseMat &data)const =0;
        virtual bool read(const std::string &name, std::vector<int> &data) const =0;
        virtual bool read(const std::string &name, std::vector<float> &data)const =0;
        virtual bool read(const std::string &name, std::vector<double> &data)const =0;
        virtual bool read(const std::string &name, std::vector<std::string> &data) const =0;
        virtual bool read(const std::string &name, std::vector<cv::KeyPoint> &ks)const =0;
        virtual bool read(const std::string &name, std::vector<cv::DMatch> &dm)const =0;
        virtual bool read(const std::string &name, cv::Range& _data)const =0;

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
            _data.width  = _tmp[0];
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
            _data.re  = _tmp[0];
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
            _data.x  = _tmp[0];
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
    class FileReaderIterator
    {
    public:
        virtual void increment(size_t i = 1) = 0;
        virtual void decrement(size_t i = 1) = 0;
        //! returns the currently observed element
        virtual FileNode operator *() const =0;
        //! accesses the currently observed element methods
        virtual FileNode operator ->() const =0;
        //! compares if two nodes are pointing to the same element
        virtual bool operator == (const FileReaderIterator& rhs) = 0;
        //! compares if two nodes are not pointing to the same element
        virtual bool operator != (const FileReaderIterator& rhs)
        {
            return !(*this == rhs);
        }
        //! compares the order of two iterator positions
        virtual bool operator < (const FileReaderIterator& rhs) = 0;
        //! pointer aritmetic between two nodes
        virtual ptrdiff_t operator- (const FileReaderIterator& rhs)=0;
    };
    /*********** OpenCV Interface for File Storage ***************/
    class FileNodeIterator
    {
        cv::Ptr<FileReaderIterator> _fri;
    public:
        FileNodeIterator(){};
        FileNodeIterator(const cv::Ptr<FileReaderIterator> &it): _fri(it){};
        //! returns the currently observed element
        virtual FileNode operator *() const;
        //! accesses the currently observed element methods
        virtual FileNode operator ->() const;
        //! moves iterator to the next node
        virtual FileNodeIterator& operator ++ ();
        //! moves iterator to the next node
        virtual FileNodeIterator operator ++ (int);
        //! moves iterator to the previous node
        virtual FileNodeIterator& operator -- ();
        //! moves iterator to the previous node
        virtual FileNodeIterator operator -- (int);
        //! moves iterator forward by the specified offset (possibly negative)
        virtual FileNodeIterator& operator += (int ofs);
        //! moves iterator backward by the specified offset (possibly negative)
        virtual FileNodeIterator& operator -= (int ofs);

        //! compares if two nodes are pointing to the same element
        virtual bool operator == (const FileNodeIterator& rhs);
        //! compares if two nodes are not pointing to the same element
        virtual bool operator != (const FileNodeIterator& rhs);
        //! pointer aritmetic between two nodes
        virtual ptrdiff_t operator- (const FileNodeIterator& rhs);
        //! compares the order of two iterator positions
        virtual bool operator < (const FileNodeIterator& rhs);
    };
    class FileNode
    {
    public:
        cv::Ptr<FileReader> _fr;

        FileNode():_fr(cv::Ptr<FileReader>()){}
        FileNode(const cv::Ptr<FileReader> &fr):_fr(fr){}

        const static std::string TYPE;
        const static std::string ROWS;
        const static std::string COLS;
        const static std::string MTYPE;
        const static size_t BUFFER  = 100;

        enum Type
        {
            NONE      = 0, //!< empty node
            INT       = 1, //!< an integer
            REAL      = 2, //!< floating-point number
            FLOAT     = REAL, //!< synonym or REAL
            STR       = 3, //!< text string in UTF-8 encoding
            STRING    = STR, //!< synonym for STR
            REF       = 4, //!< integer of size size_t. Typically used for storing complex dynamic structures where some elements reference the others
            SEQ       = 5, //!< sequence
            MAP       = 6, //!< mapping
            TYPE_MASK = 7,
            FLOW      = 8,  //!< compact representation of a sequence or mapping.
            USER      = 16, //!< a registered object (e.g. a matrix)
            EMPTY     = 32, //!< empty structure (sequence or mapping)
            NAMED     = 64  //!< the node has a name (i.e. it is element of a mapping)
        };

        virtual FileNode operator[](const std::string &nodename) const;
        virtual FileNode operator[](const char *nodename) const;
        virtual FileNode operator[](int i)const;
        virtual int  type()   const;
        virtual bool empty()  const;
        virtual bool isNone() const;
        virtual bool isSeq()  const;
        virtual bool isMap()  const;
        virtual bool isInt()  const;
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
    class FileStorage
    {
    public:
        cv::Ptr<FileWriter> _fw;

        FileStorage();
        FileStorage(const std::string &filename,
                    int flags,
                    const std::string &encoding=std::string());
        enum Mode
        {
            READ        = 0, //!< value, open the file for reading
            WRITE       = 1, //!< value, open the file for writing
            APPEND      = 2, //!< value, open the file for appending
            MEMORY      = 4, //!< flag, read data from source or write data to the internal buffer (which is
            //!< returned by FileStorage::release)
            FORMAT_MASK = (7<<3), //!< mask for format flags
            FORMAT_AUTO = 0,      //!< flag, auto format
            FORMAT_XML  = (1<<3), //!< flag, XML format
            FORMAT_YAML = (2<<3), //!< flag, YAML format
            FORMAT_H5   = (3<<3)  //!< flag, HDF5 format
        };
        enum
        {
            UNDEFINED      = 0,
            VALUE_EXPECTED = 1,
            NAME_EXPECTED  = 2,
            INSIDE_MAP     = 4
        };
        virtual bool open(const std::string &filename,
                          int flags,
                          const std::string &encoding=std::string());
        virtual bool isOpened() const;
        virtual void release();
        virtual FileNode getFirstTopLevelNode() const;
        virtual FileNode root(int streamidx=0) const;
        virtual FileNode operator[](const std::string& nodename) const;
        virtual FileNode operator[](const char* nodename) const;
        virtual std::string fullName(const std::string &node) const;
        virtual void nodeWritten();
        virtual void startScope(const std::string &name, int type);
        virtual void endScope();
        
        static inline bool cv_isalpha(char c)
        {
            return ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z');
        }
    };
    /*********** Write and Read functions for basic and OpenCV types ******/
    void write(cv2::FileStorage &fs, const std::string &name, int _data);
    void write(cv2::FileStorage &fs, const std::string &name, float _data);
    void write(cv2::FileStorage &fs, const std::string &name, double _data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::string &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const cv::Mat &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const cv::SparseMat &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<int> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<float> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<double> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<std::string> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<cv::KeyPoint> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const std::vector<cv::DMatch> &_data);
    void write(cv2::FileStorage &fs, const std::string &name, const cv::Range& _data);

    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name, const cv::Point_<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }
    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name, const cv::Point3_<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }
    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name, const cv::Size_<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }
    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name, const cv::Complex<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }
    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name,  const cv::Rect_<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }
    template<typename _Tp>
    inline void write(cv2::FileStorage &fs, const std::string &name, const cv::Scalar_<_Tp>& _data)
    {
        fs._fw->write<_Tp>(name, _data);
    }

    void read(const FileNode &fn, int &_data, int default_value);
    void read(const FileNode &fn, float &_data, float default_value);
    void read(const FileNode &fn, double &_data, double default_value);
    void read(const FileNode &fn, std::string &_data, const std::string &default_value);
    void read(const FileNode &fn, cv::Mat &_data, const cv::Mat &default_value = cv::Mat());
    void read(const FileNode &fn, cv::SparseMat &_data, const cv::SparseMat &default_value = cv::SparseMat());
    void read(const FileNode &fn, std::vector<int> &_data, const std::vector<int> &default_value);
    void read(const FileNode &fn, std::vector<float> &_data, const std::vector<float> &default_value);
    void read(const FileNode &fn, std::vector<double> &_data, const std::vector<double> &default_value);
    void read(const FileNode &fn, std::vector<std::string> &_data, const std::vector<std::string> &default_value);
    void read(const FileNode &fn, std::vector<cv::KeyPoint> &_data, const std::vector<cv::KeyPoint> &default_value);
    void read(const FileNode &fn, std::vector<cv::DMatch> &_data, const std::vector<cv::DMatch> &default_value);
    void read(const FileNode &fn, cv::Range& _data, const cv::Range &default_value);

    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Point_<_Tp>& _data, const cv::Point_<_Tp>& default_value = cv::Point_<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }
    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Point3_<_Tp>& _data, const cv::Point3_<_Tp>& default_value = cv::Point3_<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }
    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Size_<_Tp>& _data, const cv::Size_<_Tp>& default_value = cv::Size_<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }
    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Complex<_Tp>& _data, const cv::Complex<_Tp>& default_value = cv::Complex<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }
    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Rect_<_Tp>& _data, const cv::Rect_<_Tp>& default_value = cv::Rect_<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }
    template<typename _Tp>
    inline void read(const FileNode &fn, cv::Scalar_<_Tp>& _data, const cv::Scalar_<_Tp>& default_value = cv::Scalar_<_Tp>())
    {
        if (!fn._fr->read<_Tp>(fn._fr->_name, _data))
            _data = default_value;
    }



    /********** Operators for FileStorage , FileNode and FileNodeIterator **********/
    inline cv2::FileStorage& operator<< (cv2::FileStorage& fs, const std::string &str)
    {
        const char* _str = str.c_str();

        if( !fs.isOpened() || !_str )
            return fs;

        if( *_str == '}' || *_str == ']' )
        {
            if( fs._fw->structs.empty() )
                CV_Error_( CV_StsError, ("Extra closing '%c'", *_str) );

            if( (*_str == ']' ? '[' : '{') != fs._fw->structs.back() )
                CV_Error_( CV_StsError,
                          ("The closing '%c' does not match the opening '%c'",
                           *_str, fs._fw->structs.back()));

            fs._fw->structs.pop_back();
            fs._fw->state = (fs._fw->structs.empty() ||
                        fs._fw->structs.back() == '{' )?
            cv2::FileStorage::INSIDE_MAP + cv2::FileStorage::NAME_EXPECTED :
            cv2::FileStorage::VALUE_EXPECTED;

            //cvEndWriteStruct
            fs.endScope();

            fs._fw->elname = "";
        }
        else if( fs._fw->state == cv2::FileStorage::NAME_EXPECTED +
                cv2::FileStorage::INSIDE_MAP )
        {

            if( !cv2::FileStorage::cv_isalpha(*_str) )
                CV_Error_( CV_StsError, ("Incorrect element name %s", _str) );
            fs._fw->elname = str;
            fs._fw->state = cv2::FileStorage::VALUE_EXPECTED +
            cv2::FileStorage::INSIDE_MAP;
        }
        else if( (fs._fw->state & 3) == cv2::FileStorage::VALUE_EXPECTED )
        {
            if( *_str == '{' || *_str == '[' )
            {
                fs._fw->structs.push_back(*_str);
                int flags = (*_str++ == '{')? CV_NODE_MAP : CV_NODE_SEQ;

                fs._fw->state = (flags == CV_NODE_MAP) ?
                cv2::FileStorage::INSIDE_MAP + cv2::FileStorage::NAME_EXPECTED :
                cv2::FileStorage::VALUE_EXPECTED;

                if( *_str == ':' )
                {
                    flags |= CV_NODE_FLOW;
                    _str++;
                }
                //cvStartWriteStruct
                fs.startScope(fs._fw->elname, (flags == CV_NODE_MAP)? cv2::FileNode::MAP :
                              cv2::FileNode::SEQ);
                fs._fw->elname = "";
            }
            else
            {
                std::string _tmp_ = (_str[0] == '\\' &&
                                     (_str[1] == '{' || _str[1] == '}' ||
                                      _str[1] == '[' || _str[1] == ']')) ?
                std::string(_str+1) : str;

                write( fs, fs.fullName(fs._fw->elname), _tmp_);
                
                fs.nodeWritten();
                
                if( fs._fw->state == cv2::FileStorage::INSIDE_MAP +
                   cv2::FileStorage::VALUE_EXPECTED )
                    fs._fw->state = cv2::FileStorage::INSIDE_MAP +
                    cv2::FileStorage::NAME_EXPECTED;
            }
        }
        else
            CV_Error( CV_StsError, "Invalid fs.state" );
        return fs;
    }
    inline cv2::FileStorage& operator << (cv2::FileStorage& fs, const char* str)
    {
        return (fs << std::string(str));
    }
    inline cv2::FileStorage& operator << (cv2::FileStorage& fs, char* value)
    {
        return (fs << std::string(value));
    }
    template<typename _Tp>
    inline cv2::FileStorage& operator << (cv2::FileStorage& fs, const _Tp& value)
    {
        if( !fs.isOpened() )
            return fs;
        if( fs._fw->state == cv2::FileStorage::NAME_EXPECTED + cv2::FileStorage::INSIDE_MAP )
            CV_Error(CV_StsError, "No element name has been given" );
        //write( fs, fs.elname, value );
        write(fs, fs.fullName(fs._fw->elname), value);
        fs.nodeWritten();
        if( fs._fw->state & cv2::FileStorage::INSIDE_MAP )
            fs._fw->state = cv2::FileStorage::NAME_EXPECTED +
            cv2::FileStorage::INSIDE_MAP;
        return fs;
    }
    template<typename T>
    static inline void write(cv2::FileStorage &fs, const std::string &name, const std::vector<T> &data)
    {
        fs << "[" ;
        for (size_t i = 0; i < data.size(); i++)
            fs << data[i];
        fs << "]";
    }
    template<typename T>
    static inline void write(cv2::FileStorage &fs, const std::string &name, const std::map<std::string,T> &data)
    {
        fs << "{";
        for(typename std::map<std::string, T>::const_iterator it = data.begin();
            it != data.end(); it++)
            fs << it->first << it->second;
        fs << "}";
    }
    template<typename _Tp> static inline
    cv2::FileNodeIterator& operator >> (cv2::FileNodeIterator& it, _Tp& value)
    {
        read( *it, value, _Tp());
        return ++it;
    }
    //    /** @brief Reads data from a file storage.
    //     */
    //    template<typename _Tp> static inline
    //    cv::NodeIterator& operator >> (cv::NodeIterator& it, std::vector<_Tp>& vec)
    //    {
    //        cv::NodeIterator it2 = it;
    //        _Tp element;
    //        read(*it, element, _Tp());
    //
    //        while (it2 != it++)
    //        {
    //            read(*it, element, _Tp());
    //            it2 = it;
    //        }
    //        return it;
    //    }
    /** @brief Reads data from a file storage.
     */
    template<typename _Tp> static inline
    void operator >> (const cv2::FileNode& n, _Tp& value)
    {
        read(n, value, _Tp());
    }
    //    /** @brief Reads data from a file storage.
    //     */
    //    template<typename _Tp> static inline
    //    void operator >> (const cv::StorageNode& n, std::vector<_Tp>& vec)
    //    {
    //        cv::NodeIterator it = n.begin();
    //        it >> vec;
    //    }

    /************ HDF5 Implementation ********************/
    class H5Utils
    {
    public:
        //HDF5 Attribute functions
        static bool getAttribute(H5::H5Object &node, const H5::DataType &type,
                                 const std::string &name, void *value);
        static void setAttribute(H5::H5Object &node, const H5::DataType &type,
                                 const std::string &name, const void *value);
        static void setStrAttribute(H5::H5Object &node,
                                    const std::string &name, const char *value);
        static void setStrAttribute(H5::H5Object &node,
                                    const std::string &name, const std::string &value);
        static void setIntAttribute(H5::H5Object &node,
                                    const std::string &name, const int &num);
        static bool getNodeType(H5::H5Object &node, int &value);
        static void setNodeType(H5::H5Object &node, int type);

        //HDF5 Group functions
        static void setGroupAttribute(H5::H5File &file, const std::string &gname,
                                      const std::string &attr, int value);
        static void createGroup(H5::H5File &file, const std::string &name, int type);
        //HDF5 helper functions to list subnodes
        static bool sortSequenceDatasets(const std::string &a, const std::string &b);
        static herr_t readH5NodeInfo(hid_t loc_id, const char *name, const H5L_info_t *linfo, void *opdata);
        static void listSubnodes(H5::H5Object &node, std::vector<std::string> &subnodes);
        static void getNode(H5::H5File &file, const std::string &name, H5::H5Object &node);

        //HDF5 Write basic and OpenCV types
        static void writeDataset1D(H5::H5File &file, const H5::DataType &type,
                                   const std::string &name, const void *data,
                                   const std::map<std::string,int> &attr, size_t count);
        //HDF5 Read
        template<typename _Tp>
        static bool readDataset1D(const H5::H5File &file, const std::string &name, std::vector<_Tp> &data)
        {
            H5::DataSet dataset = file.openDataSet(name);
            H5::DataSpace dataspace = dataset.getSpace();
            hsize_t dims_out[1];
            int rank = dataspace.getSimpleExtentDims( dims_out, NULL);

            int _type;
            bool read = getNodeType(dataset,  _type);
            read &= (_type == cv2::FileNode::SEQ);
            read &= (rank  == 1);

            if (!read)
                return read;
            data.resize(dims_out[0]);
            dataset.read(data.data(), dataset.getDataType());
            return true;
        }
        static bool readDataset(const H5::H5File &file, const std::string &name, int type, void *data);
    };
    class H5FileWriter: public FileWriter
    {
    public:
        bool opened;
        cv::Ptr<H5::H5File> _bfs;
        std::vector<std::string> groups;
        std::vector<size_t> counter;

        H5FileWriter();
        H5FileWriter(const std::string &filename, int flags);
        virtual ~H5FileWriter();
        virtual bool open(const std::string &filename,
                          int flags,
                          const std::string &encoding=std::string());
        virtual bool isOpened() const;
        virtual void release();
        virtual FileNode getFirstTopLevelNode() const;
        virtual FileNode root(int streamidx=0) const;
        virtual FileNode operator[](const std::string& nodename) const;
        virtual FileNode operator[](const char* nodename) const;

        virtual std::string fullName(const std::string &node) const;
        virtual void nodeWritten();
        virtual void startScope(const std::string &name, int type);
        virtual void endScope();

        virtual void write(const std::string &name, int _data);
        virtual void write(const std::string &name, float _data);
        virtual void write(const std::string &name, double _data);
        virtual void write(const std::string &name, const std::string &_data);
        virtual void write(const std::string &name, const cv::Mat &_data);
        virtual void write(const std::string &name, const cv::SparseMat &_data);
        virtual void write(const std::string &name, const std::vector<int> &_data);
        virtual void write(const std::string &name, const std::vector<float> &_data);
        virtual void write(const std::string &name, const std::vector<double> &_data);
        virtual void write(const std::string &name, const std::vector<std::string> &_data);
        virtual void write(const std::string &name, const std::vector<cv::KeyPoint> &_data);
        virtual void write(const std::string &name, const std::vector<cv::DMatch> &_data);
        virtual void write(const std::string &name, const cv::Range& _data);
    };
    class H5FileReader: public FileReader
    {
    public:
        cv::Ptr<H5::H5File> _fn;
        std::vector<std::string> _sub;

        H5FileReader();
        H5FileReader(const cv::Ptr<H5::H5File> &file, const std::string &name);
        virtual ~H5FileReader(){}
        virtual FileNode operator[](const std::string &nodename) const;
        virtual FileNode operator[](const char *nodename) const;
        virtual FileNode operator[](int i)const;

        virtual size_t size() const;

        virtual FileNodeIterator begin() const;
        virtual FileNodeIterator end() const;

        virtual bool read(const std::string &name, int &data)const;
        virtual bool read(const std::string &name, float &data)const;
        virtual bool read(const std::string &name, double &data)const;
        virtual bool read(const std::string &name, std::string &data)const;
        virtual bool read(const std::string &name, cv::Mat &data)const;
        virtual bool read(const std::string &name, cv::SparseMat &data)const;
        virtual bool read(const std::string &name, std::vector<int> &data)const;
        virtual bool read(const std::string &name, std::vector<float> &data)const;
        virtual bool read(const std::string &name, std::vector<double> &data)const;
        virtual bool read(const std::string &name, std::vector<std::string> &data)const;
        virtual bool read(const std::string &name, std::vector<cv::KeyPoint> &ks)const;
        virtual bool read(const std::string &name, std::vector<cv::DMatch> &dm)const;
        virtual bool read(const std::string &name, cv::Range& _data)const;
    };
    class H5FileReaderIterator: public FileReaderIterator
    {
    private:
        size_t _pos;
        H5FileReader _node;
    public:
        H5FileReaderIterator();
        H5FileReaderIterator(const H5FileReader &node, size_t pos);
        virtual void increment(size_t i = 1);
        virtual void decrement(size_t i = 1);
        virtual bool operator == (const FileReaderIterator& rhs);
        //! compares the order of two iterator positions
        virtual bool operator < (const FileReaderIterator& rhs);
        virtual ptrdiff_t operator- (const FileReaderIterator& rhs);
        //! returns the currently observed element
        virtual FileNode operator *() const;
        //! accesses the currently observed element methods
        virtual FileNode operator ->() const;
    };

    class XMLYMLFileReaderIterator: public FileReaderIterator
    {
    public:
        XMLYMLFileReaderIterator();
        XMLYMLFileReaderIterator(const CvFileStorage* fs, const CvFileNode* node, size_t ofs=0);
        XMLYMLFileReaderIterator(const FileNodeIterator& it);

        virtual void increment(size_t i = 1);
        virtual void decrement(size_t i = 1);
        virtual bool operator == (const FileReaderIterator& rhs);
        //! compares the order of two iterator positions
        virtual bool operator < (const FileReaderIterator& rhs);
        virtual ptrdiff_t operator- (const FileReaderIterator& rhs);
        //! returns the currently observed element
        virtual FileNode operator *() const;
        //! accesses the currently observed element methods
        virtual FileNode operator ->() const;


        struct SeqReader
        {
            int          header_size;
            void*        seq;        /* sequence, beign read; CvSeq      */
            void*        block;      /* current block;        CvSeqBlock */
            schar*       ptr;        /* pointer to element be read next */
            schar*       block_min;  /* pointer to the beginning of block */
            schar*       block_max;  /* pointer to the end of block */
            int          delta_index;/* = seq->first->start_index   */
            schar*       prev_elem;  /* pointer to previous element */
        };

        const CvFileStorage* fs;
        const CvFileNode* container;
        SeqReader reader;
        size_t remaining;

    };


}

#endif