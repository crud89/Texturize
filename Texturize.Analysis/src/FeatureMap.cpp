#include "Texturize.Analysis.h"

Texturize::FeatureMap::FeatureMap() : m_image(nullptr)
{
}

Texturize::FeatureMap::~FeatureMap()
{
}

bool Texturize::FeatureMap::getImage(cv::Mat& img) const
{
	if (!m_image)
		return false;

	img = m_image->clone();
	return true;
}

void Texturize::FeatureMap::load(const std::string& file)
{
	if (m_image)
		delete m_image;

	m_image = new cv::Mat(cv::imread(file));
}

void Texturize::FeatureMap::load(const cv::Mat& img)
{
	if (m_image)
		delete m_image;

	m_image = new cv::Mat(img.clone());
}

bool Texturize::FeatureMap::save(const std::string& file)
{
	return cv::imwrite(file, *m_image);
}

void Texturize::FeatureMap::fromImage(const std::string imageFile, const EdgeDetector * edgeDetector)
{
	if (!edgeDetector)
		throw AnalysisException(ERR_ARG_NULL, "The edge detector must be initialized.");

	cv::Mat edges, image = cv::imread(imageFile);
	edgeDetector->detect(image, edges);
}