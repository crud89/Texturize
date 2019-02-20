#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Filter cascade implementation                                                           /////
///////////////////////////////////////////////////////////////////////////////////////////////////

FilterCascade::~FilterCascade()
{
	while (!_filters.empty())
	{
		delete _filters.front();
		_filters.pop();
	}
}

void FilterCascade::apply(Sample& result, const Sample& sample) const
{
	std::queue<const IFilter*> filters = _filters;
	Sample currentResult, previousResult = sample;

	while (!filters.empty())
	{
		auto filter = filters.front();
		filter->apply(currentResult, previousResult);
		filters.pop();

		previousResult = currentResult;
	}

	result = currentResult;
}

void FilterCascade::append(const IFilter* filter)
{
	_filters.push(filter);
}