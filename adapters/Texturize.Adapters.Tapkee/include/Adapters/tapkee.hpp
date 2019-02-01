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

		/// @}
	}
}