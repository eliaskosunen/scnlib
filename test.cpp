#include "include/scn/detail/scan.h"

#include <iostream>
#include <vector>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

#if 0
            struct _contiguous_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    _requires<_contiguous_range_concept, T>::value,
                    std::true_type>::type;

                template <typename T>
                auto _test_requires(T& t)
                    -> decltype(requires_expr<std::is_same<
                                    decltype(::scn::detail::ranges::data(t)),
                                    typename std::add_pointer<
                                        range_reference_t<T>>::type>::value>{});
            };
            template <typename T>
            struct contiguous_range
                : decltype(_contiguous_range_concept::test<T>(0)) {
            };
#endif

template <typename CharT, typename Traits, typename Allocator>
constexpr auto impl(std::basic_string<CharT, Traits, Allocator>& str,
                    scn::detail::priority_tag<2>) ->
    typename std::basic_string<CharT, Traits, Allocator>::pointer
{
    return std::addressof(*str.begin());
}
template <typename CharT, typename Traits, typename Allocator>
constexpr auto impl(const std::basic_string<CharT, Traits, Allocator>& str,
                    scn::detail::priority_tag<2>) ->
    typename std::basic_string<CharT, Traits, Allocator>::const_pointer
{
    return std::addressof(*str.begin());
}
template <typename CharT, typename Traits, typename Allocator>
constexpr auto impl(std::basic_string<CharT, Traits, Allocator>&& str,
                    scn::detail::priority_tag<2>) ->
    typename std::basic_string<CharT, Traits, Allocator>::pointer
{
    return std::addressof(*str.begin());
}

template <typename T,
          typename D = decltype(decay_copy(std::declval<T&>().data()))>
constexpr auto impl(T& t, scn::detail::priority_tag<1>) noexcept(
    noexcept(decay_copy(t.data()))) ->
    typename std::enable_if<rng::_is_object_pointer<D>::value, D>::type
{
    return decay_copy(t.data());
}

template <typename T>
constexpr auto impl(T&& t, scn::detail::priority_tag<0>) noexcept(
    noexcept(::scn::detail::ranges::begin(std::forward<T>(t)))) ->
    typename std::enable_if<
        rng::_is_object_pointer<
            decltype(::scn::detail::ranges::begin(std::forward<T>(t)))>::value,
        decltype(::scn::detail::ranges::begin(std::forward<T>(t)))>::type
{
    return ::scn::detail::ranges::begin(std::forward<T>(t));
}

int main()
{
    static_assert(rng::contiguous_range<scn::string_view>::value, "");
    static_assert(rng::contiguous_range<std::string>::value, "");

    std::string str = "Hello";
    auto dt = impl(str, scn::detail::priority_tag<2>{});

    debug<decltype(rng::data(std::declval<std::vector<char>&>()))> dvec{};
    debug<decltype(rng::data(std::declval<std::string&>()))> d{};
    debug<rng::range_reference_t<std::string>> d2{};
    debug<typename std::add_pointer<rng::range_reference_t<std::string>>::type>
        d3{};
}
