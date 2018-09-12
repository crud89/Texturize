#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);
    
    // Read the uv map, provided by the second command line parameter. Note that uv maps must contain 2 channels.
    cv::Mat uv = cv::imread(argv[1]);
    TEXTURIZE_ASSERT(uv.depth() == 2);

    // Create a sample for the sample and uv map.
    // Note that both images may differ in size.
    Texturize::Sample exemplar(img);
    Texturize::Sample uvMap(uv);

    // Apply the uv map to the image and store the result to a new sample.
    Texturize::Sample result;
    exemplar.sample(uv, result);

    // Display the result. It will have the same size as the uv map and the same depth as the image.
    cv::imshow("Result", (cv::Mat)result);
    cv::waitKey(1);
}