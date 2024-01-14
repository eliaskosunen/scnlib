// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#pragma once

#include <scn/detail/args.h>
#include <scn/detail/error.h>

#include <tuple>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename... Args>
struct scan_result_value_storage {
public:
    using tuple_type = std::tuple<Args...>;

    constexpr scan_result_value_storage() = default;

    constexpr scan_result_value_storage(tuple_type&& values)
        : m_values(SCN_MOVE(values))
    {
    }

    /// Access the scanned values
    tuple_type& values() &
    {
        return m_values;
    }
    /// Access the scanned values
    const tuple_type& values() const&
    {
        return m_values;
    }
    /// Access the scanned values
    tuple_type&& values() &&
    {
        return SCN_MOVE(m_values);
    }
    /// Access the scanned values
    const tuple_type&& values() const&&
    {
        return SCN_MOVE(m_values);
    }

    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() &
    {
        return std::get<0>(m_values);
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() const&
    {
        return std::get<0>(m_values);
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() &&
    {
        return SCN_MOVE(std::get<0>(m_values));
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() const&&
    {
        return SCN_MOVE(std::get<0>(m_values));
    }

private:
    SCN_NO_UNIQUE_ADDRESS tuple_type m_values{};
};

struct scan_result_convert_tag {};

template <typename Range>
struct scan_result_range_storage {
public:
    using range_type = Range;
    using iterator = ranges::iterator_t<Range>;
    using sentinel = ranges::sentinel_t<Range>;

    constexpr scan_result_range_storage() = default;

    constexpr scan_result_range_storage(range_type&& r) : m_range(SCN_MOVE(r))
    {
    }

    template <typename R>
    explicit constexpr scan_result_range_storage(scan_result_convert_tag, R&& r)
        : m_range(SCN_MOVE(r))
    {
    }

    /// Access the ununsed source range
    range_type range() const
    {
        return m_range;
    }

    /// The beginning of the unused source range
    auto begin() const
    {
        return ranges::begin(m_range);
    }
    /// The end of the unused source range
    auto end() const
    {
        return ranges::end(m_range);
    }

protected:
    template <typename Other>
    void assign_range(Other&& r)
    {
        m_range = r.m_range;
    }

private:
    SCN_NO_UNIQUE_ADDRESS range_type m_range{};
};

struct scan_result_file_storage {
public:
    using range_type = std::FILE*;

    constexpr scan_result_file_storage() = default;

    constexpr scan_result_file_storage(std::FILE* f) : m_file(f) {}

    /// File used for scanning
    std::FILE* file() const
    {
        return m_file;
    }

protected:
    void assign_range(const scan_result_file_storage& f)
    {
        m_file = f.m_file;
    }

private:
    std::FILE* m_file{nullptr};
};

struct scan_result_dangling {
    using range_type = ranges::dangling;

    constexpr scan_result_dangling() = default;

    template <typename... Args>
    explicit constexpr scan_result_dangling(Args&&...)
    {
    }

    range_type range() const
    {
        return {};
    }

    ranges::dangling begin() const
    {
        return {};
    }
    ranges::dangling end() const
    {
        return {};
    }

protected:
    template <typename... Args>
    void assign_range(Args&&...)
    {
    }
};

template <typename Range>
constexpr auto get_scan_result_base()
{
    if constexpr (std::is_same_v<remove_cvref_t<Range>, ranges::dangling>) {
        return type_identity<scan_result_dangling>{};
    }
    else if constexpr (std::is_same_v<remove_cvref_t<Range>, std::FILE*>) {
        return type_identity<scan_result_file_storage>{};
    }
    else {
        return type_identity<scan_result_range_storage<Range>>{};
    }
}

#if !SCN_DOXYGEN
template <typename Range>
using scan_result_base = typename decltype(get_scan_result_base<Range>())::type;
#else
template <typename Range>
using scan_result_base = scan_result_range_storage<Range>;
#endif
}  // namespace detail

/**
 * \defgroup result Result types
 *
 * \brief Result and error types
 *
 * Instead of using exceptions, `scn::scan` and others return an object of
 * type `scn::scan_result`, wrapped inside a `scn::scan_expected`.
 */

/**
 * Type returned by `scan`, contains the unused input as a subrange, and the
 * scanned values in a tuple.
 */
template <typename Range, typename... Args>
class scan_result : public detail::scan_result_base<Range>,
                    public detail::scan_result_value_storage<Args...> {
    using range_base = detail::scan_result_base<Range>;
    using value_base = detail::scan_result_value_storage<Args...>;

public:
    using range_type = typename range_base::range_type;
    using tuple_type = typename value_base::tuple_type;

    constexpr scan_result() = default;

    constexpr scan_result(const scan_result&) = default;
    constexpr scan_result(scan_result&&) = default;
    constexpr scan_result& operator=(const scan_result&) = default;
    constexpr scan_result& operator=(scan_result&&) = default;

    ~scan_result() = default;

    scan_result(range_type r, tuple_type&& values)
        : range_base(SCN_MOVE(r)), value_base(SCN_MOVE(values))
    {
    }

    template <typename OtherR,
              std::enable_if_t<std::is_constructible_v<range_type, OtherR>>* =
                  nullptr>
    scan_result(OtherR&& r, tuple_type&& values)
        : range_base(detail::scan_result_convert_tag{}, SCN_FWD(r)),
          value_base(SCN_MOVE(values))
    {
    }

    template <typename OtherR,
              std::enable_if_t<
                  std::is_constructible_v<range_type, OtherR> &&
                  std::is_convertible_v<const OtherR&, range_type>>* = nullptr>
    /*implicit*/ scan_result(const scan_result<OtherR, Args...>& o)
        : range_base(detail::scan_result_convert_tag{}, o.range()),
          value_base(o.values())
    {
    }
    template <typename OtherR,
              std::enable_if_t<
                  std::is_constructible_v<range_type, OtherR> &&
                  !std::is_convertible_v<const OtherR&, range_type>>* = nullptr>
    explicit scan_result(const scan_result<OtherR, Args...>& o)
        : range_base(detail::scan_result_convert_tag{}, o.range()),
          value_base(o.values())
    {
    }

    template <typename OtherR,
              std::enable_if_t<std::is_constructible_v<range_type, OtherR> &&
                               std::is_convertible_v<OtherR&&, range_type>>* =
                  nullptr>
    /*implicit*/ scan_result(scan_result<OtherR, Args...>&& o)
        : range_base(detail::scan_result_convert_tag{}, SCN_MOVE(o.range())),
          value_base(SCN_MOVE(o.values()))
    {
    }
    template <typename OtherR,
              std::enable_if_t<std::is_constructible_v<range_type, OtherR> &&
                               !std::is_convertible_v<OtherR&&, range_type>>* =
                  nullptr>
    explicit scan_result(scan_result<OtherR, Args...>&& o)
        : range_base(detail::scan_result_convert_tag{}, SCN_MOVE(o.range())),
          value_base(SCN_MOVE(o.values()))
    {
    }

    template <typename OtherR,
              typename =
                  std::enable_if_t<std::is_constructible_v<range_type, OtherR>>>
    scan_result& operator=(const scan_result<OtherR, Args...>& o)
    {
        this->assign_range(o);
        this->values() = o.values();
        return *this;
    }

    template <typename OtherR,
              typename =
                  std::enable_if_t<std::is_constructible_v<range_type, OtherR>>>
    scan_result& operator=(scan_result<OtherR, Args...>&& o)
    {
        this->assign_range(o);
        this->values() = SCN_MOVE(o.values());
        return *this;
    }
};

template <typename R, typename... Args>
scan_result(R, std::tuple<Args...>) -> scan_result<R, Args...>;

namespace detail {
template <typename SourceRange>
auto make_vscan_result_range_end(SourceRange& source)
{
    return ranges::end(source);
}
template <typename CharT, size_t N>
auto make_vscan_result_range_end(CharT (&source)[N])
    -> ranges::sentinel_t<CharT (&)[N]>
{
    return source + N - 1;
}

template <typename SourceRange>
auto make_vscan_result_range(SourceRange&& source, std::ptrdiff_t n)
    -> borrowed_subrange_with_sentinel_t<SourceRange>
{
    return {ranges::next(ranges::begin(source), n),
            make_vscan_result_range_end(source)};
}
inline auto make_vscan_result_range(std::FILE* source, std::ptrdiff_t)
{
    return source;
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
