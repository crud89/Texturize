#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);

    // Calculate the appearance space. Neighborhoods will retain a dimensionality of 4, which is a suitable choice for RGB(A) exemplars.
	Texturize::AppearanceSpace* searchSpace;
	Texturize::AppearanceSpace::calculate(exemplar, &searchSpace, 4);

    // It is recommended to use smart pointers to manage search space references.
	std::unique_ptr< Texturize::AppearanceSpace > descriptors(searchSpace);

    // Display the result.
    // Note: Since the appearance space uses principal component analysis (PCA) to transform pixel neighborhoods, the result might be hard
    //       to interpret for human observers. Also the retained dimensionality does not need to equal 4, which makes this visualization a
    //       special case. Note that it may differ for different input exemplars!  
    Texturize::Sample transformedExemplar;
    descriptors->sample(transformedExemplar);
    cv::imshow("Appearance Space", (cv::Mat)transformedExemplar);
    cv::waitKey(1);
}