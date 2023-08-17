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

#include <memory>
#include <type_traits>

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

            contiguous_range_factory()
            {
                _construct_string_view();
                m_stores_string = false;
            }

            template <typename Range,
                      std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
            contiguous_range_factory(Range&& range)
            {
                _construct_from_range(SCN_FWD(range));
            }

            contiguous_range_factory(string_view_wrapper<CharT> svw)
            {
                _construct_string_view(svw.view());
                m_stores_string = false;
            }

            contiguous_range_factory(const contiguous_range_factory& other)
            {
                _copy(other);
            }
            contiguous_range_factory& operator=(
                const contiguous_range_factory& other)
            {
                _destruct();
                _copy(other);
                return *this;
            }

            contiguous_range_factory(contiguous_range_factory&& other)
            {
                _move(SCN_MOVE(other));
            }
            contiguous_range_factory& operator=(
                contiguous_range_factory&& other)
            {
                _destruct();
                _move(SCN_MOVE(other));
                return *this;
            }

            ~contiguous_range_factory()
            {
                _destruct();
            }

            template <typename Range,
                      std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
            void assign(Range&& range)
            {
                _destruct();
                _construct_from_range(SCN_FWD(range));
            }

            string_view_type view() const
            {
                if (m_stores_string) {
                    return {_get_string()->data(), _get_string()->size()};
                }

                return *_get_string_view();
            }

            constexpr bool stores_allocated_string() const
            {
                return m_stores_string;
            }

            string_type& get_allocated_string() &
            {
                SCN_EXPECT(m_stores_string);
                return *_get_string();
            }
            const string_type& get_allocated_string() const&
            {
                SCN_EXPECT(m_stores_string);
                return *_get_string();
            }
            string_type get_allocated_string() &&
            {
                SCN_EXPECT(m_stores_string);
                return SCN_MOVE(*_get_string());
            }

            string_type& make_into_allocated_string()
            {
                if (stores_allocated_string()) {
                    return get_allocated_string();
                }

                auto sv = view();
                _destruct();
                _construct_string(sv.data(), sv.size());
                m_stores_string = true;

                return get_allocated_string();
            }

        private:
            template <typename... Args>
            void _construct_string(Args&&... args)
            {
                ::new (static_cast<void*>(m_memory))
                    string_type(SCN_FWD(args)...);
            }
            template <typename... Args>
            void _construct_string_view(Args&&... args)
            {
                ::new (static_cast<void*>(m_memory))
                    string_view_type(SCN_FWD(args)...);
            }

            template <typename Range>
            void _construct_from_range(Range&& range)
            {
                if constexpr (ranges::borrowed_range<Range> &&
                              ranges::contiguous_range<Range> &&
                              ranges::sized_range<Range>) {
                    _construct_string_view(ranges::data(range),
                                           ranges_polyfill::usize(range));
                    m_stores_string = false;
                }
                else if constexpr (std::is_same_v<detail::remove_cvref_t<Range>,
                                                  std::basic_string<CharT>>) {
                    _construct_string(SCN_FWD(range));
                    m_stores_string = true;
                }
                else {
                    _construct_string();
                    if constexpr (ranges::sized_range<Range>) {
                        _get_string()->reserve(ranges_polyfill::usize(range));
                    }
                    std::copy(ranges::begin(range), ranges::end(range),
                              std::back_inserter(*_get_string()));
                    m_stores_string = true;
                }
            }

            string_type* _get_string()
            {
                return reinterpret_cast<string_type*>(
                    SCN_ASSUME_ALIGNED(m_memory, alignof(string_type)));
            }
            const string_type* _get_string() const
            {
                return reinterpret_cast<const string_type*>(
                    SCN_ASSUME_ALIGNED(m_memory, alignof(string_type)));
            }

            string_view_type* _get_string_view()
            {
                return reinterpret_cast<string_view_type*>(
                    SCN_ASSUME_ALIGNED(m_memory, alignof(string_view_type)));
            }
            const string_view_type* _get_string_view() const
            {
                return reinterpret_cast<const string_view_type*>(
                    SCN_ASSUME_ALIGNED(m_memory, alignof(string_view_type)));
            }

            void _copy(const contiguous_range_factory& other)
            {
                if (other.m_stores_string) {
                    _construct_string(other.view().data(), other.view().size());
                }
                else {
                    _construct_string_view(other.view());
                }
                m_stores_string = other.m_stores_string;
            }

            void _move(contiguous_range_factory&& other)
            {
                if (other.m_stores_string) {
                    _construct_string(SCN_MOVE(other.get_allocated_string()));
                }
                else {
                    _construct_string_view(other.view());
                }
                m_stores_string = other.m_stores_string;
            }

            void _destruct()
            {
                if (m_stores_string) {
                    _get_string()->~string_type();
                }
                else {
                    _get_string_view()->~string_view_type();
                }
            }

            static constexpr auto alignment =
                std::max(alignof(string_type), alignof(string_view_type));
            static constexpr auto size =
                std::max(sizeof(string_type), sizeof(string_view_type));
            alignas(alignment) unsigned char m_memory[size]{};
            bool m_stores_string{false};
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
