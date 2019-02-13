#pragma once

#include <algorithm>
#include <memory>
#include <functional>

namespace foo
{
	/**
	 * Filters Container<InputType> based on Filter function
	 */
	template <typename Container, typename Filter, typename OutputContainer = Container>
	OutputContainer filtered(const Container& container, Filter filter)
	{
		OutputContainer filtered_elements;
		std::copy_if(container.begin(), container.end(), std::back_inserter(filtered_elements), filter);

		return filtered_elements;
	}

	/**
	 * Filters Container<InputType> based on Filter function and allows to explicitly specify the type of the OutputContainer
	 */
	template <typename OutputContainer, typename Container, typename Filter>
	auto filteredAs(const Container& container, Filter filter)
	{
		return filtered<Container, Filter, OutputContainer>(container, filter);
	}
}
