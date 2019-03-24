#include "stdafx.h"

#include <sampling.hpp>

#include <algorithm>

#include "log2.h"

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// FLANN-based ANN search index implementation                                             /////
///////////////////////////////////////////////////////////////////////////////////////////////////

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	SearchIndex(searchSpace, descriptorExtractor, normType)
{
	this->init(*indexParams);
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	ANNIndex(searchSpace, std::make_unique<PCADescriptorExtractor>(), indexParams, normType)
{
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& weightMap, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	SearchIndex(searchSpace, descriptorExtractor, normType)
{
	this->init(*indexParams, weightMap);
}

ANNIndex::ANNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& weightMap, const cv::Ptr<const cv::flann::IndexParams> indexParams, cv::NormTypes normType) :
	ANNIndex(searchSpace, std::make_unique<PCADescriptorExtractor>(), weightMap, indexParams, normType)
{
}

void ANNIndex::init(const cv::flann::IndexParams& indexParams)
{
	this->init(indexParams, { });
}

void ANNIndex::init(const cv::flann::IndexParams& indexParams, std::optional<std::reference_wrapper<const Sample>> weightMap)
{
	// Precompute the neighborhood descriptors used to train data.
	std::shared_ptr<const Sample> sample;
	_searchSpace->sample(sample);

	int height, weightChannels(-1);
	sample->getSize(_sampleWidth, height);

	// Form a descriptor vector from the sample.
	_descriptors = _descriptorExtractor->calculateNeighborhoodDescriptors(*sample);
	TEXTURIZE_ASSERT(_descriptors.isContinuous());

	// Check if weights need to be added; if so, append them to the descriptors.
	if (weightMap.has_value()) {
		// Validate the weight map pixel count against the number of descriptors.
		auto map = weightMap.value().get();
		TEXTURIZE_ASSERT(_descriptors.rows == (map.height() * map.width()));
		weightChannels = static_cast<int>(map.channels());

		// Transpose descriptors, so that each column represents one descriptor.
		_descriptors = _descriptors.t();

		// From each channel, append the values to the descriptors.
		for (int cn(0); cn < weightChannels; ++cn)
			_descriptors.push_back(map.getChannel(cn).reshape(1, 1));

		// Again, transpose the descriptors, so that each row holds on descriptor.
		_descriptors = _descriptors.t();
	}

	// Create the index instance and initialize it.
	TMatrix dataset((TElement*)_descriptors.data, _descriptors.rows, _descriptors.cols);
	_index = std::make_unique<TIndex>(dataset, *(cvflann::IndexParams*)(indexParams.params), weightChannels);
	_index->buildIndex();
}

bool ANNIndex::findNearestNeighbor(const std::vector<float>& descriptor, MatchType& match, DistanceType minDist) const
{
	std::vector<MatchType> matches;

	// Find the k = 1 nearest neighbor.
	if (!this->findNearestNeighbors(descriptor, matches, 1, minDist))
		return false;

	// Return the match...
	match = matches.front();
	return true;
}

bool ANNIndex::findNearestNeighbors(const std::vector<float>& descriptor, std::vector<MatchType>& mtchs, const int k, DistanceType minDist) const
{
	typedef typename ANNIndex::_L2Ex<float> TDistance;
	typedef typename TDistance::ElementType TElement;
	typedef typename TDistance::ResultType  TResult;
	typedef int TIndex;

	// NOTE: One descriptor per row.
	cv::InputArray _query = descriptor;
	cv::Mat indices(1, k, CV_32SC1);
	cv::Mat distances(1, k, CV_32FC1);
	cv::Mat query = _query.getMat();

	TEXTURIZE_ASSERT(static_cast<size_t>(k) <= _index->size());
	TEXTURIZE_ASSERT(query.type() == cv::DataType<TElement>::type);
	TEXTURIZE_ASSERT(indices.type() == cv::DataType<TIndex>::type);
	TEXTURIZE_ASSERT(distances.type() == cv::DataType<TResult>::type);
	TEXTURIZE_ASSERT(query.isContinuous() && indices.isContinuous() && distances.isContinuous());

	::cvflann::Matrix<TElement> q((TElement*)query.data, query.rows, query.cols);
	::cvflann::Matrix<TIndex> i(indices.ptr<TIndex>(), indices.rows, indices.cols);
	::cvflann::Matrix<TResult> d(distances.ptr<TResult>(), distances.rows, distances.cols);

	//_index->knnSearch(descriptor, indices, distances, k);
	_index->knnSearch(q, i, d, k, ::cvflann::SearchParams());

	// Retain and store the pixel coordinates and distances for each match.
	std::vector<MatchType> matches;

	for (int _k(0); _k < k; ++_k)
	{
		int index = indices.at<int>(0, _k);
		PositionType position(
			static_cast<CoordinateType>(index % _sampleWidth) / static_cast<CoordinateType>(_sampleWidth), 
			static_cast<CoordinateType>(index / _sampleWidth) / static_cast<CoordinateType>(_sampleWidth));
		DistanceType distance{ distances.at<TResult>(0, _k) };

		matches.push_back(std::make_pair<PositionType, DistanceType>(std::move(position), std::move(distance)));
	}

	return true;
}

bool ANNIndex::findNearestNeighbor(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, MatchType& match, DistanceType minDist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbor(targetDescriptor, match, minDist);
}

bool ANNIndex::findNearestNeighbors(const cv::Mat& descriptors, const cv::Mat& uv, const cv::Point2i& at, std::vector<MatchType>& matches, const unsigned int k, DistanceType minDist) const
{
	std::vector<float> targetDescriptor = descriptors.row(at.y * uv.cols + at.x);

	return this->findNearestNeighbors(targetDescriptor, matches, k, minDist);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///// KD-Tree-based search index implementation                                               /////
///////////////////////////////////////////////////////////////////////////////////////////////////

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor) :
	ANNIndex(searchSpace, descriptorExtractor, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace) :
	ANNIndex(searchSpace, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, std::shared_ptr<IDescriptorExtractor> descriptorExtractor, const Sample& weightMap) :
	ANNIndex(searchSpace, descriptorExtractor, weightMap, cv::makePtr<const KDTreeSingleIndexParams>())
{
}

KNNIndex::KNNIndex(std::shared_ptr<ISearchSpace> searchSpace, const Sample& weightMap) :
	ANNIndex(searchSpace, weightMap, cv::makePtr<const KDTreeSingleIndexParams>())
{
}