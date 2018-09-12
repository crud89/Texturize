#include <analysis.hpp>

using namespace Texturize;

// ------------------------------------------------------------------------------------------------
// 
// The example demonstrates how to implement a custom filter, that blurs an image.
// 
// ------------------------------------------------------------------------------------------------

class BlurFilter : public IFilter {
public:
    void apply(Sample& result, const Sample& sample) const override;
};

void BlurFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat buffer;
	cv::GaussianBlur((cv::Mat)sample, buffer, cv::Size(5, 5), 0);
	result = Sample(buffer);
}

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Sample exemplar(img);
	
    // Apply the filter fuction to the exemplar.
    Sample result;
	BlurFilter filter;
    filter.apply(result, exemplar);

    // Display the result.
    cv::imshow("Result", (cv::Mat)result);
    cv::waitKey(1);
}