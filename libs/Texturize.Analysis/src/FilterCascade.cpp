#include "stdafx.h"

#include <analysis.hpp>

using namespace Texturize;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// Filter cascade implementation                                                           /////
///////////////////////////////////////////////////////////////////////////////////////////////////

void FilterCascade::apply(Sample& result, const Sample& sample) const
{
	std::queue<std::shared_ptr<const IFilter>> filters = _filters;
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

void FilterCascade::append(std::shared_ptr<const IFilter> filter)
{
	_filters.push(filter);
}