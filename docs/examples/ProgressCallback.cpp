#include <sampling.hpp>

int main(int argc, const char** argv)
{
    // Read an image using OpenCV. The name of the image is provided within the first command line parameter.
    cv::Mat img = cv::imread(argv[0]);

    // Create a sample that contains the image.
    Texturize::Sample exemplar(img);
    int width = exemplar.width();
    int height = exemplar.height();
	int depth = log2(width);

    // Calculate the appearance space. Neighborhoods will retain a dimensionality of 4, which is a suitable choice for RGB(A) exemplars.
	Texturize::AppearanceSpace* searchSpace;
	Texturize::AppearanceSpace::calculate(exemplar, &searchSpace, 4);

    // It is recommended to use smart pointers to manage search space references.
	std::unique_ptr<Texturize::AppearanceSpace> descriptors(searchSpace);

    // Index the neighborhood descriptors. Neighborhood matching is performed based on random march.
	Texturize::RandomWalkIndex index(descriptors.get());

    // Setup a parallel pyramid synthesizer. The synthesizer is configured with a constant randomness of 0.1, a 5 pixel kernel and a constant seed.
	auto synthesizer = Texturize::ParallelPyramidSynthesizer::createSynthesizer(&index);
	Texturize::PyramidSynthesisSettings config(width, cv::Point2f(0.f, 0.f), 0.1f, 5, 0);
	
    // Setup a progress handler, that prints information to the console and displays the current sample.
	config._progressHandler.add([&exemplar, depth](int level, int pass, const cv::Mat& uv) -> void {
		if (pass == -1)
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass skipped)" << std::endl;
		else
			std::cout << "Executed level " << level + 1 << "/" << depth << " (Correction pass " << pass + 1 << ")" << std::endl;

        // Display the current sample.
        Texturize::Sample result;
        exemplar.sample(uv, result);
        cv::imshow("Current Sample", (cv::Mat)result);
        cv::waitKey(1);
	});

    // Start synthesis.
    Texturize::Sample result;
	synthesizer->synthesize(width, height, result, config);
}