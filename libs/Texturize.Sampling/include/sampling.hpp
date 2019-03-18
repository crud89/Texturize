#pragma once

#include <texturize.hpp>
#include <analysis.hpp>

#include <vector>
#include <random>
#include <optional>

#include <opencv2\core.hpp>
#include <opencv2\ml.hpp>
#include <opencv2\features2d.hpp>
#include <opencv2\flann\flann_base.hpp>

/// \namespace Texturize
/// \brief The root namespace, that contains all framework classes.
namespace Texturize {

	/// \defgroup synthesis Synthesis
	/// Contains components used during Synthesis phase. For more information see \ref index.
	/// @{

	/// \brief
	///
	///
	class TEXTURIZE_API IDescriptorExtractor {
	public:
		/// \brief Calculates the runtime neighborhood descriptors of a search space exemplar.
		/// \param exemplar A sample, containing the search space descriptors of the exemplar.
		/// \returns A matrix, containing the runtime neighborhood descriptors of the provided sample.
		virtual cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar) const = 0;

		/// \brief Calculates the runtime neighborhood descriptors of a search space exemplar.
		/// \param exemplar A sample, containing the search space descriptors of the exemplar.
		/// \param uv The uv map used to resolve the pixel coordinates inside the exemplar sample.
		/// \returns A matrix, containing the runtime neighborhood descriptors of the provided sample.
		virtual cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const = 0;
	};

	class TEXTURIZE_API DescriptorExtractor :
		public IDescriptorExtractor
	{
	public:
		/// \brief Returns the value of a pixel neighborhood from the exemplar.
		/// \param exemplar The exemplar sample, projected into search space.
		/// \param at The x and y coordinate of the center texel.
		/// \param delta The offset of the direction of the proxy pixel.
		/// \returns A vector containing the proxy pixel's value.
		///
		/// Proxy pixels are not neccessarily existing within the exemplar. Instead they describe some form of neighborhood appearance relation, i.e. they can also be mean
		/// color values of the neighborhood into direction `delta` or other representations. The following image shows the relation of a centering texel to it's four proxy
		/// pixels, as originally described by Sylvain Lefebvre and Hugues Hoppe.
		///
		/// \image html DefaultConvergence.svg 
		///
		/// They later improved neighborhood matching by averaging the value of the proxy pixels. Each one is calculated from an L-shaped neighborhood, as shown in the 
		/// following figure. The values of each L-neighborhood is averaged and returned as *proxy pixel* in this implementation.
		///
		/// \image html ImprovedConvergence.svg
		/// 
		/// Proxy pixels capture the *appearance* of a pixel neighborhood and are used to create a runtime neighborhood descriptor. Differently to the *appearance space*
		/// neighborhoods, described in `Texturize::AppearanceSpace`, those neighborhood descriptors capture the appearance of an already synthesized pixel and it's neighbors.
		///
		/// \see Sylvain Lefebvre and Hugues Hoppe. "Parallel Controllable Texture Synthesis." In: ACM Trans. Graph. 24.3 (July 2005), pp. 777-786. issn: 0730-0301. doi: 10.1145/1073204.1073261. url: http://doi.acm.org/10.1145/1073204.1073261
		/// \see Sylvain Lefebvre and Hugues Hoppe. "Appearance-space Texture Synthesis." In: ACM Trans. Graph. 25.3 (July 2006), pp. 541-548. issn: 0730-0301. doi: 10.1145/1141911.1141921. url: http://doi.acm.org/10.1145/1141911.1141921. 
		/// \see Texturize::AppearanceSpace
		/// \see Texturize::ISearchSpace
		/// \see Texturize::ISearchIndex
		/// \see Texturize::SearchIndex
		/// \see Texturize::DescriptorExtractor::getPixelNeighborhoods
		static std::vector<float> getProxyPixel(const Sample& exemplar, const cv::Point2i& at, const cv::Vec2i& delta);

		/// \brief Returns the value of a pixel neighborhood from the exemplar.
		/// \param exemplar The exemplar sample, projected into search space.
		/// \param at The x and y coordinate of the center texel.
		/// \param delta The offset of the direction of the proxy pixel.
		/// \param uv The uv map used to resolve the pixel coordinate provided by the `at` parameter.
		/// \returns A vector containing the proxy pixel's value.
		///
		/// \see Texturize::DescriptorExtractor::getProxyPixel(const Sample& exemplar, const cv::Point2i& at, const cv::Vec2i& delta)
		static std::vector<float> getProxyPixel(const Sample& exemplar, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta);

		/// \brief Returns the value of a pixel neighborhood from the exemplar.
		/// \param exemplar The exemplar sample, projected into search space.
		/// \param at The x and y coordinate of the center texel.
		/// \param delta The offset of the direction of the proxy pixel.
		/// \param uv The uv map used to resolve the pixel coordinate provided by the `at` parameter.
		/// \param rowPtr A pointer to an array of color values, that store the result. Note that the array must have at least as many items as there are channels within the `exemplar`.
		///
		/// \see Texturize::DescriptorExtractor::getProxyPixel(const Sample& exemplar, const cv::Point2i& at, const cv::Vec2i& delta)
		static void getProxyPixel(const Sample& exemplar, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta, float* const rowPtr);

		// IDescriptorExtractor
	public:
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar) const override;
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const override;

	protected:
		/// \brief Generates a simple UV map, that reproduces the exemplar.
		/// \param exemplar The exemplar sample to generate the UV map for.
		/// \returns A matrix with a depth of 2 channels, containing the generated UV map.
		cv::Mat createContinuousUvMap(const Sample& exemplar) const;

		/// \brief Extracts the pixel neighborhoods for all pixels of an exemplar sample.
		/// \param exemplar The exemplar sample to extract the pixel neighborhoods from.
		/// \param uv The uv map used to resolve the pixel coordinates inside the exemplar sample.
		/// \returns A column-major matrix, containing the pixel neighborhoods for each exemplar pixel.
		cv::Mat getPixelNeighborhoods(const Sample& exemplar, const cv::Mat& uv) const;
	};

	/// \brief Provides access to runtime neighborhood descriptors during synthesis.
	///
	/// This class is typically inherited by implementaions of `ISearchIndex` in order to extract runtime neighborhood descriptors. Those differ from analysis neighborhoods
	/// in a way, that they are not neccessarily existent within the exemplar.
	///
	/// \see Texturize::ISearchIndex
	/// \see Texturize::ISearchSpace
	/// \see Texturize::SearchIndex
	class TEXTURIZE_API PCADescriptorExtractor :
		public DescriptorExtractor 
	{
	private:
		// TODO: Re-use projector from search space. Currently, the PCA matrix will be re-evaluated.
		std::unique_ptr<cv::PCA> _projector;

		// IDescriptorExtractor
	public:
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar) const override;
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const override;

	private:
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv, std::unique_ptr<cv::PCA>& projector) const;
		cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv, const std::unique_ptr<cv::PCA>& projector) const;
	};

	/// \brief An interface that is used to access index search spaces.
	///
	/// A *search index* is created from a *search space* prior to synthesis in order to match pixel neighborhoods. There are different approaches to this, so this 
	/// interface only provides methods for neighborhood matching. The actual indexing process needs to be provided along implementations of this interface.
	class TEXTURIZE_API ISearchIndex {
	public:
		/// \brief Finds the best match for a given pixel neighborhood.
		/// \param descriptors An array, containing all neighborhood descriptors of the currently synthesized sample.
		/// \param uv A two-dimensional map, where each pixel contains the continuous u and v coordinates of the exemplar texel at the pixel's location.
		/// \param at The x and y coordinates of the pixel to match.
		/// \param match The u and v coordinates of the best match.
		/// \param minDist The minimum distance between the source texel and match within the exemplar.
		/// \param dist A pointer to a value, containing the distance between the source texel and match, after the method has returned.
		/// \returns True, if a match has been found, given the provided constraints.
		///
		/// Actual synthesis is a two-phase process. After analyzing the exemplar, a set of neighborhood descriptors is stored within a *search space*. Those neighborhoods,
		/// however, are not necessarily present within the synthesized sample. In order to match neighborhoods anyway, a runtime descriptor needs to be calculated for each
		/// neighborhood of the sample. A set of those descriptors is provided alongside an uv map. The index then extracts the descriptor at a given location (by resolving
		/// the `at` parameter inside the uv map) and tries to find close matches within the exemplar descriptors. Note that the definition of the distance parameter depends
		/// on the implementation of the actual index. Typically the `minDist` parameter contains a geometric distance between two texels, whilst the `dist` parameter
		/// describes the L2 distance between both texel vectors, i.e. their "similarity".
		///
		/// \see Texturize::ISearchSpace
		virtual bool findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const = 0;

		/// \brief Finds the best matches for a given pixel neighborhood.
		/// \param descriptors An array, containing all neighborhood descriptors of the currently synthesized sample.
		/// \param uv A two-dimensional map, where each pixel contains the continuous u and v coordinates of the exemplar texel at the pixel's location.
		/// \param at The x and y coordinates of the pixel to match.
		/// \param matches A vector of u and v coordinates of the best matches.
		/// \param k The number of matches to find.
		/// \param minDist The minimum distance between the source texel and a match within the exemplar.
		/// \param dist A pointer to a vector, containing the distances between the source texel and match, after the method has returned.
		/// \returns True, if at least one match has been found, given the provided constraints.
		///
		/// This method typically runs `findNearestNeighbor` multiple times, excluding the already found matches. Please refer to the notes there.
		///
		/// \see Texturize::ISearchSpace
		/// \see Texturize::ISearchIndex::findNearestNeighbor
		virtual bool findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const = 0;

		/// \brief Returns a reference of the search space, indexed by the current instance.
		/// \returns A reference of the search space, indexed by the current instance.
		virtual std::shared_ptr<ISearchSpace> getSearchSpace() const = 0;

		/// \brief Returns a reference of the descriptor extractor, that is used to extract runtime neighborhood descriptors.
		/// \returns A reference of the descriptor extractor, that is used to extract runtime neighborhood descriptors.
		virtual std::shared_ptr<IDescriptorExtractor> getDescriptorExtractor() const = 0;
	};

	/// \brief Implements a search index, based on pixel neighborhood appearances.
	///
	/// The class is a base class for custom search index implementations. A search index has two roles: it generates runtime neighborhood descriptors from search space 
	/// descriptors and implements neighborhood matching. The `DescriptorExtactor` class implements runtime neighborhood extraction, so all that's left to do is implementing
	/// the `ISearchIndex` interface to provide different neighborhood matching strategies.
	///
	/// \todo The `DescriptorExtractor` abstracts the calculation of neighborhood descriptors, so the search index only needs to handle nearest neighbor search. However,
	///       there may be different forms of neighborhood descriptors, like CV descriptors (ORB, etc.) or other forms of representing them. For example, a different 
	///       extractor might implement Local Linear Embedding or Isomaps to reduce dimensionality. Currently, however, all thats left when implementing a search index is to
	///       implement the `ISearchIndex` interface and inherit from this class.
	///
	/// \see Texturize::CoherentIndex
	/// \see Texturize::RandomWalkIndex
	/// \see Texturize::ISearchIndex
	/// \see Texturize::DescriptorExtractor
	class TEXTURIZE_API SearchIndex :
		public ISearchIndex
	{
		/// \example TrivialSearchIndex.cpp This example demonstrates how to implement a search index. The implementation matches pixel neighborhoods by calculating the 
		///                                 euclidean distance between pixel neighborhood descriptors. This basically resembles the naive sampling algorithm, described by
		///                                 Efros and Leung. It is simple to implement, but comes with a certain runtime cost. The `init` method, thus indexes pixel 
		///                                 neighborhoods by using dimensionality reduction to minimize the number of descriptor components. The 
		///                                 `Texturize::DescriptorExtractor` base class uses Principal Component Analysis (PCA) for this. Other approaches could use Local 
		///                                 Linear Embedding (LLE) or Isomaps. Note that this dimensionality reduction was not done in naive sampling.
		///
		///                                 The search index is also responsible for matching pixel neighborhoods during runtime. In this example, each pixel neighborhood
		///                                 of the exemplar is compared against the requested descriptor. This corresponds to an overall cost of 
		///                                 \f$ \mathcal{O} (n \times m) \f$, which is not ideal. More satisfied approaches may use quantized search trees or coherencies.
		///                                 The `Texturize::CoherentIndex` uses a pre-calculated set of coherent pixel candidates, from which a random selection is chosen.
		///
		/// \see Alexei A. Efros and Thomas K. Leung. "Texture Synthesis by Non Parametric Sampling." In: Proceedings of the International Conference on Computer Vision - Volume 2. ICCV '99. Washington, DC, USA: IEEE Computer Society, 1999. isbn: 0-7695-0164-8. url: http://dl.acm.org/citation.cfm?id=850924.851569.
		/// \see Li-Yi Wei and Marc Levoy. "Fast Texture Synthesis Using Tree-structured Vector Quantization." In: Proceedings of the 27th Annual Conference on Computer Graphics and Interactive Techniques. SIGGRAPH '00. New York, NY, USA: ACM Press/Addison-Wesley Publishing Co., 2000, pp. 479-488. isbn: 1-58113-208-5. doi: 10.1145/344779.345009. url: http://dx.doi.org/10.1145/344779.345009. 
		/// \see Michael Ashikhmin. "Synthesizing Natural Textures." In: Proceedings of the 2001 Symposium on Interactive 3D Graphics. I3D '01. New York, NY, USA: ACM, 2001, pp. 217-226. isbn: 1-58113-292-1. doi: 10.1145/364338.364405. url: http://doi.acm.org/10.1145/364338.364405. 
		/// \see Xin Tong et al. "Synthesis of Bidirectional Texture Functionson Arbitrary Surfaces." In: ACM Trans. Graph. 21.3 (July 2002), pp. 665-672. issn: 07300301. doi: 10.1145/566654.566634. url: http://doi.acm.org/10.1145/566654.566634. 

	protected:
		const std::shared_ptr<ISearchSpace> _searchSpace;
		const std::shared_ptr<IDescriptorExtractor> _descriptorExtractor;
		const cv::NormTypes _normType;

	protected:
		/// \brief Creates a new search index.
		/// \param searchSpace A reference of a search space instance.
		SearchIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, cv::NormTypes normType = cv::NORM_L2SQR);

		virtual ~SearchIndex() = default;

	public:
		/// \brief Returns a reference of the search space, indexed by the current instance.
		/// \returns A reference of the search space, indexed by the current instance.
		std::shared_ptr<ISearchSpace> getSearchSpace() const override;

		/// \brief Returns a reference of the descriptor extractor, that is used to extract runtime neighborhood descriptors.
		/// \returns A reference of the descriptor extractor, that is used to extract runtime neighborhood descriptors.
		std::shared_ptr<IDescriptorExtractor> getDescriptorExtractor() const override;
	};
	
	class TEXTURIZE_API ANNIndex :
		public SearchIndex
	{
	protected:
		template <typename TVal = float>
		struct _L2Ex {
		private:
			int _weightDims{ -1 };

		public:
			_L2Ex(int weightDims = -1) : _weightDims(weightDims) {}

		public:
			typedef ::cvflann::True                         is_kdtree_distance;
			typedef ::cvflann::True                         is_vector_space_distance;
			typedef TVal                                    ElementType;
			typedef typename cv::Accumulator<TVal>::Type    ResultType;

			template <typename Tlhs, typename Trhs>
			ResultType operator()(Tlhs lhs, Trhs rhs, size_t size, ResultType worstDist = -1) const {
				ResultType result = ResultType(), weight = ResultType();
				ResultType diff0, diff1, diff2, diff3;
				Tlhs lastElement = lhs + size;

				// At least one dimension is responsible for holding additional weights, if `_weightDims` >= 0.
				if (_weightDims >= 0)
					lastElement -= _weightDims;

				Tlhs lastGroup = lastElement - 3;

				// Group-wise process each value.
				while (lhs < lastGroup) {
					diff0 = (ResultType)(lhs[0] - rhs[0]); diff1 = (ResultType)(lhs[1] - rhs[1]);
					diff2 = (ResultType)(lhs[2] - rhs[2]); diff3 = (ResultType)(lhs[3] - rhs[3]);

					result += diff0 * diff0 + diff1 * diff1 + diff2 * diff2 + diff3 * diff3;

					// Move on...
					lhs += 4; rhs += 4;

					//if ((worstDist > 0) && result > worstDist)
					//	return result;
				}

				// Process the remaining values.
				while (lhs < lastElement) {
					diff0 = (ResultType)(*lhs++ - *rhs++);
					result += diff0 * diff0;
				}

				// Finally, process the weight maps, which are added in differently (non-squared).
				while (lhs < lastElement + _weightDims)
					weight += std::abs<ResultType>((ResultType)(*lhs++ - *rhs++));

				return result * (weight / static_cast<ResultType>(_weightDims));
			}

			template <typename Tlhs, typename Trhs>
			inline ResultType accum_dist(const Tlhs& lhs, const Trhs& rhs, int) const {
				return std::pow((lhs - rhs), 2);
			}
		};

		typedef typename ANNIndex::_L2Ex<float>           DistanceType;
		typedef typename ::cvflann::Index<DistanceType>   IndexType;
		typedef typename DistanceType::ElementType        ElementType;
		typedef typename ::cvflann::Matrix<ElementType>   MatrixType;

	private:
		std::unique_ptr<IndexType> _index;
		cv::Mat _descriptors;
		int _sampleWidth{ 0 };

	protected:
		void init(const cv::flann::IndexParams& indexParams);
		void init(const cv::flann::IndexParams& indexParams, std::optional<std::reference_wrapper<const Sample>> weightMap);

	public:
		ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const cv::Ptr<const cv::flann::IndexParams> indexParams = cv::makePtr<const cv::flann::KDTreeIndexParams>(), cv::NormTypes normType = cv::NORM_L2SQR);
		ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const cv::Ptr<const cv::flann::IndexParams> indexParams = cv::makePtr<const cv::flann::KDTreeIndexParams>(), cv::NormTypes normType = cv::NORM_L2SQR);

		ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& guidanceMap, const cv::Ptr<const cv::flann::IndexParams> indexParams = cv::makePtr<const cv::flann::KDTreeIndexParams>(), cv::NormTypes normType = cv::NORM_L2SQR);
		ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap, const cv::Ptr<const cv::flann::IndexParams> indexParams = cv::makePtr<const cv::flann::KDTreeIndexParams>(), cv::NormTypes normType = cv::NORM_L2SQR);

	public:
		bool findNearestNeighbor(const std::vector<float>& descriptor, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const;
		bool findNearestNeighbors(const std::vector<float>& descriptor, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const;

		// ISearchIndex
	public:
		bool findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const override;
		bool findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const override;
	};

	/// \brief A search index implementation that clusters the search space using a quantized kd-tree, which allows for fast neighborhood queries.
	///
	/// This search index allows for fast, yet accurate (i.e. non-approximative) runtime neighborhood queries. This is achieved by clustering the search space when creating the index.
	///
	/// TODO: DOC
	class TEXTURIZE_API KNNIndex :
		public ANNIndex
	{
	private:
		// NOTE: This is currently missing in OpenCV 4.0.1 cv::flann namespace, hence the redefine.
		struct KDTreeSingleIndexParams : public cv::flann::KDTreeIndexParams {
			KDTreeSingleIndexParams(int trees = 4)
			{
				this->setAlgorithm(cvflann::FLANN_INDEX_KDTREE_SINGLE);
				this->setInt("trees", trees);
			}
		};

	public:
		/// \brief Creates a search index based on exact kd-tree matching.
		KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor);
		KNNIndex(std::shared_ptr<ISearchSpace> searchSpace);

		KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& weightMap);
		KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& weightMap);
	};

	/// \brief Implements a search index, that matches pixel neighborhoods based on coherent pixels.
	///
	/// Coherent pixels have first been described by Michael Ashikhmin and are based on the observation that typically good match candidates are direct neighbors of already
	/// synthesized pixels. By resolving the coordinates in the synthesized neighborhood of a pixel, those *coherent* pixels can be resolved efficiently, without performing
	/// a heavy knn-search for each result pixel. The following image demonstrates this approach.
	///
	/// \image html Coherence.svg
	///
	/// \see Michael Ashikhmin. "Synthesizing Natural Textures." In: Proceedings of the 2001 Symposium on Interactive 3D Graphics. I3D 2001. New York, NY, USA: ACM, 2001, pp. 217-226. isbn: 1-58113-292-1. doi: 10.1145/364338.364405. url: http://doi.acm.org/10.1145/364338.364405. 
	class TEXTURIZE_API CoherentIndex :
		public SearchIndex
	{
	protected:
		/// \brief The type of a match candidate.
		typedef cv::Vec2f							TCandidate;					// A candidate is described by it's position within the exemplar.
		
		/// The type of the similarity distance.
		typedef float								TDistance;					// The distance between two candidates is described by a single-precision floating point value.

		/// \brief Describes a match as a set of candidate coordinates and similarity.
		typedef std::tuple<TCandidate, TDistance>	TMatch;						// Matches are tuples of candidates and their distance towards the sample descriptor.
		
	private:
		cv::Mat _exemplarDescriptors;
		const Sample _guidanceMap;

	public:
		/// \brief Creates a new search index.
		/// \param searchSpace A reference of a search space instance.
		/// \param distanceMeasure The type of norm to use to measure similarity between a candidate and a descriptor.
		CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace);
		CoherentIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap);

	private:
		void init();

	protected:
		/// \brief Calculates the similarity between a candidate and descriptor.
		/// \param targetDescriptor The descriptor to compare against a candidate from the exemplar sample.
		/// \param towards The point from which the candidate descriptor should be compared towards the provided target.
		/// \returns A value, describing the similarity between the target descriptor and exemplar descriptor at the provided point. The similarity is measured by the norm, 
		///			 the index has been initialized with.
		TDistance measureVisualDistance(const std::vector<float>& targetDescriptor, const cv::Point2i& towards) const;

		/// \brief Gets a coherent candidate from a neighborhood pixel.
		/// \param targetDescriptor The descriptor to compare against a candidate from the exemplar sample.
		/// \param uv A two-dimensional map, where each pixel contains the continuous u and v coordinates of the exemplar texel at the pixel's location.
		/// \param at The x and y coordinate within the uv map of the pixel to extract the coherent candidate from.
		/// \param delta The direction into which the coherent pixel should be extracted.
		/// \param match A buffer to store the coherent candidate in.
		///
		/// The method resolves the u and v coordinates of the pixel addressed by the `at` parameter and then applies the `delta` to it to get the actual coherent u and v 
		/// coordinates from the `uv` map. It then resolves those coordinates in the exemplar and shifts them into negative `delta` location. The descriptor retrieved by this
		/// process is then compared towards the `targetDescriptor`. The result is stored inside `match`.
		void getCoherentCandidate(const std::vector<float>& targetDescriptor, const cv::Mat& uv, const cv::Point2i& at, const cv::Vec2i& delta, TMatch& match) const;

		// ISearchIndex
	public:
		/// \param minDist A lower thresold for similarity measures.
		/// \param dist The similarity between the candidate and the match.
		bool findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const override;

		/// \param minDist A lower thresold for similarity measures.
		/// \param dist The similarity between the candidate and the match.
		/// \param k The number of coherent candidates to return. Must be a value between 1 and 8.
		bool findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const override;
	};

	// class TEXTURIZE_API kCoherentIndex : public SearchIndex { };

	/// \brief Implements a search index, that matches pixels by randomly walking coherent candidate neighborhoods.
	/// 
	/// The random walk strategy has been introduced by Busto et al. to improve performance of k-Coherence analysis. Different from k-Coherence, it does not require heavy 
	/// offline knn matching during exemplar analysis to calculate a set of k additional candidates. Instead it starts by selecting a coherent candidate and improves the
	/// result by randomly walking the neighborhood of it, replacing it in case a better match has been found.
	///
	/// \see Pau Panareda Busto et al. "Instant Texture Synthesis by Numbers." In: VMV. 2010, pp.81-85.
	/// \see Texturize::CoherentIndex
	class TEXTURIZE_API RandomWalkIndex :
		public CoherentIndex
	{
	private:
		mutable std::mt19937 _rng;

	public:
		/// \brief Creates a new search index.
		/// \param searchSpace A reference of a search space instance.
		/// \param distanceMeasure The type of norm to use to measure similarity between a candidate and a descriptor.
		RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace);
		RandomWalkIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& guidanceMap);

	private:
		cv::Vec2f getRandomPixelAround(const cv::Vec2f& point, int radius, int dominantDimensionExtent) const;
		cv::Vec2f getRandomPixelAround(const cv::Vec2f& point, float radius) const;

		// ISearchIndex
	public:
		/// \param minDist A lower thresold for similarity measures.
		/// \param dist The similarity between the candidate and the match.
		bool findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, cv::Vec2f& match, float minDist = 0.0f, float* dist = nullptr) const override;

		/// \param minDist A lower thresold for similarity measures.
		/// \param dist The similarity between the candidate and the match.
		/// \param k The number of coherent candidates to return. Must be a value between 1 and 8.
		bool findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<cv::Vec2f>& matches, const int k = 1, float minDist = 0.0f, std::vector<float>* dist = nullptr) const override;
	};

	/// \brief Generates a permutation vector from a set of coordinates.
	/// 
	/// Different to random noise functions, like gaussian or perlin noise, the `CoordinateHash` returns the same offset for a indentical set of input coordinates, hence the 
	/// name *hash*. It can be used to create reproduceable noise in parallel algorithms.
	class TEXTURIZE_API CoordinateHash 
	{
		// TODO: Move this to analysis.
	protected:
		std::vector<int> _permutation;
		std::vector<cv::Vec2f> _gradients;
		const TX_DWORD _mask;

	public:
		/// \brief Creates a new coordinate hash function.
		/// \param maskSize The size of the permutation vector, i.e. the number of possible combinations that can be produced.
		CoordinateHash(int maskSize = 256);

		/// \brief Creates a new coordinate hash function.
		/// \param seed The seed used to initialize the random number generator, filling the hash table.
		/// \param maskSize The size of the permutation vector, i.e. the number of possible combinations that can be produced.
		CoordinateHash(uint64_t seed, int maskSize = 256);

	private:
		void init(uint64_t seed);

	public:
		/// \brief Calculates a offset vector from a set of x and y coordinates.
		/// \param x The x coordinate of the input vector.
		/// \param y The y coordinate of the input vector.
		/// \returns An offset vector.
		cv::Vec2i calculate(int x, int y) const;

		/// \brief Calculates a offset vector from a set of x and y coordinates.
		/// \param v The x and y coordinates of the input vector.
		/// \returns An offset vector.
		cv::Vec2i calculate(const cv::Vec2i& v) const;
	};

	/// \brief A set of settings to initialize a synthesizer with.
	///
	/// Note that not all synthesizers require nor use all of the settings provided here. Also the function of the member variables might vary between synthesizer
	/// implementations. For example the `_seedCoords` and `_seedKernel` members are typically used by pixel-sampling algorithms to paste an initial window from the exemplar
	/// to the synthesis result as a handle to start the synthesis from, whilst pyramid-based algorithms ignore the `_seedKernel` completely and only use the `_seedCoords` 
	/// member to initialize their pyramid origin, whereby the vector actually acts like an offset to initially translate the texture within the synthesis result. On the other 
	/// hand, pyramid-based synthesizers require the `_scale` value to produce meaningful output.
	///
	/// Also custom synthesizers might implement different versions of this class, in order to support other specific settings. The actual synthesizer is then responsible to
	/// evaluate the settings.
	class TEXTURIZE_API SynthesisSettings {
	public:
		/// \brief The coordinates the synthesizer uses to initialize the synthesis result.
		cv::Point2f _seedCoords;

		/// \brief The size of the kernel the synthesizer pastes from the exemplar to the synthesis result during initialization.
		int _seedKernel;

		/// \brief The seed to initialize random number generators with, in order to create reproducible results.
		uint64_t _rngState;

	public:
		/// \brief Creates a new settings object.
		/// \param rngState The seed to initialize random number generators with.
		SynthesisSettings(uint64_t rngState = 0);

		/// \brief Creates a new settings object.
		/// \param seedCoords The u and v coordinates of the pixel, the result will be initialized with.
		/// \param kernel The size of the runtime neighborhood window, used to generate descriptors.
		/// \param rngState The seed to initialize random number generators with.
		SynthesisSettings(cv::Point2f seedCoords, int kernel = 5, uint64_t rngState = 0);

	public:
		/// \brief Checks if the settings match constraints required by the synthesizer.
		/// \returns True, if the settings can be used with the synthesizer, they are intented to be used with.
		virtual bool validate() const;

	public:
		/// \brief Generates a settings object, initialized with a random set of coordinates.
		/// \param kernel The size of the runtime neighborhood window, used to generate descriptors.
		/// \param state The seed to initialize random number generators with.
		/// \returns A settings object, initialized with a random set of coordinates and the provided parameters.
		static SynthesisSettings random(int kernel = 5, uint64_t state = time(nullptr));
	};

	/// \brief A set of settings to initialize instances of \ref `Texturize::PyramidSynthesizer` with.
	///
	/// \see Texturize::PyramidSynthesizer
	class TEXTURIZE_API PyramidSynthesisSettings :
		public SynthesisSettings
	{
	public:
		/// \brief Defines a function, that returns a value between 0.0 and 1.0, based on a pyramid level, passed to it as an integer.
		///
		/// \see _randomnessSelector
		typedef std::function<float(int)> RandomnessSelectorFunction;

		/// \brief A callback that can report the current synthesis progress.
		///
		/// The handler reports the following parameters to a callback:
		/// - The current pyramid level
		/// - The correction pass index
		/// - The current uv map
		/// It gets called after a synthesis pass has finished, i.e. the algorithm wants to continue with the next pyramid level or has completed a correction pass. In
		/// case no correction pass has been executed, the parameter is -1.
		///
		/// **Example**
		///
		/// The following code demonstrates how to setup a callback, that displays the result after each synthesis level.
		/// \include ProgressCallback.cpp
		typedef EventDispatcher<void, int, int, const cv::Mat&> ProgressHandler;

		/// \brief A callback that reports the result of a sub-pass.
		///
		/// The handler works similar to the `ProgressHandler`, however, it calls back after each synthesis step or correction sub-pass. It gets passed the following 
		/// parameters:
		/// - The name of the current algorithm step (i.e. upsampling, jitter, ...)
		/// - The current uv map
		///
		/// \see Texturize::PyramidSynthesisSettings::ProgressHandler
		typedef EventDispatcher<void, const std::string&, const cv::Mat&> SubpassFeedbackHandler;

	public:
		/// \brief A callback that get's called to inform clients about the current synthesis progress.
		///
		/// \see Texturize::PyramidSynthesisSettings::ProgressHandler
		ProgressHandler _progressHandler;

		/// \brief A callback that get's called to inform a client about the result of a subpass.
		///
		/// \see Texturize::PyramidSynthesisSettings::ProgressHandler
		SubpassFeedbackHandler _feedbackHandler;

	public:
		/// \brief The scale factor that is used to align the output spacing within scale-invariant algorithms.
		///
		/// Pyramid-based algorithms typically start synthesis within a single point that gets upsampled over and over again until the final sample size has been reached. 
		/// However, such scale-invariant implementations have no deal of the actual extent of the original exemplar. Therefor the scale factor is used to adjust the output 
		/// spacing within the synthesis result between two texels.
		/// 
		/// For example an 128 square texel exemplar will appear scaled up by a factor of two, if a scale value of 256 is provided and scaled down by a factor of two for 
		/// a value of 64.0.
		float _scale;

		/// \brief A function that returns the amount of randomness per pyramid level.
		///
		/// The randomness selector function is used to control the amount of jitter per pyramid level. It is a function that returns a value between 0.0 and 1.0, based on
		/// a pyramid level, passed to it as integer. It is typically the only parameter, that can control the synthesis process. It can either be user-defined or generated
		/// from a distribution function. Good results can be achieved for most textures by using a natural distribution. The deviation equals the average scale of features
		/// in the exemplar (in pyramid levels, i.e. \f$ log_2 ( 2^l ) \f$). The offset of the distribution peak correlates with the texture regularity score (g-score),
		/// described by Liu et al. More information on this subject can be found within [this thesis](https://github.com/Aschratt/Texturize-Thesis).
		///
		/// \see Yanxi Liu, Wen-Chieh Lin, and James Hays. "Near-regular Texture Analysis and Manipulation." In: ACM SIGGRAPH 2004 Papers. SIGGRAPH '04. Los Angeles, California: ACM, 2004, pp. 368-376. doi: 10.1145/1186562.1015731. url: http://doi.acm.org/10.1145/1186562.1015731.
		/// \see Sylvain Lefebvre and Hugues Hoppe. "Parallel Controllable Texture Synthesis." In: ACM Trans. Graph. 24.3 (July 2005), pp. 777-786. issn: 0730-0301. doi: 10.1145/1073204.1073261. url: http://doi.acm.org/10.1145/1073204.1073261
		/// \see Sylvain Lefebvre and Hugues Hoppe. "Appearance-space Texture Synthesis." In: ACM Trans. Graph. 25.3 (July 2006), pp. 541-548. issn: 0730-0301. doi: 10.1145/1141911.1141921. url: http://doi.acm.org/10.1145/1141911.1141921. 
		RandomnessSelectorFunction _randomnessSelector;

		/// \brief The lower threshold, defining the pyramid level from which correction passes are executed.
		///
		/// Correction can usually not produce any meaningful results for samples with low resolutions and are therefor typically skipped until a certain level within the 
		/// synthesis has been reached. The default value of this parameter, 3 means that the algorithm will start correcting the sample from pyramid level three and beyond.
		unsigned int _correctionLevelThreshold = 3;

		/// \brief The number of correction passes applied to each pyramid level.
		///
		/// More correction passes typically result in better convergence, but also weaker performance. A value of 8 or higher typically does not produce significantly
		/// better results than lower values.
		unsigned int _correctionPasses = 2;

		/// \brief The number of sub-passes per correction pass along one axis.
		///
		/// Each correction pass executes a number of sub-passes for all **n** pixels along one axis. If this value is 2, two sub-passes are executed per step within a
		/// correction pass. This means that four actual sub-passes are executed. If the value is set to 3, nine sub-passes are executed and so on.
		///
		/// A sub-pass corrects one pixel by evaluating it's neighborhood. If this value is set to 1, the algorithm behaves similar to classical pixel-based algorithms, 
		/// evaluating the neighborhood of each individual pixel subsequently. However, since this relies on pixel-neighbors that are also corrected, this may produce 
		/// low-quality results and poor convergence. Higher values let the algorithm correct each pixel within different sub-passes, that are sequentially executed in 
		/// row-major order.
		///
		/// Values greater than 3 typically do not improve synthesis quality significantly.
		unsigned int _correctionSubPasses = 2;



		std::optional<Sample> _guidanceMap;

	public:
		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param randomness A constant randomness value, that defines the jitter amplitude at each pyramid level.
		/// \param rngState A seed to initialize random number generators with.
		PyramidSynthesisSettings(float scale, float randomness = 0.5f, uint64_t rngState = 0);

		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param randomness A vector of randomness values, that define the jitter amplitude at each pyramid level. In case the vector is too short, a value of 0 is 
		///		   assumed for the missing levels.
		/// \param rngState A seed to initialize random number generators with.
		PyramidSynthesisSettings(float scale, const std::vector<float>& randomness, uint64_t rngState = 0);

		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param fn A function that gets passed an integer value, containing a pyramid level and returns the jitter amplitude at this level.
		/// \param rngState A seed to initialize random number generators with.
		///
		/// \see Texturize::PyramidSynthesisSettings::RandomnessSelectorFunction
		PyramidSynthesisSettings(float scale, RandomnessSelectorFunction fn, uint64_t rngState = 0);

		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param seedCoords The coordinates to initialize the result sample with.
		/// \param randomness A constant randomness value, that defines the jitter amplitude at each pyramid level.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param rngState A seed to initialize random number generators with.
		PyramidSynthesisSettings(float scale, cv::Point2f seedCoords, float randomness = 0.5f, int kernel = 5, uint64_t rngState = 0);

		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param seedCoords The coordinates to initialize the result sample with.
		/// \param randomness A vector of randomness values, that define the jitter amplitude at each pyramid level. In case the vector is too short, a value of 0 is 
		///		   assumed for the missing levels.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param rngState A seed to initialize random number generators with.
		PyramidSynthesisSettings(float scale, cv::Point2f seedCoords, const std::vector<float>& randomness, int kernel = 5, uint64_t rngState = 0);

		/// \brief Creates a new configuration instance for pyramid-based synthesizers.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param seedCoords The coordinates to initialize the result sample with.
		/// \param fn A function that gets passed an integer value, containing a pyramid level and returns the jitter amplitude at this level.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param rngState A seed to initialize random number generators with.
		///
		/// \see Texturize::PyramidSynthesisSettings::RandomnessSelectorFunction
		PyramidSynthesisSettings(float scale, cv::Point2f seedCoords, RandomnessSelectorFunction fn, int kernel = 5, uint64_t rngState = 0);
		
	public:
		/// \brief Creates a new configuration instance for pyramid-based synthesizers, based on a randomly selected seed coordinate.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param randomness A constant randomness value, that defines the jitter amplitude at each pyramid level.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param state A seed to initialize random number generators with.
		static PyramidSynthesisSettings random(float scale, float randomness = 0.5f, int kernel = 5, uint64_t state = time(nullptr));

		/// \brief Creates a new configuration instance for pyramid-based synthesizers, based on a randomly selected seed coordinate.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param randomness A vector of randomness values, that define the jitter amplitude at each pyramid level. In case the vector is too short, a value of 0 is 
		///		   assumed for the missing levels.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param state A seed to initialize random number generators with.
		static PyramidSynthesisSettings random(float scale, const std::vector<float>& randomness, int kernel = 5, uint64_t state = time(nullptr));

		/// \brief Creates a new configuration instance for pyramid-based synthesizers, based on a randomly selected seed coordinate.
		/// \param scale The scale factor that is used to align the output spacing within scale-invariant algorithms.
		/// \param fn A function that gets passed an integer value, containing a pyramid level and returns the jitter amplitude at this level.
		/// \param kernel The size of the neighborhood window used for matching pixel neighborhoods.
		/// \param state A seed to initialize random number generators with.
		static PyramidSynthesisSettings random(float scale, RandomnessSelectorFunction fn, int kernel = 5, uint64_t state = time(nullptr));
	};

	/// \brief The runtime state of a synthesizer, persistent between multiple synthesis passes.
	///
	/// An instance of this class is created by synthesizers to pass state information between multiple synthesis passes.
	class TEXTURIZE_API SynthesizerState
	{
	private:
		const SynthesisSettings _config;

	public:
		/// \brief Creates a new synthesizer state object.
		/// \param config A reference of the configuration, the synthesizer has been initialized with.
		SynthesizerState(const SynthesisSettings& config);
		virtual ~SynthesizerState() = default;

	public:
		/// \brief Returns the settings, the synthesizer has been configured with.
		/// \returns The settings, the synthesizer has been configured with.
		SynthesisSettings config() const;

		/// \brief Gets a reference of the settings, the synthesizer has been configured with.
		/// \param config The settings, the synthesizer has been configured with.
		void config(SynthesisSettings& config) const;
	};

	/// \brief The runtime state of a pyramid-based synthesizer, persistent between multiple synthesis passes.
	///
	/// \see Texturize::SynthesizerState
	/// \see Texturize::PyramidSynthesizer
	/// \see Texturize::ParallelPyramidSynthesizer
	class TEXTURIZE_API PyramidSynthesizerState :
		public SynthesizerState
	{
	private:
		const PyramidSynthesisSettings _configEx;

	private:
		std::unique_ptr<CoordinateHash> _hash;

	public:
		/// \brief Creates a new synthesizer state object.
		/// \param config A reference of the configuration, the synthesizer has been initialized with.
		PyramidSynthesizerState(const PyramidSynthesisSettings& config);

	public:
		/// \brief Returns the settings, the synthesizer has been configured with.
		/// \returns The settings, the synthesizer has been configured with.
		PyramidSynthesisSettings config() const;

		/// \brief Gets a reference of the settings, the synthesizer has been configured with.
		/// \param config The settings, the synthesizer has been configured with.
		void config(PyramidSynthesisSettings& config) const;

		/// \brief Returns an object that is used to calculate jitter offsets.
		/// \returns A pointer to an object, used to calculate jitter offsets.
		const CoordinateHash* getHash() const;

		/// \brief Returns the jitter amplitude at a certain pyramid level.
		/// \param level The pyramid level at which the jitter amplitude is requested.
		/// \returns A value between 0.0 and 1.0, stating the jitter amplitude at the requested pyramid level.
		float getRandomness(int level) const;

		/// \brief Returns the synthesis scale factor.
		/// \returns The synthesis scale factor.
		///
		/// \see Texturize::PyramidSynthesisSettings::_scale
		float getSpacing() const;

	private:
		cv::Mat _sample = cv::Mat();
		int _level = 0;

	public:
		/// \brief Updates the synthesizer state.
		/// \param level The pyramid level of the current synthesizer pass.
		/// \param sample The uv map, that resembles the currently synthesized sample.
		void update(const int level, const cv::Mat& sample);

		/// \brief Returns a copy of the currently synthesized sample.
		/// \returns A copy of the currently synthesized sample.
		///
		/// \see Texturize::PyramidSynthesizerState::update
		cv::Mat sample() const;

		/// \brief Gets a reference of the currently synthesized sample.
		/// \param sample A reference of the currently synthesized sample.
		///
		/// \see Texturize::PyramidSynthesizerState::update
		void sample(cv::Mat& sample) const;

		/// \brief Returns the pyramid level, the current synthesizer pass is working on.
		/// \returns The pyramid level, the current synthesizer pass is working on.
		int level() const;

		/// \brief Gets the pyramid level, the current synthesizer pass is working on.
		/// \param level The pyramid level, the current synthesizer pass is working on.
		void level(int& level) const;
	};

	/// \brief The base interface for synthesizer implementations.
	///
	/// \see Texturize::SynthesizerBase
	class TEXTURIZE_API ISynthesizer {
	public:
		/// \brief Synthesizes a new texture.
		/// \param width The width of the result sample.
		/// \param height The height of the result sample.
		/// \param result A reference to the result sample.
		/// \param config The configuration to initialize the synthesizer with.
		virtual void synthesize(int width, int height, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const = 0;

		/// \brief Synthesizes a new texture.
		/// \param size The width and height of the result sample.
		/// \param result A reference to the result sample.
		/// \param config The configuration to initialize the synthesizer with.
		virtual void synthesize(const cv::Size& size, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const = 0;
	};

	/// \brief The base interface for style transfer implementations.
	///
	/// Style transfer is a special case of texture synthesis. Instead of creating a completely new texture, the exemplar is used to fit a second image - the transfer
	/// target - to it. Basically for each pixel of the target, the synthesizer looks up the best correspondence from the exemplar.
	///
	/// \see Texturize::SynthesizerBase
	class TEXTURIZE_API IStyleTransfer {
	public:
		/// \brief Transfers the style of the search space to a target sample and stores the result.
		/// \param target The target sample the style should be transferred to.
		/// \param result A reference to the result sample.
		/// \param config The configuration to initialize the synthesizer with.
		virtual void transferStyle(const Sample& target, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const = 0;
	};

	/// \brief A base implementation for exemplar-based synthesizers.
	class TEXTURIZE_API SynthesizerBase :
		public ISynthesizer,
		public IStyleTransfer
	{
		/// \example NaiveSamplingSynthesizer.cpp
		/// This example demonstrates how to implement a custom synthesizer. The following code implements a synthesizer, that generates a new texture by naive sampling, as
		/// described by Efros and Leung.
		///
		/// \see Alexei A. Efros and Thomas K. Leung. "Texture Synthesis by Non Parametric Sampling." In: Proceedings of the International Conference on Computer Vision - Volume 2. ICCV '99. Washington, DC, USA: IEEE Computer Society, 1999. isbn: 0-7695-0164-8. url: http://dl.acm.org/citation.cfm?id=850924.851569.
	protected:
		const std::shared_ptr<ISearchIndex> _catalog;

	protected:
		/// \brief Creates a new synthesizer instance.
		SynthesizerBase() = delete;

		/// \brief Creates a new synthesizer instance.
		SynthesizerBase(const SynthesizerBase&) = delete;

		/// \brief Creates a new synthesizer instance.
		/// \param catalog An index, that provides access to exemplar pixel neighborhoods and implements neighborhood matching.
		SynthesizerBase(std::shared_ptr<ISearchIndex> catalog);

	public:
		virtual ~SynthesizerBase() { }

	public:
		/// \brief Returns the search index used to match pixel neighborhoods during synthesis.
		/// \returns A pointer to the search index, used to match pixel neighborhood during synthesis.
		std::shared_ptr<ISearchIndex> getIndex() const;
	};

	// TODO: This one can either take a search space pointer, or directly use an exemplar to build up a ColorSpaceDescriptor.
	// class TEXTURIZE_API PixelSamplingSynthesizer : public SynthesizerBase { };

	/// \brief Implements a pyramid-bases, non-parametric, per-pixel synthesizer.
	/// 
	/// The synthesizer implements an algorithm, that uses an indexed exemplar and synthesizes a result sample by traversing an image pyramid. The algorithm follows the
	/// one, that has originally been described by Sylvain Lefebvre and Hugues Hoppe. Note that this implementation works sequentially. For a parallel version use the 
	/// `ParallelPyramidSynthesizer` class.
	///
	/// Lefebvre and Hoppe originally based their synthesizer on image pyramids and later introduced an hierarchy, called "gaussian stack", that, instead of traversing
	/// a pyramidal hierarchy, uses a stack of increasingly blurred samples. This implementation does not use the stack, but instead uses simple pyramids.
	///
	/// **Example**
	///
	/// The following example shows how to create a synthesizer and synthesize a new texture from an indexed exemplar.
	/// \include PyramidSynthesizer.cpp
	/// 
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Parallel Controllable Texture Synthesis." In: ACM Trans. Graph. 24.3 (July 2005), pp. 777-786. issn: 0730-0301. doi: 10.1145/1073204.1073261. url: http://doi.acm.org/10.1145/1073204.1073261
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Appearance-space Texture Synthesis." In: ACM Trans. Graph. 25.3 (July 2006), pp. 541-548. issn: 0730-0301. doi: 10.1145/1141911.1141921. url: http://doi.acm.org/10.1145/1141911.1141921. 
	class TEXTURIZE_API PyramidSynthesizer :
		public SynthesizerBase
	{
	protected:
		/// \brief Creates a new synthesizer instance.
		/// \param catalog An index, that provides access to exemplar pixel neighborhoods and implements neighborhood matching.
		PyramidSynthesizer(std::shared_ptr<ISearchIndex> catalog);

	protected:
		/// \brief Performs synthesis for the next level of the image pyramid.
		/// \param sample The current result sample.
		/// \param state An object, that provides access to the runtime state of the synthesizer.
		virtual void synthesizeLevel(cv::Mat& sample, const PyramidSynthesizerState& state) const;

		/// \brief Doubles the effective resolution of the result sample.
		/// \param sample The current result sample.
		/// \param state An object, that provides access to the runtime state of the synthesizer.
		///
		/// The method is responsible of traversing the image pyramid one level upwards. It aligns the current coordinates along the upsampled map and interpolates the
		/// space between.
		virtual void upsample(cv::Mat& sample, const PyramidSynthesizerState& state) const;

		/// \brief Introduces spatial randomness to the current result sample.
		/// \param sample The current result sample.
		/// \param state An object, that provides access to the runtime state of the synthesizer.
		///
		/// The method disturbs the image in order to create randomness at different scales. The amplitude of the introduced randomness is provided by the jitter 
		/// configuration of the `PyramidSynthesizerConfig`.
		virtual void jitter(cv::Mat& sample, const PyramidSynthesizerState& state) const;

		/// \brief Corrects the current result sample by searching for close pixel neighborhoods for each texel.
		/// \param sample The current result sample.
		/// \param state An object, that provides access to the runtime state of the synthesizer.
		///
		/// This method is called multiple times in order to let the result sample converge agains a "best match". The number of correction passes can be configured within
		/// the `PyramidSynthesizerConfig`. For each texel, it asks the search index for a good candidate to replace it with, based on the current neighborhood. This process
		/// is not done sequentially, but rather the method divides the image into a set of sub-passes that are then executed sequentially. The number of sub-passes can be
		/// configured within the `PyramidSynthesizerConfig`.
		virtual void correct(cv::Mat& sample, const PyramidSynthesizerState& state) const;

		/// \brief Interpolates a pixel coordinate to fill the space created during upsampling.
		/// \param uv The uv coordinates of the origin texel from the previous pyramid level.
		/// \param delta The offset from the origin pixel in the current pyramid level to the target pixel.
		/// \param spacing The scale factor that is used to linearily interpolate the pixel coordinates.
		/// \returns The new coordinates for the pixel at the current pyramid level.
		virtual cv::Vec2f scaleTexel(const cv::Vec2f& uv, const cv::Vec2f& delta, float spacing) const;
		
		/// \brief Jitters a pixel at a given coordinate.
		/// \param at The x and y coordinate of the pixel to disturb.
		/// \param state An object, that provides access to the runtime state of the synthesizer.
		virtual cv::Vec2f translateTexel(const cv::Point2i& at, const PyramidSynthesizerState& state) const;

		/// \brief Calculates the runtime neighborhood descriptor of a texel.
		/// \deprecated This method is deprecated and not called by any built-in synthesizer. It does only exist for compatibility reasons, and will be removed in future
		///				releases. Consider using `SearchIndex::findNearestNeighbor` directly.
		virtual std::vector<float> getNeighborhoodDescriptor(const Sample* exemplar, const cv::Mat& sample, const cv::Point2i& uv, int kernel, bool weight = true) const;

		virtual void transferTo(const Sample& target, Sample& result, const PyramidSynthesizerState& state) const;

	public:
		void synthesize(int width, int height, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const override;
		void synthesize(const cv::Size& size, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const override;
		void transferStyle(const Sample& target, Sample& result, const SynthesisSettings& config = SynthesisSettings()) const override;

	public:
		/// \brief A factory method that creates a new synthesizer and initializes with a search index, that provides access to exemplar neighborhoods.
		/// \param catalog An search index, that provides access to exemplar neighborhoods and provides runtime pixel neighborhood matching.
		/// \return An instance of a synthesizer.
		static std::unique_ptr<SynthesizerBase> createSynthesizer(std::shared_ptr<ISearchIndex> catalog);
	};

	/// \brief Implements a parallel, pyramid-bases, non-parametric, per-pixel synthesizer.
	///
	/// The synthesizer implements an algorithm, that uses an indexed exemplar and synthesizes a result sample by traversing an image pyramid. The algorithm follows the
	/// one, that has originally been described by Sylvain Lefebvre and Hugues Hoppe. Note that this is the parallel implementation of the `PyramidSynthesizer`.
	///
	/// Lefebvre and Hoppe originally based their synthesizer on image pyramids and later introduced an hierarchy, called "gaussian stack", that, instead of traversing
	/// a pyramidal hierarchy, uses a stack of increasingly blurred samples. This implementation does not use the stack, but instead uses simple pyramids.
	///
	/// **Example**
	///
	/// The following example shows how to create a synthesizer and synthesize a new texture from an indexed exemplar. To use the parallel implementation, change the call
	/// to `Texturize::PyramidSynthesizer::createSynthesizer` to the `Texturize::ParallelPyramidSynthesizer::createSynthesizer` factory method.
	/// \include PyramidSynthesizer.cpp
	/// 
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Parallel Controllable Texture Synthesis." In: ACM Trans. Graph. 24.3 (July 2005), pp. 777-786. issn: 0730-0301. doi: 10.1145/1073204.1073261. url: http://doi.acm.org/10.1145/1073204.1073261
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Appearance-space Texture Synthesis." In: ACM Trans. Graph. 25.3 (July 2006), pp. 541-548. issn: 0730-0301. doi: 10.1145/1141911.1141921. url: http://doi.acm.org/10.1145/1141911.1141921. 
	class TEXTURIZE_API ParallelPyramidSynthesizer :
		public PyramidSynthesizer
	{
	private:
		ParallelPyramidSynthesizer(std::shared_ptr<ISearchIndex> catalog) :
			PyramidSynthesizer(std::move(catalog)) 
		{ 
		}

	protected:
		void upsample(cv::Mat& sample, const PyramidSynthesizerState& state) const override;
		void jitter(cv::Mat& sample, const PyramidSynthesizerState& state) const override;
		void correct(cv::Mat& sample, const PyramidSynthesizerState& state) const override;
		void transferTo(const Sample& target, Sample& result, const PyramidSynthesizerState& state) const override;

	public:
		/// \brief A factory method that creates a new synthesizer and initializes with a search index, that provides access to exemplar neighborhoods.
		/// \param catalog An search index, that provides access to exemplar neighborhoods and provides runtime pixel neighborhood matching.
		/// \return An instance of a synthesizer.
		static std::unique_ptr<SynthesizerBase> createSynthesizer(std::shared_ptr<ISearchIndex> catalog);
	};

	// class TEXTURIZE_API GPUPyramidSynthesizer : public PyramidSynthesizer { }

	/// @}
};