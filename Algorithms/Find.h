#pragma once

#include "TypeTraits.h"

#include <algorithm>
#include <functional>
#include <optional>

namespace foo
{
	/**
	 * Combines multiple selectors into one.
	 *
	 * @returns a Callable the can be invoked on an object.
	 *
	 * @note This Callable will invoke Selectors on the itermediate object in the left-to-right order.
	 * Such chain can be used to extract multiple nested properties of an object, or in foo::find()
	 */
	template <typename Selector, typename... Selectors>
	auto chained(Selector selector, Selectors... selectors)
	{
		return [=](auto object)
		{
			if constexpr (sizeof...(Selectors) == 0)
			{
				return std::invoke(selector, object);
			}
			else
			{
				return chained(selectors...)(std::invoke(selector, object));
			}
		};
	}

	template <class Container, class ValueType>
	auto find(Container&& container, const ValueType& value)
		-> optional<traits::ValueTypeOf<Container>>
	{
		auto it = std::find(std::begin(container), std::end(container), value);
		if (it != std::end(container))
		{
			return foo::make_optional(*it);
		}
		else
		{
			return nullopt;
		}
	}

	/**
	 * Heterogenous find - Allows searching the \a container based on its property (obtainable via \a selector)
	 * 
	 * @example:
	 *  find(container, &Element::name, "some string");
	 *
	 * @returns optional<value_type of the Container>
	 */
	template <class Container, typename Selector, class ValueType>
	auto find(Container&& container, Selector selector, const ValueType& value)
		-> optional<traits::ValueTypeOf<Container>>
	{
		auto it = std::find_if(std::begin(container), std::end(container), [&](const auto& element)
		{
			return std::invoke(selector, element) == value;
		});

		if (it != std::end(container))
		{
			return foo::make_optional(*it);
		}
		else
		{
			return nullopt;
		}
	}

	template <class Container, typename Selector, class ValueType>
	auto find_ref(Container&& container, Selector selector, const ValueType& value)
		-> optional<std::reference_wrapper<traits::ValueTypeOf<Container>>>
	{
		auto it = std::find_if(std::begin(container), std::end(container), [&](const auto& element)
		{
			return std::invoke(selector, element) == value;
		});

		if (it != std::end(container))
		{
			return foo::make_optional(std::ref(*it));
		}
		else
		{
			return nullopt;
		}
	}


	template <class Container, class UnaryPredicate>
	auto find_if(Container&& container, UnaryPredicate&& predicate)
		-> optional<traits::ValueTypeOf<Container>>
	{
		auto it = std::find_if(std::begin(container), std::end(container), FWD(predicate));
		if (it != std::end(container))
		{
			return foo::make_optional(*it);
		}
		else
		{
			return nullopt;
		}
	}

}
