#include "include/scn/detail/scan.h"

#include <iostream>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

template <typename T, std::size_t N>
SCN_CONSTEXPR14 void impl(T(&&)[N], scn::detail::priority_tag<3>) = delete;

template <typename T, std::size_t N>
SCN_CONSTEXPR14 auto impl(T (&t)[N], scn::detail::priority_tag<3>) noexcept
    -> decltype((t) + 0)
{
    return (t) + 0;
}

template <typename C>
SCN_CONSTEXPR14 auto impl(scn::basic_string_view<C> sv,
                          scn::detail::priority_tag<2>) noexcept
    -> decltype(sv.begin())
{
    return sv.begin();
}

template <typename T>
SCN_CONSTEXPR14 auto impl(T& t, scn::detail::priority_tag<1>) noexcept(
    noexcept(scn::detail::decay_copy(t.begin())))
    -> decltype(scn::detail::decay_copy(t.begin()))
{
    return scn::detail::decay_copy(t.begin());
}

template <typename T>
SCN_CONSTEXPR14 auto impl(T&& t, scn::detail::priority_tag<0>) noexcept(
    noexcept(scn::detail::decay_copy(begin(std::forward<T>(t)))))
    -> decltype(scn::detail::decay_copy(begin(std::forward<T>(t))))
{
    return scn::detail::decay_copy(begin(std::forward<T>(t)));
}

template <typename T>
SCN_CONSTEXPR14 auto fn(T&& t) noexcept(
    noexcept(impl(std::forward<T>(t), scn::detail::priority_tag<3>{})))
    -> decltype(impl(std::forward<T>(t), scn::detail::priority_tag<3>{}))
{
    return impl(std::forward<T>(t), scn::detail::priority_tag<3>{});
}

int main()
{
    int i;
    std::string str;
    scn::scan("42 foo", "{} {}", i, str);
    std::cout << i << ' ' << str << '\n';

    // auto str = "Hello world!";
    // auto sr = rng::subrange<const char*>{str, str + std::strlen(str)};
    // sr.begin();
    // impl(sr, scn::detail::priority_tag<3>{});
    // fn(sr);
}
