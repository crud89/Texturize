#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);

    // Create a new filter.
    // The filter assumes that the result and sample are equally sized and the result depth is 1 channel.
    // The result will contain the mean value of all sample texel values.
    FunctionFilter filter([](Sample& result, Sample& sample) -> void {
        std::vector<float>& sampleTexel;
        std::vector<float>& resultTexel;
        float mean = 0.f;

        // Iterate each texel of the sample.
        for (int x(0); x < sample.width(); ++x)
        for (int y(0); y < sample.height(); ++y)
        {
            // Get the sample and result texel references.
            sample.at(x, y, sampleTexel);
            result.at(x, y, resultTexel);

            // Calculate the mean of the sample texel values.
            for each (float val in sampleTexel)
                mean += val;

            mean /= sampleTexel.size();

            // Store it to the result texel.
            resultTexel[0] = mean;
        }
    });

    // Apply the filter fuction to the exemplar.
    Texturize::Sample result(1, exemplar.size());
    filter.apply(result, exemplar);

    // Display the result.
    cv::imshow("Result", (cv::Mat)result);
    cv::waitKey(1);
}