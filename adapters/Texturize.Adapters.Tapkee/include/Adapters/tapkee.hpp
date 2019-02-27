#pragma once

// Disable C++17 compatibility warnings.
#pragma warning( disable: 4267 4244 4996 )
// Disable warnings about discarded nodiscard-return values.
#pragma warning( disable: 4834 )

#include <texturize.hpp>
#include <sampling.hpp>

// Single precision is enough for now.
#define TAPKEE_CUSTOM_INTERNAL_NUMTYPE float 

#include <tapkee/tapkee.hpp>

#include <opencv2/imgproc.hpp>

namespace Texturize {
	namespace Tapkee {

		/// \defgroup tapkee Tapkee Adapter
		/// Contains different dimensionality reductors implemented in Tapkee.
		/// @{

		class TEXTURIZE_API PCADescriptorExtractor :
			public DescriptorExtractor {
			// IDescriptorExtractor
		public:
			cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar) const override;
			cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const override;
		};

		class TEXTURIZE_API SNEDescriptorExtractor :
			public DescriptorExtractor {
			// IDescriptorExtractor
		public:
			cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar) const override;
			cv::Mat calculateNeighborhoodDescriptors(const Sample& exemplar, const cv::Mat& uv) const override;
		};

		class TEXTURIZE_API IDistanceMetric {
		public:
			// TODO: cost is actually optional.
			virtual float calculateDistance(const cv::Mat& lhs, const cv::Mat& rhs, const cv::Mat& cost) const = 0;
		};

		class TEXTURIZE_API EuclideanDistanceMetric :
			public IDistanceMetric {
		public:
			float calculateDistance(const cv::Mat& lhs, const cv::Mat& rhs, const cv::Mat& cost) const override;
		};

		class TEXTURIZE_API EarthMoversDistanceMetric :
			public IDistanceMetric {
		public:
			float calculateDistance(const cv::Mat& lhs, const cv::Mat& rhs, const cv::Mat& cost) const override;
		};

		class TEXTURIZE_API PairwiseDistanceExtractor {
		private:
			std::unique_ptr<IDistanceMetric> _distanceMetric;

		public:
			PairwiseDistanceExtractor() = delete;
			PairwiseDistanceExtractor(std::unique_ptr<IDistanceMetric> distanceMetric);
			virtual ~PairwiseDistanceExtractor() = default;

		public:
			cv::Mat computeDistances(const cv::Mat& sample) const;
			cv::Mat computeDistances(const std::vector<cv::Mat>& samples) const;
			tapkee::DenseSymmetricMatrix computeDistances(const cv::Mat& sample, std::vector<tapkee::IndexType>& indices) const;
			tapkee::DenseSymmetricMatrix computeDistances(const std::vector<cv::Mat>& sample, std::vector<tapkee::IndexType>& indices) const;
		};

		/// @}
	}
}