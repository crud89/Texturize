#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);

    // Iterate each texel.
    for (int x(0); x < exemplar.width(); ++x)
    for (int y(0); y < exemplar.height(); ++y)
    {
        std::vector<float> texel, neighborhood;

        // Get the texel at the coordinates.
        // Assuming the image has 4 channels, the texel vector will have 4 values.
        exemplar.at(x, y, texel);

        // Get the gaussian-weighted 5 pixel neighborhood at the coordinates.
        // Assuming again, the image has 4 channels, the neighborhood vector will contain 5x5x4 = 100 values.
        exemplar.getNeighborhood(x, y, 5, neighborhood, true);
    }
}