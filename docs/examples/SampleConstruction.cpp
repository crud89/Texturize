#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);

    // Create another sample with 1 channel and the sample size as the earlier created instance.
    Texturize::Sample channelA(1, exemplar.size());

    // Copy the alpha channel to the sample.
    exemplar.extract({3, 0}, channelA);

    // Convert the sample back to an cv::Mat.
    cv::Mat alpha = (cv::Mat)channelA;
}