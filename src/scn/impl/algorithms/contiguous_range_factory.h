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

#include <scn/detail/ranges.h>
#include <scn/util/string_view.h>

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename View>
class take_width_view;

template <typename CharT>
struct string_view_wrapper {
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;
    using string_view_type = std::basic_string_view<CharT>;

    constexpr string_view_wrapper() = default;

    template <typename Range,
              std::enable_if_t<ranges::borrowed_range<Range> &&
                               ranges::contiguous_range<Range> &&
                               ranges::sized_range<Range>>* = nullptr>
    constexpr string_view_wrapper(Range&& r)
        : sv(ranges::data(r), ranges_polyfill::usize(r))
    {
    }

    template <typename Range,
              std::enable_if_t<ranges::borrowed_range<Range> &&
                               ranges::contiguous_range<Range> &&
                               ranges::sized_range<Range>>* = nullptr>
    void assign(Range&& r)
    {
        sv = string_view_type{ranges::data(r), ranges_polyfill::usize(r)};
    }

    constexpr auto view() const
    {
        return sv;
    }

    constexpr bool stores_allocated_string() const
    {
        return false;
    }

    [[noreturn]] string_type get_allocated_string() const
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    string_view_type sv;
};

template <typename Range>
string_view_wrapper(Range)
    -> string_view_wrapper<detail::char_t<detail::remove_cvref_t<Range>>>;

template <typename CharT>
class contiguous_range_factory {
public:
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;
    using string_view_type = std::basic_string_view<CharT>;

    contiguous_range_factory() = default;

    template <typename Range,
              std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
    contiguous_range_factory(Range&& range)
    {
        emplace_range(SCN_FWD(range));
    }

    contiguous_range_factory(string_view_wrapper<CharT> svw)
        : m_storage(std::nullopt), m_view(svw.view())
    {
    }

    contiguous_range_factory(const contiguous_range_factory&) = delete;
    contiguous_range_factory& operator=(const contiguous_range_factory&) =
        delete;

    contiguous_range_factory(contiguous_range_factory&& other)
        : m_storage(SCN_MOVE(other.m_storage))
    {
        if (m_storage) {
            m_view = *m_storage;
        }
        else {
            m_view = other.m_view;
        }
    }
    contiguous_range_factory& operator=(contiguous_range_factory&& other)
    {
        m_storage = SCN_MOVE(other.m_storage);
        if (m_storage) {
            m_view = *m_storage;
        }
        else {
            m_view = other.m_view;
        }
        return *this;
    }

    ~contiguous_range_factory() = default;

    template <typename Range,
              std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
    void assign(Range&& range)
    {
        emplace_range(SCN_FWD(range));
    }

    string_view_type view() const
    {
        return m_view;
    }

    constexpr bool stores_allocated_string() const
    {
        return m_storage.has_value();
    }

    string_type& get_allocated_string() &
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }
    const string_type& get_allocated_string() const&
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }
    string_type&& get_allocated_string() &&
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }

    string_type& make_into_allocated_string()
    {
        if (stores_allocated_string()) {
            return get_allocated_string();
        }

        auto& str = m_storage.emplace(m_view.data(), m_view.size());
        m_view = string_view_type{str.data(), str.size()};
        return str;
    }

private:
    template <typename Range>
    void emplace_range(Range&& range)
    {
        using value_t = ranges::range_value_t<Range>;
        if constexpr (ranges::borrowed_range<Range> &&
                      ranges::contiguous_range<Range> &&
                      ranges::sized_range<Range>) {
            m_storage.reset();
            m_view = string_view_type{ranges::data(range),
                                      ranges_polyfill::usize(range)};
        }
        else if constexpr (std::is_same_v<detail::remove_cvref_t<Range>,
                                          std::basic_string<CharT>>) {
            m_storage.emplace(SCN_FWD(range));
            m_view = string_view_type{*m_storage};
        }
        else if constexpr (std::is_same_v<ranges::iterator_t<Range>,
                                          typename detail::basic_scan_buffer<
                                              value_t>::forward_iterator> &&
                           ranges::common_range<Range>) {
            auto beg_seg = range.begin().contiguous_segment();
            auto end_seg = range.end().contiguous_segment();
            if (SCN_UNLIKELY(detail::to_address(beg_seg.end()) !=
                             detail::to_address(end_seg.end()))) {
                auto& str = m_storage.emplace();
                str.reserve(range.end().position() - range.begin().position());
                std::copy(range.begin(), range.end(), std::back_inserter(str));
                m_view = string_view_type{str};
                return;
            }

            m_view = detail::make_string_view_from_pointers(beg_seg.data(),
                                                            end_seg.data());
            m_storage.reset();
        }
        else {
            auto& str = m_storage.emplace();
            if constexpr (ranges::sized_range<Range>) {
                str.reserve(ranges_polyfill::usize(range));
            }
            std::copy(ranges::begin(range), ranges::end(range),
                      std::back_inserter(str));
            m_view = string_view_type{str};
        }
    }

    std::optional<string_type> m_storage{std::nullopt};
    string_view_type m_view{};
};

template <typename Range>
contiguous_range_factory(Range)
    -> contiguous_range_factory<detail::char_t<detail::remove_cvref_t<Range>>>;

template <typename Range>
auto make_contiguous_buffer(Range&& range)
{
    if constexpr (ranges::borrowed_range<Range> &&
                  ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return string_view_wrapper{SCN_FWD(range)};
    }
    else {
        return contiguous_range_factory{SCN_FWD(range)};
    }
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
