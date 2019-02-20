#pragma once

#include <texturize.hpp>

#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <utility>
#include <functional>
#include <initializer_list>

#include <opencv2\core.hpp>
#include <opencv2\imgproc.hpp>
#include <opencv2\ximgproc.hpp>

/// \namespace Texturize
/// \brief The root namespace, that contains all framework classes.
namespace Texturize {

	/// \defgroup analysis Image Analysis
	/// Contains components used during Image Analysis phase. For more information see \ref index.
	/// @{

	/// \brief A class that provides unified access to image samples.
	///
	/// The class is used throughout the framework to simplify access to individual pixel channels, pixel neighborhoods and to wrap coordinates. It is designed as a 
	/// wrapper to OpenCV's `cv::Mat` objects and provides similar semantics, just as simple conversion functionality.
	///
	/// The basic unit for working with samples is called *texel*. A texel represents an individual pixel within the sample, i.e. a vector of float values, that can
	/// be accessed either by absolute integer coordinates `x` and `y` or by continuous floating point `u` and `v` coordinates. The information stored by one texel
	/// is only meaningful in the scope of an application, e.g. when loading a RGBA image, a texel is represented by a 4-dimensional vector, containing the R, G, B
	/// and A values of a pixel.
	///
	/// **Example**
	///
	/// The following example demonstrates how to create, initialize and convert `Sample` instances.
	/// \include SampleConstruction.cpp
	///
	/// **Example**
	///
	/// The following example demonstrates how to acces texels and their neighborhoods.
	/// \include SamplePixelAccess.cpp
	///
	/// **Example**
	///
	/// The following example demonstrates how to remap an exemplar using an uv map.
	/// \include SampleMapping.cpp
	///
	/// \see Texturize::Sample_
	/// \see https://docs.opencv.org/master/d3/d63/classcv_1_1Mat.html
	class TEXTURIZE_API Sample 
	{
	public:
		/// \brief Defines a texel as a vector of floating-point values.
		typedef std::vector<float> Texel;

	protected:
		/// \brief A vector that stores each channel of the sample as a separate `cv::Mat` object.
		std::vector<cv::Mat> _channels = { cv::Mat() };

	public:
		/// \brief Creates a new `Sample` instance.
		Sample() = default;

		/// \brief Creates a new `Sample` instance.
		/// \param raw An matrix that may contain one or multiple channels to initialize the sample with.
		explicit Sample(const cv::Mat& raw);

		/// \brief Creates a new `Sample` instance.
		/// \param channels The number of channels to initialize the sample with.
		explicit Sample(const size_t channels);

		/// \brief Creates a new `Sample` instance.
		/// \param channels The number of channels to initialize the sample with.
		/// \param size The size of the sample.
		Sample(const size_t channels, const cv::Size& size);

		/// \brief Creates a new `Sample` instance.
		/// \param channels The number of channels to initialize the sample with.
		/// \param width The width of the sample.
		/// \param height The height of the sample.
		Sample(const size_t channels, const int width, const int height);

		virtual ~Sample() = default;

	protected:
		/// \brief Wraps a set of absolute coordinates, so that they are within the range of the sample size.
		/// \param x The x coordinate to wrap.
		/// \param y The y coordinate to wrap.
		///
		/// The provided \ref `x` and \ref `y` coordinates may contain any value. The method wraps them to the size of the sample, i.e. after the method
		/// returns, their value will be a valid coordinate, according to the expression \f$0 \leq x \le w\f$ and \f$0 \leq y \le h\f$, where \f$w\f$
		/// is the sample width and \f$h\f$ is the sample height.
		void wrapCoords(int& x, int& y) const;

	public:
		/// \brief Wraps a set of absolute coordinates, so that they are within the range of the sample size.
		/// \param width The width, the x coordinate should be wraped to.
		/// \param height The height, the y coordinate should be wraped to.
		/// \param x The x coordinate to wrap.
		/// \param y The y coordinate to wrap.
		///
		/// The provided \ref `x` and \ref `y` coordinates may contain any value. The method wraps them to the size of the sample, i.e. after the method
		/// returns, their value will be a valid coordinate, according to the expression \f$0 \leq x \le w\f$ and \f$0 \leq y \le h\f$, where \f$w\f$
		/// is the provided \ref `width` and \f$h\f$ is the \ref `height` parameter.
		static void wrapCoords(int width, int height, int& x, int& y);

		/// \brief Wraps a set of absolute coordinates, so that they are within the range of the sample size.
		/// \param width The width, the x coordinate should be wraped to.
		/// \param height The height, the y coordinate should be wraped to.
		/// \param coords The x and y coordinates, that should be wraped.
		///
		/// The provided \ref `coords` coordinates may contain any set of x and y coordinates. The method wraps them to the size of the sample, i.e. after 
		/// the method returns, their value will be a valid coordinate, according to the expression \f$0 \leq x \le w\f$ and \f$0 \leq y \le h\f$, where 
		/// \f$w\f$ is the provided \ref `width` and \f$h\f$ is the \ref `height` parameter.
		static void wrapCoords(int width, int height, cv::Point2i& coords);

		/// \brief Wraps a set of continuous coordinates, to a normalized range
		/// \param coords A set of continuous coordinates.
		///
		/// The provided \ref `coords` coordinates may contain any set of u and v coordinates. The method wraps them to the size of the sample, i.e. after 
		/// the method returns, their value will be a valid coordinate, according to the expression \f$0 \leq x \le 1\f$ and \f$0 \leq y \le 1\f$.
		static void wrapCoords(cv::Vec2f& coords);
		
		/// \brief Samples a provided sample using a provided uv map.
		/// \param sample The sample to map according to the `uv` map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the \ref `sample` that should be mapped to the result.
		/// \param to The `Sample` to map the \ref `sample` parameter to.
		static void sample(const Sample& sample, const cv::Mat& uv, Sample& to);

		/// \brief Samples a provided sample using a provided uv map.
		/// \param sample The sample to map according to the `uv` map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the \ref `sample` that should be mapped to the result.
		/// \param fromTo A vector that contains a mapping to mix individual channels.
		/// \param to The `Sample` to map the \ref `sample` parameter to.
		///
		/// Note, that the \ref `fromTo` vector must have a dimensionality that is a multiple of 2. Each pair of integers states from which channel in the
		/// \ref `sample` parameter to which channel in the \ref `to` parameter the mapping should be performed.
		///
		/// \see Texturize::Sample::extract
		static void sample(const Sample& sample, const cv::Mat& uv, const std::vector<int>& fromTo, Sample& to);

		/// \brief Samples a provided sample using a provided uv map.
		/// \param sample The sample to map according to the `uv` map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the \ref `sample` that should be mapped to the result.
		/// \param fromTo A vector that contains a mapping to mix individual channels.
		/// \param to The `Sample` to map the \ref `sample` parameter to.
		///
		/// Note, that the \ref `fromTo` vector must have a dimensionality that is a multiple of 2. Each pair of integers states from which channel in the
		/// \ref `sample` parameter to which channel in the \ref `to` parameter the mapping should be performed.
		///
		/// \see Texturize::Sample::extract
		static void sample(const Sample& sample, const cv::Mat& uv, std::initializer_list<int> fromTo, Sample& to);

	public:
		/// \brief Overwrites a single channel.
		/// \param index The index of the channel to overwrite.
		/// \param channel A matrix with a depth of 1 and a size equal to the current sample, that contains the pixel values to overwrite the current channel with.
		virtual void setChannel(const int index, const cv::Mat& channel);

		/// \brief Copies a channel from the current sample.
		/// \param index The index of the channel to copy.
		/// \param channel A buffer to copy the channel to.
		virtual void copyChannel(const int index, cv::Mat& channel) const;

		/// \brief Returns a channel from the current sample.
		/// \param index The index of the channel to return.
		/// \returns A copy of the channel, identified by \ref `index`.
		virtual cv::Mat getChannel(const int index) const;

		/// \brief Sets the values of a single texel within the current sample.
		/// \param at A set of absolute coordinates that address the texel.
		/// \param texel A vector of floating point values to overwrite the current texel values with. The vector must have the same dimensionality as the sample.
		virtual void setTexel(const cv::Point2i& at, const std::vector<float>& texel);

		/// \brief Sets the values of a single texel within the current sample.
		/// \param x The absolute x coordinate of the texel.
		/// \param y The absolute y coordinate of the texel.
		/// \param texel A vector of floating point values to overwrite the current texel values with. The vector must have the same dimensionality as the sample.
		virtual void setTexel(const int x, const int y, const std::vector<float>& texel);

		/// \brief Gets the size of the current sample.
		/// \param width The width of the current sample.
		/// \param height The height of the current sample.
		virtual void getSize(int& width, int& height) const;

		/// \brief Gets the size of the current sample.
		/// \param size The size of the current sample.
		virtual void getSize(cv::Size& size) const;

		/// \brief Returns the number of channels of the current sample.
		/// \returns The number of channels of the current sample.
		virtual size_t channels() const;

		/// \brief Returns the size of the current sample.
		/// \returns The size of the current sample.
		virtual cv::Size size() const;

		/// \brief Returns the width of the current sample.
		/// \returns The width of the current sample.
		virtual int width() const;

		/// \brief Returns the height of the current sample.
		/// \returns The height of the current sample.
		virtual int height() const;

		/// \brief Gets a texel at a specified location.
		/// \param x The x coordinate of the texel to return.
		/// \param y The y coordinate of the texel to return.
		/// \param The texel identified by the x and y coordinates.
		void at(int& x, int& y, Texel& texel) const;

		/// \brief Gets a texel at a specified location.
		/// \param p The x and y coordinates of the texel to return.
		/// \param The texel identified by the x and y coordinates.
		void at(cv::Point& p, Texel& texel) const;

		/// \brief Gets a texel at a specified location.
		/// \param u The u coordinate of the texel to return.
		/// \param v The v coordinate of the texel to return.
		/// \param The texel identified by the x and y coordinates.
		void at(const float& u, const float& v, Texel& texel) const;

		/// \brief Gets a texel at a specified location.
		/// \param uv The u and v coordinates of the texel to return.
		/// \param The texel identified by the x and y coordinates.
		void at(const cv::Vec2f& uv, Texel& texel) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo A pointer to an array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param pairs The number of pairs within the \ref `fromTo` array.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		void extract(const int* fromTo, const size_t pairs, Sample& sample) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		void extract(const std::vector<int>& fromTo, Sample& sample) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		void extract(std::initializer_list<int> fromTo, Sample& sample) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo A pointer to an array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param pairs The number of pairs within the \ref `fromTo` array.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		///
		/// \deprecated Note that the method is deprecated and will be removed in future releases. Consider using \ref `extract` instead.
		/// \see Texturize::Sample::extract
		void map(const int* fromTo, const size_t pairs, const Sample& sample);

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		///
		/// \deprecated Note that the method is deprecated and will be removed in future releases. Consider using \ref `extract` instead.
		/// \see Texturize::Sample::extract
		void map(const std::vector<int>& fromTo, const Sample& sample);

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		///
		/// \deprecated Note that the method is deprecated and will be removed in future releases. Consider using \ref `extract` instead.
		/// \see Texturize::Sample::extract
		void map(std::initializer_list<int> fromTo, const Sample& sample);

		/// \brief Merges the current sample with another one to a buffer.
		/// \param with The sample to merge the current instance with.
		/// \param to The buffer to merge both samples to.
		///
		/// When merging two samples, the current instance copies all its channels to the buffer and appends the channels of the \ref `with` parameter.
		void merge(const Sample& with, Sample& to) const;

		/// \brief Returns the neighborhood of a texel.
		/// \param x The x coordinate of the texel.
		/// \param y the y coordinate of the texel.
		/// \param kernel The size of the neighborhood window. Must be an odd value.
		/// \param v The buffer to copy the neighborhood to.
		/// \param weight A boolean value that toggles gaussian weighting of neighborhood values, depending on the distance to the texel identified by the provided coordinates.
		/// 
		/// If the pixel coordinates address a texel at the border of the sample, the method will automatically wrap the coordinates. Other border modes are currently unsupported.
		/// The result texel \ref `v` is a high-dimensional vector, that contains all neighborhood values in row-major order. If the sample has multiple channels, all channels
		/// are copied, before moving to the next pixel.
		void getNeighborhood(const int x, const int y, const int kernel, Texel& v, const bool weight = false) const;

		/// \brief Returns the neighborhood of a texel.
		/// \param p The x and y coordinates of the texel.
		/// \param kernel The size of the neighborhood window. Must be an odd value.
		/// \param v The buffer to copy the neighborhood to.
		/// \param weight A boolean value that toggles gaussian weighting of neighborhood values, depending on the distance to the texel identified by the provided coordinates.
		/// 
		/// If the pixel coordinates address a texel at the border of the sample, the method will automatically wrap the coordinates. Other border modes are currently unsupported.
		/// The result texel \ref `v` is a high-dimensional vector, that contains all neighborhood values in row-major order. If the sample has multiple channels, all channels
		/// are copied, before moving to the next pixel.
		void getNeighborhood(const cv::Point& p, const int kernel, Texel& v, const bool weight = false) const;

		/// \brief Clones the current sample to a specified buffer.
		/// \param s The buffer to clone the current sample to.
		void clone(Sample& s) const;

		/// \brief Clones the current sample to a specified buffer.
		/// \param s The buffer to clone the current sample to.
		void clone(Sample** const s) const;

		/// \brief Clones the current sample to a specified buffer.
		/// \returns The buffer to clone the current sample to.
		Sample clone() const;

		/// \brief Multiplies each pixel value by a constant weight.
		/// \param weight The weight to multiply all pixel values with.
		void weight(const float weight);

		/// \brief Samples the current sample using a provided uv map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the current sample.
		/// \param to The buffer to map the current instance to.
		void sample(const cv::Mat& uv, Sample& to) const;

		/// \brief Samples the current sample using a provided uv map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the current sample.
		/// \param fromTo A vector that contains a mapping to mix individual channels.
		/// \param to The buffer to map the current instance to.
		///
		/// \see Texturize::Sample::extract
		void sample(const cv::Mat& uv, const std::vector<int>& fromTo, Sample& to) const;

		/// \brief Samples the current sample using a provided uv map.
		/// \param uv A two-dimensional uv map, where a pixel contains the u and v coordinate of the current sample.
		/// \param fromTo A vector that contains a mapping to mix individual channels.
		/// \param to The buffer to map the current instance to.
		///
		/// \see Texturize::Sample::extract
		void sample(const cv::Mat& uv, std::initializer_list<int> fromTo, Sample& to) const;

	public:
		/// \brief Converts the current sample to a `cv::Mat` object.
		/// 
		/// **Example**
		/// \code
		/// Texturize::Sample sample(4, 512, 512);	// Create a 512x512 pixel sample, containing 4 channels.
		/// cv::Mat mat = (cv::Mat)sample;			// Convert to a 512x512 cv::Mat object with a depth of 4.
		/// \endcode
		explicit operator cv::Mat() const;

	public:
		/// \brief Merges multiple input samples into a single `Sample` instance.
		/// \param samples A list of samples to merge.
		/// \returns A single `Sample` instance, containing all channels of all input samples.
		static Sample mergeSamples(std::initializer_list<const Sample>& samples);
	};

	/// \brief A class that provides unified access to image samples.
	/// \tparam cn The number of channels of the sample.
	///
	/// This class directly inherits from \ref `Sample` and provides the same semantics. It is designed for cases, where the number of channels of a sample is known
	/// at compile time. For example, UV maps allways require a set of 2 channels. In such cases, the template can be used to efficiently pre-allocate memory.
	///
	/// \see Texturize::Sample
	template <size_t cn> 
	class TEXTURIZE_API Sample_ : 
		public Sample
	{
	public:
		/// \brief Defines a type for a `Sample` with a certain number of channels.
		typedef Sample_<cn> SampleType;

		/// \brief Defines a type for a texel with a certain number of channels.
		typedef cv::Vec<float, cn> Texel;

	public:
		/// \brief Creates a new `Sample_` instance.
		Sample_();

		/// \brief Creates a new `Sample_` instance.
		/// \param raw An matrix that may contain one or multiple channels to initialize the sample with.
		explicit Sample_(const cv::Mat& raw);

		/// \brief Creates a new `Sample_` instance.
		/// \param size The size of the sample.
		explicit Sample_(const cv::Size& size);

		/// \brief Creates a new `Sample_` instance.
		/// \param width The width of the sample.
		/// \param height The height of the sample.
		Sample_(const int width, const int height);

		virtual ~Sample_() = default;

	public:
		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo A pointer to an array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param pairs The number of pairs within the \ref `fromTo` array.
		/// \param sample The sample to map the channels to.
		/// \tparam _cn The number of channels of the target sample.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		template <size_t _cn> 
		void extract(const int* fromTo, const size_t pairs, Sample_<_cn>& sample) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		/// \tparam _cn The number of channels of the target sample.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		template <size_t _cn>
		void extract(const std::vector<int>& fromTo, Sample_<_cn>& sample) const;

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo A pointer to an array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param pairs The number of pairs within the \ref `fromTo` array.
		/// \param sample The sample to map the channels to.
		/// \tparam _cn The number of channels of the target sample.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		///
		/// \deprecated Note that the method is deprecated and will be removed in future releases. Consider using \ref `extract` instead.
		/// \see Texturize::Sample::extract
		template <size_t _cn>
		void map(const int* fromTo, const size_t pairs, const Sample_<_cn>& sample);

		/// \brief Maps a subset of channels from the current sample to another sample.
		/// \param fromTo An array of channel indices, containing a set of pairs that identify a mapping between the channels.
		/// \param sample The sample to map the channels to.
		/// \tparam _cn The number of channels of the target sample.
		///
		/// The \ref `fromTo` array must contain a even number of values, where each pair identifies the source and target index of the channel mapping.
		///
		/// \deprecated Note that the method is deprecated and will be removed in future releases. Consider using \ref `extract` instead.
		/// \see Texturize::Sample::extract
		template <size_t _cn>
		void map(const std::vector<int>& fromTo, const Sample_<_cn>& sample);

		/// \brief Returns the neighborhood of a texel.
		/// \param x The x coordinate of the texel.
		/// \param y the y coordinate of the texel.
		/// \param v The buffer to copy the neighborhood to.
		/// \param weight A boolean value that toggles gaussian weighting of neighborhood values, depending on the distance to the texel identified by the provided coordinates.
		/// \tparam _sk The size of the neighborhood window. Must be an odd value.
		/// 
		/// If the pixel coordinates address a texel at the border of the sample, the method will automatically wrap the coordinates. Other border modes are currently unsupported.
		/// The result texel \ref `v` is a high-dimensional vector, that contains all neighborhood values in row-major order. If the sample has multiple channels, all channels
		/// are copied, before moving to the next pixel.
		template <size_t _sk>
		void getNeighborhood(const int x, const int y, cv::Vec<float, _sk * cn>& v, const bool weight = false) const;

		/// \brief Returns the neighborhood of a texel.
		/// \param p The x and y coordinates of the texel.
		/// \param v The buffer to copy the neighborhood to.
		/// \param weight A boolean value that toggles gaussian weighting of neighborhood values, depending on the distance to the texel identified by the provided coordinates.
		/// \tparam _sk The size of the neighborhood window. Must be an odd value.
		/// 
		/// If the pixel coordinates address a texel at the border of the sample, the method will automatically wrap the coordinates. Other border modes are currently unsupported.
		/// The result texel \ref `v` is a high-dimensional vector, that contains all neighborhood values in row-major order. If the sample has multiple channels, all channels
		/// are copied, before moving to the next pixel.
		template <size_t _sk>
		void getNeighborhood(const cv::Point& p, cv::Vec<float, _sk * cn>& v, const bool weight = false) const;
	};

	/// \brief A base class for filter functions that can be applied to `Sample` instances.
	///
	/// Filters are functions that are applied to each pixel of a `Sample`. The result is then stored in a new sample instance. Although this can be implemented inline, this 
	/// interface is used to abstract them, enabling them to be cascaded.
	///
	/// \see Texturize::FilterCascade
	class TEXTURIZE_API IFilter
	{
		/// \example BlurFilter.cpp The example demonstrates how to create and use a custom filter implementation. The filter uses OpenCV to apply gaussian blur with a kernel of 5 to an input image.

	public:
		virtual ~IFilter() = default;

	public:
		/// \brief Applies the filter to \ref `sample` and stores the result in \ref `result`.
		/// \param result The result of the operation.
		/// \param sample The sample to apply the filter function to.
		virtual void apply(Sample& result, const Sample& sample) const = 0;
	};

	/// \brief A filter that accepts a function or a lambda that is applied to each pixel of the input sample.
	/// \tparam TResult The type of the result of the filter function.
	/// \tparam TSample The type of the input sample of the filter function.
	///
	/// **Example**
	///
	/// The class can be used to create custom filters in an inline fashion. The example demonstrates how to use a lambda expression to create a mean filter.
	/// \include FilterFunction.cpp
	template <typename TResult = Sample, typename TSample = Sample>
	class TEXTURIZE_API FunctionFilter :
		public IFilter
	{
		static_assert(are_base_of<Sample, TResult, TSample>::value, "The input and output samples of a filter must implement the 'Sample' class.");

	public:
		/// \brief A type that defines the filter.
		typedef FunctionFilter<TResult, TSample> FilterType;
		
		/// \brief The type of the result sample.
		typedef TResult ResultType;
		
		/// \brief The type of the input sample.
		typedef TSample SampleType;

		/// \brief The type that defines the layout for the filter function.
		typedef std::function<ResultType(const SampleType&)> Functor;

	protected:
		/// \brief The function to execute on each pixel of the input sample.
		Functor _functor;

	public:
		/// \brief Creates a new `FilterFunction` instance.
		/// \param fn The function to execute on each pixel of the input sample.
		FunctionFilter(Functor fn);

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;

		/// \copydoc Texturize::IFilter::apply
		template <typename TRes = TResult, typename TSmpl = TSample, typename std::enable_if<
			!std::is_same<TRes, Sample>::value && !std::is_same<TSmpl, Sample>::value>::type = 0>
		void apply(TRes& result, const TSmpl& sample) const
		{
			result = this->_functor(sample);
		}
	};

	/// \brief A filter that can be used to execute multiple filters as a cascade.
	class TEXTURIZE_API FilterCascade : 
		public IFilter
	{
	protected:
		/// \brief Stores the filters to execute in the order they have been appended.
		std::queue<const IFilter*> _filters;

	public:
		virtual ~FilterCascade();

	public:
		/// \copydoc Texturize::IFilter::apply
		///
		/// The method executes each filter in the cascade in the order they have been appended.
		void apply(Sample& result, const Sample& sample) const override;

		/// \brief Appends a filter to the cascade.
		/// \param filter The filter to append to the cascade.
		void append(const IFilter* filter);
	};
	
	/// \brief The base class for filters that are capable of detecting edges.
	class TEXTURIZE_API EdgeDetector :
		public IFilter
	{
	};

	/// \brief Implements an edge detection filter, based on structured forrests.
	///
	/// The original algorithm has been described by Dollar and Zitnick. The model used for the edge detector has originally been trained by Kaspar et al. The 
	/// implementation uses the `StructuredEdgeDetection` implementation by OpenCV's ximgproc library. Custom models can be provided in the constructor.
	/// 
	/// \see Piotr Dollar and Larry Zitnick. "Structured Forests for Fast Edge Detection." in: International Conference on Computer Vision, Dec. 2013, url: https://www.microsoft.com/en-us/research/publication/structured-forests-for-fast-edge-detection/
	/// \see Alexandre Kaspar et al. "Self Tuning Texture Optimization." in: Computer Graphics Forum 34.2 (May 2015), pp. 349-359, issn: 0167-7055, doi: 10.1111/cgf.12565, url: http://dx.doi.org/10.1111/cgf.12565
	class TEXTURIZE_API StructuredEdgeDetector :
		public EdgeDetector
	{
	private:
		cv::Ptr<cv::ximgproc::StructuredEdgeDetection> _detector;

	public:
		/// \brief Creates a new edge detector instance.
		/// \param modelName The name of the model used for edge detection.
		StructuredEdgeDetector(const std::string& modelName);

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements an dynamic threshold filter.
	/// 
	/// The implementation uses a dynamic thresold, as described by Nobuyuki Otsu. It is based on OpenCV's `cv::thresold` method, parameterized with the `CV_THRESH_BINARY` and 
	/// `CV_THRESH_OTSU` flags
	///
	/// \see Nobuyuki Otsu. "A threshold selection method from gray-level histograms." In: IEEE transactions on systems, man, and cybernetics 9.1 (1979), pp. 62-66
	class TEXTURIZE_API DynamicThresholdFilter :
		public IFilter
	{
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements a euclidean distance filter.
	///
	/// The filter calculates the L2 distance for each pixel from a binarized image. The result is normalized in a range from 0 to 255.
	class TEXTURIZE_API FeatureDistanceFilter :
		public IFilter
	{
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements a normalization filter.
	///
	/// The filter normalizes an image to a range from 0.0 to 1.0.
	class TEXTURIZE_API NormalizationFilter :
		public IFilter
	{
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements a grayscale filter.
	class TEXTURIZE_API GrayscaleFilter :
		public IFilter
	{
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements a gaussian blur filter.
	class TEXTURIZE_API GaussianBlurFilter :
		public IFilter
	{
	private:
		float _deviation;
		int _kernel;

	public:
		/// \brief Creates a new gaussian blur filter.
		/// \param deviation The standard deviation \f$ \sigma \f$ that defines the amount of blur.
		GaussianBlurFilter(const float& deviation);

		/// \brief Creates a new gaussian blur filter.
		/// \param deviation The standard deviation \f$ \sigma \f$ that defines the amount of blur.
		/// \param kernel The size of the blur kernel into each direction.
		GaussianBlurFilter(const float& deviation, const int& kernel);

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief Implements a filter that calculates a *feature map*
	///
	/// A feature map is the result of the consecutive execution of an \ref `EdgeDetector` and a `FeatureDistanceFilter`. The result describes the distance
	/// of each pixel towards the closest edge and can be used to further guide synthesis algorithms by supporting feature formation.
	class TEXTURIZE_API FeatureExtractor :
		public IFilter
	{
	private:
		FilterCascade _cascade;

	public:
		/// \brief Creates a new feature extractor instance.
		FeatureExtractor();

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	class TEXTURIZE_API HistogramMatchingFilter :
		public IFilter 
	{
	private:
		/// \brief The cummulative probability distribution function for the reference sample, that is used to match the target histogram against.
		cv::Mat _referenceCdf;

	public:
		/// \brief Creates a new histogram equalization filter.
		/// \param referemceSample The sample to match the target histogram against.
		HistogramMatchingFilter(const Sample& referenceSample);

	private:
		cv::Mat toGrayscale(const Sample& sample) const;
		cv::Mat sampleCdf(const Sample& sample) const;
		cv::Mat sampleCdf(const Sample& sample, cv::Mat& grayscale) const;
		cv::Mat cummulate(const cv::Mat& histogram) const;

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	class TEXTURIZE_API HistogramExtractionFilter :
		public IFilter
	{
	private:
		const int _binsPerDimension;

	public:
		HistogramExtractionFilter(const int& binsPerDim = 4);
		virtual ~HistogramExtractionFilter() = default;

	private:
		cv::Mat mask(const cv::Size& sampleSize, const cv::Point2i& at, const int kernel, const bool wrap = true) const;

	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	/// \brief 
	class TEXTURIZE_API IFilterBank {
	public:
		/// \brief
		///
		/// 
		///
		/// \param bank A sample, that receives all kernels of the filter bank.
		virtual void computeRootFilterSet(Sample& bank, const int kernelSize = 49) const = 0;
	};

	/// 
	class TEXTURIZE_API MaxResponseFilterBank :
		public IFilterBank,
		public IFilter
	{
	private:
		const Sample _rootFilterSet;

	public:
		MaxResponseFilterBank(const int kernelSize = 49);
		virtual ~MaxResponseFilterBank() = default;

	private:
		Sample computeRootFilterSet(const int kernelSize) const;

		// IFilterBank
	public:
		void computeRootFilterSet(Sample& bank, const int kernelSize = 49) const override;

		// IFilterBank
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	class TEXTURIZE_API INoiseFunction {
	public:
		virtual ~INoiseFunction() = default;

	public:
		virtual void makeNoise(Sample& sample) const = 0;
	};

	class TEXTURIZE_API PerlinNoise2D :
		public INoiseFunction,
		public IFilter {
	public:
		virtual ~PerlinNoise2D() = default;

		// INoiseFunction
	public:
		void makeNoise(Sample& sample) const override;

		// IFilter
	public:
		/// \copydoc Texturize::IFilter::apply
		void apply(Sample& result, const Sample& sample) const override;
	};

	class TEXTURIZE_API MatchingVarianceNoise :
		public PerlinNoise2D {
	private:
		const std::vector<float> _referenceVariance;

	public:
		MatchingVarianceNoise() = delete;
		MatchingVarianceNoise(const float referenceVariance);
		MatchingVarianceNoise(const std::vector<float>& referenceVariances);
		virtual ~MatchingVarianceNoise() = default;

		// INoiseFunction
	public:
		void makeNoise(Sample& sample) const override;

	public:
		static std::unique_ptr<IFilter> FromSample(const Sample& sample);
	};

	/// \brief An interface that defines methods to create, transform and work with search spaces.
	///
	/// Basically, a search space is the result of image analysis. It is used later during synthesis in order to match pixel or patch neighborhoods. This interface
	/// describes the base for different possible search space implementations. Naive sampling approaches match pixel neighborhoods based on their color values, so
	/// there is not much analysis required. More advanced approaches utilize complex search space implementations.
	///
	/// The search space itself describes pixels and pixel neighborhoods as a `Sample`. Synthesis algorithms do not directly access the search space, but instead
	/// have to create an index over the search space. The details of this process is described under \ref `Texturize::ISearchIndex`.
	///
	/// \see Texturize::ISearchIndex
	/// \see Texturize::AppearanceSpace
	class TEXTURIZE_API ISearchSpace
	{
	public:
		/// \brief Projects a texel into the search space.
		/// \param texel The pixel values that should be projected into the search space.
		/// \param desc A descriptor, that describes the projected texel.
		virtual void transform(const std::vector<float>& texel, std::vector<float>& desc) const = 0;

		/// \brief Projects a texel into the search space.
		/// \param sample The sample, containing the texel that should be projected.
		/// \param x The x coordinate of the texel to project.
		/// \param y The y coordinate of the texel to project.
		/// \param desc A descriptor, that describes the projected texel.
		virtual void transform(const Sample& sample, const int x, const int y, std::vector<float>& desc) const = 0;

		/// \brief Projects a texel into the search space.
		/// \param sample The sample, containing the texel that should be projected.
		/// \param texelCoords The x and y coordinates of the texel to project.
		/// \param desc A descriptor, that describes the projected texel.
		virtual void transform(const Sample& sample, const cv::Point& texelCoords, std::vector<float>& desc) const = 0;

		virtual void transform(const Sample& sample, Sample& to, const int ks = 5) const = 0;

		/// \brief Gets a copy of the sample, containing search space descriptors.
		/// \param sample A buffer to copy the sample to.
		virtual void sample(Sample& sample) const = 0;

		/// \brief Gets a reference of the sample, containing search space descriptors.
		/// \param sample A pointer to store the sample reference to.
		virtual void sample(const Sample** const sample) const = 0;

		/// \brief Gets the kernel used to calculate the search space neighborhoods.
		/// \param kernel A reference, the kernel gets stored to.
		virtual void kernel(int& kernel) const = 0;

		/// \brief Gets the size of the sample used to build up the search space.
		/// \param size A reference, the size gets stored to.
		virtual void sampleSize(cv::Size& size) const = 0;

		/// \brief Gets the size of the sample used to build up the search space.
		/// \param width A reference, the width gets stored to.
		/// \param height A reference, the height gets stored to.
		virtual void sampleSize(int& width, int& height) const = 0;
	};

	// class TEXTURIZE_API ColorSpace : public ISearchSpace { };

	/// \brief A search space implementation based on pixel neighborhood appearances.
	///
	/// The *Appearance Space* has been first described by Sylvain Lefebvre and Hugues Hoppe and describes a search space, based on pixel neighborhood appearances, rather than 
	/// color values. In their work they have shown that matching color values often results in poor quality. Furthermore, matching complex neighborhoods in a high-dimensional
	/// search space is a very time-consuming task.
	///
	/// The appearance space uses gaussian weighting for a 5 pixel kernel neighborhood and uses all sample neighborhoods to calculate principal components using *principal
	/// component analysis* (PCA). This projection allows to reduce the dimensionality of neighborhoods. The same projection matrix can be used during synthesis to speed
	/// up runtime descriptor extraction. For more information see \ref `Texturize::DescriptorExtractor`.
	///
	/// The following figure describes the principle of reducing neighborhood dimensionality.
	///
	/// \image html pca.svg
	///
	/// **Example**
	///
	/// The following example shows how to calculate the appearance space for an exemplar sample.
	/// \include AppearanceSpace.cpp
	///
	/// **Example**
	/// 
	/// The following example shows how to add guidance channels to the appearance space to further improve synthesis quality by using feature maps to support feature formation.
	/// \include FeatureGuidance.cpp
	///
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Parallel Controllable Texture Synthesis." In: ACM Trans. Graph. 24.3 (July 2005), pp. 777-786. issn: 0730-0301. doi: 10.1145/1073204.1073261. url: http://doi.acm.org/10.1145/1073204.1073261
	/// \see Sylvain Lefebvre and Hugues Hoppe. "Appearance-space Texture Synthesis." In: ACM Trans. Graph. 25.3 (July 2006), pp. 541-548. issn: 0730-0301. doi: 10.1145/1141911.1141921. url: http://doi.acm.org/10.1145/1141911.1141921. 
	/// \see Texturize::ISearchIndex
	/// \see Texturize::DescriptorExtractor
	/// \see Texturize::SearchIndex
	class TEXTURIZE_API AppearanceSpace :
		public ISearchSpace
		//public cv::DescriptorExtractor
	{
		/// \example ColorSearchSpace.cpp The example demonstrates how to implement a custom search space, that builds simple neighborhood descriptors from pixel color values.

	private:
		const cv::PCA* _projection;
		const std::unique_ptr<Sample> _exemplar;
		const int _kernelSize;

	public:
		/// \brief Initializes a new appearance space instance.
		/// \param The projection matrix used to transform texel neighborhoods into the search space.
		/// \param exemplar The exemplar sample that has been used to calculate the projection.
		/// \param kernelSize The size of the pixel neighborhood that has been used to calculate the projection.
		///
		/// Note: typically you do not want to directly construct an `AppearanceSpace` instance. Instead use the \ref `Texturize::AppearanceSpace::calculate` methods
		/// to get a newly created instance or load them from a persistent asset.
		AppearanceSpace(const cv::PCA* projection, Sample* exemplar, const int kernelSize);

	protected:
		/// \brief Returns a matrix, containing all the pixel neighborhoods of the provided exemplar.
		/// \param exemplar The exemplar sample, of which the pixel neighborhoods should be extracted.
		/// \param kernel The kernel size of the neighborhood window.
		/// \returns A column-major matrix containing high-dimensional vectors, that describe a pixel neighborhood. Each column represents a single pixel of the input exemplar.
		static cv::Mat getComponents(const Sample& exemplar, int kernel);

	public:
		/// \brief Calculates the appearance space for an individual exemplar.
		/// \param exemplar The exemplar sample to calculate the seach space for.
		/// \param desc A pointer that will be initialized with the calculated search space instance.
		/// \param resultDim The number of dimensionality retained after transforming pixel neighborhoods into search space.
		/// \param kernel The size of the kernel window used to extract search space neighborhoods.
		static void calculate(const Sample& exemplar, AppearanceSpace** desc, size_t resultDim = 3, int kernelSize = 5);

		/// \brief Calculates the appearance space for an individual exemplar.
		/// \param exemplar The exemplar sample to calculate the seach space for.
		/// \param desc A pointer that will be initialized with the calculated search space instance.
		/// \param targetVariance The variance retained by the search space transform.
		/// \param kernel The size of the kernel window used to extract search space neighborhoods.
		static void calculate(const Sample& exemplar, AppearanceSpace** desc, float targetVariance = 0.9f, int kernelSize = 5);

		/// \brief Calculates the appearance space for a set of exemplar samples.
		/// \param exemplar The exemplar samples to calculate the seach space for.
		/// \param desc A pointer that will be initialized with the calculated search space instance.
		/// \param resultDim The number of dimensionality retained after transforming pixel neighborhoods into search space.
		/// \param kernel The size of the kernel window used to extract search space neighborhoods.
		static void calculate(std::initializer_list<const Sample> exemplarMaps, AppearanceSpace** desc, size_t resultDim, int kernelSize = 5);

		/// \brief Calculates the appearance space for a set of exemplar samples.
		/// \param exemplar The exemplar samples to calculate the seach space for.
		/// \param desc A pointer that will be initialized with the calculated search space instance.
		/// \param targetVariance The variance retained by the search space transform.
		/// \param kernel The size of the kernel window used to extract search space neighborhoods.
		static void calculate(std::initializer_list<const Sample> exemplarMaps, AppearanceSpace** desc, float targetVariance = 0.9f, int kernelSize = 5);

	public:
		/// \brief Gets a reference of the projection matrix used to project pixel neighborhoods into the search space.
		/// \param projection A pointer to a reference that will contain the projection matrix after the method returns.
		void getProjector(const cv::PCA** projection) const;

		/// \brief Gets a copy of the sample, containing search space descriptors.
		/// \param exemplar A pointer to a sample that will be initialized with a copy of the exemplar descriptor sample.
		///
		/// \see Texturize::ISearchSpace::sample
		void getExemplar(const Sample** exemplar) const;

		/// \brief Gets the kernel used to get the exemplar sample neighborhood descriptors.
		/// \param kernel The kernel used to get the exemplar sample neighborhood descriptors.
		///
		/// \see Texturize::ISearchSpace::kernel
		void getKernel(int& kernel) const;

		// ISearchSpace
	public:
		void transform(const std::vector<float>& pixelNeighborhood, std::vector<float>& desc) const override;
		void transform(const Sample& sample, const int x, const int y, std::vector<float>& desc) const override;
		void transform(const Sample& sample, const cv::Point& texelCoords, std::vector<float>& desc) const override;
		void transform(const Sample& sample, Sample& to, const int ks = 5) const override;
		void sample(Sample& sample) const override;
		void sample(const Sample** const sample) const override;
		void kernel(int& kernel) const override;
		void sampleSize(cv::Size& size) const override;
		void sampleSize(int& width, int& height) const override;
	};

	class TEXTURIZE_API IImagePyramid {
	public:
		virtual void construct(const Sample& sample, const unsigned int levels) = 0;
		virtual void reconstruct(Sample& to, const unsigned int toLevel = 0) = 0;
	};
	
	class TEXTURIZE_API ImagePyramid :
		public IImagePyramid
	{
	protected:
		std::vector<Sample> _levels;
		
	public:
		Sample getLevel(const unsigned int level) const;
		void filterLevel(std::unique_ptr<IFilter>& filter, const unsigned int level);
	};

	class TEXTURIZE_API GaussianImagePyramid :
		public ImagePyramid {
	public:
		void construct(const Sample& sample, const unsigned int levels) override;
		void reconstruct(Sample& to, const unsigned int toLevel = 0) override;
	};

	class TEXTURIZE_API LaplacianImagePyramid :
		public GaussianImagePyramid {
	public:
		void construct(const Sample& sample, const unsigned int levels) override;
		void reconstruct(Sample& to, const unsigned int toLevel = 0) override;
	};

	/// @}
}

#include "sample.hpp"