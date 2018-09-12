#include "stdafx.h"

#ifdef _DEBUG
#pragma comment(lib, "opencv_core330d.lib")
#pragma comment(lib, "opencv_imgproc330d.lib")
#pragma comment(lib, "opencv_ximgproc330d.lib")
#pragma comment(lib, "opencv_highgui330d.lib")
#pragma comment(lib, "opencv_imgcodecs330d.lib")
#pragma comment(lib, "opencv_flann330d.lib")
#pragma comment(lib, "opencv_features2d330d.lib")
#else
#pragma comment(lib, "opencv_core330.lib")
#pragma comment(lib, "opencv_imgproc330.lib")
#pragma comment(lib, "opencv_ximgproc330.lib")
#pragma comment(lib, "opencv_highgui330.lib")
#pragma comment(lib, "opencv_imgcodecs330.lib")
#pragma comment(lib, "opencv_flann330.lib")
#pragma comment(lib, "opencv_features2d330.lib")
#endif