#pragma once

namespace krt
{
namespace detail
{
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
	return f(std::get<I>(std::forward<Tuple>(t))...);
}
}

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
	return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
	                          std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
}
}