#pragma once

// from http://stackoverflow.com/a/21740143

namespace krt
{
namespace detail
{
//plain function pointers
template <typename... Args, typename ReturnType>
inline auto make_function(ReturnType (*p)(Args...)) -> std::function<ReturnType(Args...)>
{
	return {p};
}

//nonconst member function pointers
template <typename... Args, typename ReturnType, typename ClassType>
inline auto make_function(ReturnType (ClassType::*p)(Args...)) -> std::function<ReturnType(Args...)>
{
	return {p};
}

//const member function pointers
template <typename... Args, typename ReturnType, typename ClassType>
inline auto make_function(ReturnType (ClassType::*p)(Args...) const) -> std::function<ReturnType(Args...)>
{
	return {p};
}

//qualified functionoids
template <typename FirstArg, typename... Args, class T>
inline auto make_function(T&& t) -> std::function<decltype(t(std::declval<FirstArg>(), std::declval<Args>()...))(FirstArg, Args...)>
{
	return {std::forward<T>(t)};
}

//unqualified functionoids try to deduce the signature of `T::operator()` and use that.
template <class T>
inline auto make_function(T&& t) -> decltype(make_function(&std::remove_reference<T>::type::operator()))
{
	return {std::forward<T>(t)};
}
}
}