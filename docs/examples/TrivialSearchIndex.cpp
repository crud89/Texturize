#include <sampling.hpp>
#include <queue>

using namespace Texturize;

// ------------------------------------------------------------------------------------------------
// 
// The example demonstrates how to implement a trivial search index, that matches pixel 
// neighborhoods by comparing their pixel values.
// 
// ------------------------------------------------------------------------------------------------

class TrivialSearchIndex : public SearchIndex {
private:
    cv::Mat _exemplarDescriptors;
    cv::NormTypes _normType;

    // Constructor
public:
    TrivialSearchIndex(const ISearchSpace* searchSpace, cv::NormTypes distanceMeasure = cv::NORM_L2SQR);

private:
    void init();

    // ISearchIndex interface
public:
    bool findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const override;
    bool findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const override;
};

// ------------------------------------------------------------------------------------------------
// 
// Construction
// 
// ------------------------------------------------------------------------------------------------

TrivialSearchIndex::TrivialSearchIndex(const ISearchSpace* searchSpace, cv::NormTypes distanceMeasure = cv::NORM_L2SQR) :
    SearchIndex(searchSpace), _normType(distanceMeasure) {
}

void TrivialSearchIndex::init() {
	// Precompute the neighborhood descriptors used to train data.
	const Sample* sample;
	this->getSearchSpace()->sample(&sample);

	// Form a descriptor vector from the sample.
	_exemplarDescriptors = DescriptorExtractor::indexNeighborhoods(*sample);
}

// ------------------------------------------------------------------------------------------------
// 
// ISearchIndex interface
// 
// ------------------------------------------------------------------------------------------------

bool TrivialSearchIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist, float* dist) const {
    // This method handles the special case of the k nearest neighbor search, where k equals 1.
    std::vector<cv::Vec2f> coordinates;
    std::vector<float> distances;
    
    bool result = this->findNearestNeighbors(descriptors, uv, at, coordinates, 1, minDist, &distances);

    // In case the search was successfull, return the result.
    if (result) {
        match = coordinates[0];

        if (dist != nullptr)
            *dist = distances[0];
    }

    return result;
}

// The descriptors are a set of pre-calculated neighborhood descriptors, that are passed by the
// synthesizer, which calculates them from the current sample for each synthesis pass. The uv 
// parameter contains the uv map for the current sample.
bool TrivialSearchIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k, float minDist, std::vector<float>* dist) const {
    // Results are handled by putting them into a priority queue, where the similarity represents
    // the priority. A smaller similarity value equals a higher priority. The queue has a capacity
    // of k, i.e. only the k most similar candidates are kept within the queue, whilst the others
    // will be discarded. To do this, the following trait is used to provide the comparison logic 
    // between candidates.
    // 
    // Note, that a priority queue is not the most efficient way of solving this problem, since the
    // loop needs to perform many inefficient pops. It is, however, a straightforward and simple
    // to follow approach.
    struct TakeMoreSimilarCandidate {
        bool operator() (const TMatch& lhs, const TMatch& rhs) const {
            return std::get<1>(lhs) < std::get<1>(rhs);
        }
    };

    std::priority_queue<TMatch, std::vector<TMatch>, TakeMoreSimilarCandidate> candidates();

	// Get the target descriptor in order to calculate the similarity later on. A descriptor matrix
    // contains in each row a descriptor. Each column contains a descriptor value. The matrix 
    // itself is a flat array of vectors, that contains as many rows, as there are sample pixels.
    // The target descriptor matrix thus contains as many rows, as there are pixels in the uv map,
    // while the exemplar _exemplarDescriptors matrix, contains as many rows as there are exemplar 
    // pixels. The descriptor row can thus be calculated from y * width + x.
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);
    
	// Get the search space transformed exemplar and some of its properties.
	const Sample* exemplar;
    int kernel, width, height;
	this->getSearchSpace()->sample(&exemplar);
    this->getSearchSpace()->kernel(kernel);
    width = exemplar->width();
    height = exemplar->height();

	// Compare each exemplar pixel to the descriptor.
	for (int x(0); x < width; ++x)
	for (int y(0); y < height; ++y)
	{
        // Get the distance between the exemplar pixel and the currently checked pixel. In case its
        // is too small, skip the pixel. This increases visual variety by forcing candidates to not
        // be direct neighbors of the original pixel.
        cv::Vec2f originalPos = uv.at<cv::Vec2f>(at);
		cv::Vec2f candidatePos = cv::Vec2f(static_cast<float>(x) / static_cast<float>(width), static_cast<float>(y) / static_cast<float>(height));

        if (cv::norm(originalPos - candidatePos) < minDist)
            continue;

        // Get the exemplar neighborhood and calculate its similarity to the target descriptor.
        std::vector<float> exemplarNeighborhood = _exemplarDescriptors.row(y * width + x);
        float similarity = cv::norm(targetDescriptor, exemplarDescriptor, _normType);

		// Store the current candidate.
	    TMatch candidate(candidatePos, similarity);
		candidates.push(candidate);

        // Discard the most dissimilar candidate.
        if (candidates.length() > k)
            candidates.pop_back();
	}

	// Return the best candidate matches.
    while (!candidates.empty()) {
        TMatch candidate = candidates.pop();
        matches.push_back(std::get<0>(candidate));

        if (dist)
            dist->push_back(std::get<1>(candidate));
    }

    // If no matches have been found, return false. Here, this only corresponds to the case, where
    // minDist is set too large and excludes all candidates.
	return !matches.empty();
}