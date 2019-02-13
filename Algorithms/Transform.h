#pragma once

#include <algorithm>
#include <memory>
#include <functional>

namespace foo
{

	namespace detail
	{
		/**
		 * Transform Container into OutputContainer based on Transform function
		 */
		template <typename Container,
				  typename Transform,
				  typename OutputContainer
				  >
		auto transformed_for_containers(const Container& input, Transform&& transformer)
		{
			using SizeType = decltype(std::declval<OutputContainer>().size());

			OutputContainer output;
			output.reserve(static_cast<SizeType>(input.size()));
			std::transform(input.begin(), input.end(), std::back_inserter(output), std::forward<Transform>(transformer));
			return output;
		}

		/**
		 * Transform Container<ContainerTypes...> into OutputContainer based on Transform function
		 */
		template <template <typename...> class Container,
				  typename... ContainerTypes,
				  typename Transform,
				  class OutputContainer = Container<decltype(std::declval<Transform>()(std::declval<traits::ValueTypeOf<Container<ContainerTypes...>>>()))>
				  >
		auto transformed_for_container_templates(const Container<ContainerTypes...>& input, Transform&& transformer, OutputContainer* = nullptr)
		{
			return transformed_for_containers<Container<ContainerTypes...>, Transform, OutputContainer>(input, std::forward<Transform>(transformer));
		}
	}

	/**
	 * Transform Container into "similar" Container based on Transform function
	 */
	template <typename Container, typename Transform>
	auto transformed(const Container& input, Transform&& transformer)
	{
		if constexpr (traits::IsTemplate_v<Container>)
		{
			return detail::transformed_for_container_templates(input, std::forward<Transform>(transformer));
		}
		else
		{
			return detail::transformed_for_containers<Container, Transform, Container>(input, std::forward<Transform>(transformer));
		}
	}

	/**
	 * Transform Container into OutputContainer based on Transform function
	 */
	template <typename OutputContainer, typename Container, typename Transform>
	auto transformedAs(const Container& input, Transform&& transformer)
	{
		if constexpr (traits::IsTemplate_v<Container>)
		{
			return detail::transformed_for_container_templates(input, std::forward<Transform>(transformer), static_cast<OutputContainer*>(nullptr));
		}
		else
		{
			return detail::transformed_for_containers<Container, Transform, OutputContainer>(input, std::forward<Transform>(transformer));
		}
	}

}
