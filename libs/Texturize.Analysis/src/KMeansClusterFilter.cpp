#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Histogram extraction filter implementation				                              /////
///////////////////////////////////////////////////////////////////////////////////////////////////

KMeansClusterFilter::KMeansClusterFilter(const int clusters, const int iterations) :
	_numClusters(clusters), _iterations(iterations)
{
}

void KMeansClusterFilter::apply(Sample& result, const Sample& sample) const
{
	cv::Mat features = ((cv::Mat)sample).reshape(1, sample.height() * sample.width());
	cv::TermCriteria terminationRule(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, _iterations, 1.0);

	//int dimensions = features.cols;
	//cv::Mat labels, centers;
	//cv::kmeans(features, _numClusters, labels, terminationRule, _iterations, cv::KMEANS_PP_CENTERS, centers);
	//
	//labels = labels.reshape(1, sample.height());
	//cv::Mat centeroids(labels.size(), CV_32FC(dimensions));
	//
	// // TODO: For each [x, y] in `labels`: Copy centers[labels[x, y]] to centeroids[x, y].

	cv::Mat labels;
	cv::kmeans(features, _numClusters, labels, terminationRule, _iterations, cv::KMEANS_PP_CENTERS);
	labels.convertTo(labels, CV_32F, 1.f / static_cast<float>(_numClusters));
	result = Sample(labels.reshape(1, sample.height()));
}