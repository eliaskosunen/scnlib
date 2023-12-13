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
                sv = string_view_type{ranges::data(r),
                                      ranges_polyfill::usize(r)};
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
        string_view_wrapper(Range) -> string_view_wrapper<
            detail::char_t<detail::remove_cvref_t<Range>>>;

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
                : m_buffer(std::in_place_type<string_view_type>, svw.view())
            {
            }

            template <typename Range,
                      std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
            void assign(Range&& range)
            {
                emplace_range(SCN_FWD(range));
            }

            string_view_type view() const
            {
                if (!stores_allocated_string()) {
                    return std::get<string_view_type>(m_buffer);
                }

                const auto& str = get_allocated_string();
                return {str.data(), str.size()};
            }

            constexpr bool stores_allocated_string() const
            {
                return std::holds_alternative<string_type>(m_buffer);
            }

            string_type& get_allocated_string() &
            {
                SCN_EXPECT(stores_allocated_string());
                return std::get<string_type>(m_buffer);
            }
            const string_type& get_allocated_string() const&
            {
                SCN_EXPECT(stores_allocated_string());
                return std::get<string_type>(m_buffer);
            }
            string_type get_allocated_string() &&
            {
                SCN_EXPECT(stores_allocated_string());
                return std::get<string_type>(SCN_MOVE(m_buffer));
            }

            string_type& make_into_allocated_string()
            {
                if (stores_allocated_string()) {
                    return get_allocated_string();
                }

                auto sv = std::get<string_view_type>(m_buffer);
                return m_buffer.template emplace<string_type>(sv.data(),
                                                              sv.size());
            }

        private:
            template <typename Range>
            void emplace_range(Range&& range)
            {
                using value_t = ranges::range_value_t<Range>;
                if constexpr (ranges::borrowed_range<Range> &&
                              ranges::contiguous_range<Range> &&
                              ranges::sized_range<Range>) {
                    m_buffer.template emplace<string_view_type>(
                        ranges::data(range), ranges_polyfill::usize(range));
                }
                else if constexpr (std::is_same_v<detail::remove_cvref_t<Range>,
                                                  std::basic_string<CharT>>) {
                    m_buffer.template emplace<string_type>(SCN_FWD(range));
                }
                else if constexpr (std::is_same_v<
                                       ranges::iterator_t<Range>,
                                       typename detail::basic_scan_buffer<
                                           value_t>::forward_iterator> &&
                                   ranges::common_range<Range>) {
                    SCN_EXPECT(ranges::begin(range).parent() &&
                               ranges::end(range).parent());
                    SCN_EXPECT(ranges::begin(range)
                                   .parent()
                                   ->get_contiguous_segment()
                                   .second == ranges::end(range)
                                                  .parent()
                                                  ->get_contiguous_segment()
                                                  .second);
                    auto sv = detail::make_string_view_from_iterators<value_t>(
                        ranges::begin(range).to_contiguous_segment_iterator(),
                        ranges::end(range).to_contiguous_segment_iterator());
                    m_buffer.template emplace<string_view_type>(sv);
                }
                else {
                    auto& str = m_buffer.template emplace<string_type>();
                    if constexpr (ranges::sized_range<Range>) {
                        str.reserve(ranges_polyfill::usize(range));
                    }
                    std::copy(ranges::begin(range), ranges::end(range),
                              std::back_inserter(str));
                }
            }

            std::variant<string_view_type, string_type> m_buffer{};
        };

        template <typename Range>
        contiguous_range_factory(Range) -> contiguous_range_factory<
            detail::char_t<detail::remove_cvref_t<Range>>>;

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
