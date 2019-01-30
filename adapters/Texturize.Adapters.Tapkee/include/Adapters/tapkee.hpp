#pragma once

#pragma warning( disable: 4267 4244 )

#include <texturize.hpp>
#include <sampling.hpp>

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