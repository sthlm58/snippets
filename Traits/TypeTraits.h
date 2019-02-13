#pragma once

#include <iterator>
#include <type_traits>

namespace foo::traits
{
	template <template <typename...> class Template, class TestedType>
	struct IsSpecializationOf
	{
		template <typename... AA>
		static constexpr std::true_type check(Template<AA...>*);

		static constexpr std::false_type check(...);

		static constexpr bool value = decltype(check(static_cast<TestedType*>(nullptr))){};
	};

	template <template <typename...> class Template, class TestedType>
	constexpr bool IsSpecializationOf_v = IsSpecializationOf<Template, TestedType>::value;


	template <typename T>
	struct FirstTemplateParameter { using Type = T; };

	template <template <typename, typename...> class X, typename T, typename ...Args>
	struct FirstTemplateParameter<X<T, Args...>> { using Type = T; };

	template <typename T>
	using FirstTemplateParameter_t = typename FirstTemplateParameter<T>::Type;


	template <class F, class...Args>
	struct IsCallable
	{
		template <typename FF, typename... AA>
		static constexpr auto check(int) -> decltype( std::declval<FF>()(std::declval<AA>()...), std::true_type());

		template <typename FF, typename... AA>
		static constexpr std::false_type check(...);

		static constexpr bool value = decltype(check<F, Args...>(0)){};
	};

	template <class F, class...Args>
	constexpr bool IsCallable_v = IsCallable<F, Args...>::value;


	template <class, class = std::void_t<>>
	struct HasSubscriptOperator : std::false_type {};

	template <class T>
	struct HasSubscriptOperator<
		T,
		std::void_t<decltype (std::declval<T&>()[0])>
	> : std::true_type {};

	template <class T>
	constexpr bool HasSubscriptOperator_v = HasSubscriptOperator<T>::value;


	// moved from eCATS
	template <typename T>
	struct Arity : Arity<decltype(&T::operator())> {};

	template <typename ReturnType, typename... Args>
	struct Arity<ReturnType(*)(Args...)>
	{
		static constexpr size_t value = sizeof...(Args);
	};

	template <typename ClassType, typename ReturnType, typename... Args>
	struct Arity<ReturnType(ClassType::*)(Args...) const>
	{
		static constexpr size_t value = sizeof...(Args);
	};

	template <typename T>
	constexpr size_t Arity_v = Arity<T>::value;

	template <size_t Arity>
	struct ArityDispatch;

	template <>
	struct ArityDispatch<1>
	{
		template <typename Function, typename Arg1, typename Arg2>
		static auto call(Function&& f, Arg1&& /*arg1*/, Arg2&& arg2)
		{
			return f(arg2);
		}
	};

	template <>
	struct ArityDispatch<2>
	{
		template <typename Function, typename Arg1, typename Arg2>
		static auto call(Function&& f, Arg1&& arg1, Arg2&& arg2)
		{
			return f(arg1, arg2);
		}
	};

	template <typename Function>
	using ArityDispatched = ArityDispatch<Arity_v<Function>>;

	template <typename Container>
	using ValueTypeOf = std::remove_const_t<std::remove_reference_t<decltype(*std::begin(std::declval<Container>()))>>;

	template <typename>
	struct IsTemplate : std::false_type {};

	template <template <typename...> class Type, typename ...Args>
	struct IsTemplate<Type<Args...>> : std::true_type {};

	template <typename T>
	constexpr bool IsTemplate_v = IsTemplate<T>::value;

	template <typename T>
	struct IsReferenceWrapper : std::false_type {};

	template <typename T>
	struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type {};

	template <typename T>
	constexpr bool IsReferenceWrapper_v = IsReferenceWrapper<T>::value;

	/* Doesn't work with MSVC
	template <class T>
	constexpr bool isReferenceWrapper()
	{
		if constexpr (IsSpecializationOf_v<std::reference_wrapper, T>)
		{
			return true;
		}

		return false;
	}
	*/
}
