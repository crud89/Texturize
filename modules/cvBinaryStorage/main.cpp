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

#include "ipersistence.hpp"



class MyData
{
public:
    MyData() : A(0), X(0), id()
    {}

    // explicit to avoid implicit conversion
    explicit MyData(int)
        : A(97), X(CV_PI), id("mydata1234")
    {}

    //Write serialization for this class
    //Using cv::Storage instead of cv::FileStorage
    void write(cv2::FileStorage& fs) const
    {
        fs << "{" << "A" << A << "X" << X << "id" << id << "}";
    }

    //Read serialization for this class
    //Using cv::StorageNode instead of cv::FileNode
    void read(const cv2::FileNode& node)
    {
        A = (int)node["A"];
        X = (double)node["X"];
        id = (std::string)node["id"];
    }
    // Data Members
public:
    int A;
    double X;
    std::string id;
};

//These write and read functions must be defined for
//the serialization in FileStorage to work.
//Note the changes cv::Storage and cv::StorageNode
static void write(cv2::FileStorage& fs, const std::string&, const MyData& x)
{
    x.write(fs);
}
static void read(const cv2::FileNode& node, MyData& x,
                    const MyData& default_value = MyData())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}

// This function will print our custom class to the console
static std::ostream& operator<<(std::ostream& out, const MyData& m)
{
    out << "{ id = " << m.id << ", ";
    out << "X = " << m.X << ", ";
    out << "A = " << m.A << "}";
    return out;
}

int main(int ac, char** av)
{


    std::string filename = "example.h5";
    //write
    {
        cv::Mat R = cv::Mat_<uchar>::eye(3, 3) * 32,
        T = cv::Mat_<double>::ones(3, 1)* 10.342 ;
        MyData m(1);

        cv2::FileStorage fs(filename, cv2::FileStorage::WRITE | cv2::FileStorage::FORMAT_H5);

        fs << "iterationNr" << 100;
        // text - string sequence
        fs << "strings" << "[";
        fs << "image1.jpg" << "Awesomeness" << "baboon.jpg";
        // close sequence
        fs << "]";

        // text - mapping
        fs << "Mapping";
        fs << "{" << "One" << 1;
        fs <<        "Two" << 2 << "}";

        // cv::Mat
        fs << "R" << R;
        fs << "T" << T;

        // your own data structures
        fs << "MyData" << m;

        // explicit close
        fs.release();
        std::cout << "Write Done." << std::endl;
    }
    //read
    {
        std::cout << std::endl << "Reading: " << std::endl;
        cv2::FileStorage fs;
        fs.open(filename, cv2::FileStorage::READ | cv2::FileStorage::FORMAT_H5);

        int itNr;
        //fs["iterationNr"] >> itNr;
        itNr = (int) fs["iterationNr"];
        std::cout << itNr;
        if (!fs.isOpened())
        {
            std::cerr << "Failed to open " << filename << std::endl;

            return 1;
        }

        // Read string sequence - Get node
        cv2::FileNode n = fs["strings"];
        if (n.type() != cv2::FileNode::SEQ)
        {
            std::cerr << "strings is not a sequence! FAIL" << std::endl;
            return 1;
        }

        // Go through the node
        cv2::FileNodeIterator it = n.begin(), it_end = n.end();
        for (; it != it_end; ++it)
            std::cout << (std::string)*it << std::endl;

        // Read mappings from a sequence
        n = fs["Mapping"];
        std::cout << "Two  " << (int)(n["Two"]) << "; ";
        std::cout << "One  " << (int)(n["One"]) << std::endl << std::endl;

        
        MyData m;
        cv::Mat R, T;

        // Read cv::Mat
        (cv2::FileNode)fs["R"] >> R;
        fs["T"] >> T;
        // Read your own structure_
        fs["MyData"] >> m;

        std::cout << std::endl << R.cols << " " << R.rows << "R = " << R << std::endl;
        std::cout << "T = " << T << std::endl << std::endl;
        std::cout << "MyData = " << std::endl << m << std::endl << std::endl;

        //Show default behavior for non existing nodes
        std::cout << "Attempt to read NonExisting " <<
                " (should initialize the data structure with its default).";
        
        fs["NonExisting"] >> m;
        std::cout << std::endl << "NonExisting = " << std::endl << m << std::endl;
    }

    std::cout << std::endl
    << "Tip: Open up " << filename <<
    " with a text editor to see the serialized data." << std::endl;

    return 0;
}