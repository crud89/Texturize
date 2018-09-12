#include <analysis.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // The second command line parameter must contain the edge detector model.
    std::string edgeDetectorModel(argv[1]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);

    // Run an edge detector on the exemplar.
    Texturize::Sample exemplarEdges;
	std::unique_ptr<EdgeDetector> edgeDetector = std::make_unique<StructuredEdgeDetector>(edgeDetectorModel);
	edgeDetector->apply(exemplarEdges, exemplar);

    // Calculate the feature distances that are used as guidance.
    Texturize::Sample exemplarFeatures;
	std::unique_ptr<FeatureExtractor> featureExtractor = std::make_unique<FeatureExtractor>();
	featureExtractor->apply(exemplarFeatures, exemplarEdges);

    // Calculate the appearance space. Neighborhoods will retain a dimensionality of 4, which is a suitable choice for RGB(A) exemplars.
	Texturize::AppearanceSpace* searchSpace;
	Texturize::AppearanceSpace::calculate({ exemplarFeatures, exemplar}, &searchSpace, 4);

    // It is recommended to use smart pointers to manage search space references.
	std::unique_ptr<AppearanceSpace> descriptors(searchSpace);

    // Display the result.
    // Note: Since the appearance space uses principal component analysis (PCA) to transform pixel neighborhoods, the result might be hard
    //       to interpret for human observers. Also the retained dimensionality does not need to equal 4, which makes this visualization a
    //       special case. Note that it may differ for different input exemplars!  
    Texturize::Sample transformedExemplar;
    descriptors->sample(transformedExemplar);
    cv::imshow("Appearance Space", transformedExemplar);
    cv::waitKey(1);
}