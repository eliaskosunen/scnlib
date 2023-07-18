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

#include <bits/ranges_base.h>
#include <scn/impl/algorithms/eof_check.h>
#include <scn/util/meta.h>

#include <algorithm>
#include <cstring>
#include <string_view>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        /**
         * No-op output range
         */
        template <typename CharT>
        struct null_output_range {
            using value_type = CharT;

            constexpr null_output_range() = default;

            constexpr auto begin() const SCN_NOEXCEPT
            {
                return iterator{};
            }
            constexpr auto end() const SCN_NOEXCEPT
            {
                return ranges_std::unreachable_sentinel_t{};
            }

            struct iterator {
                using value_type = CharT;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type*;
                using reference = value_type&;
                using iterator_category = std::output_iterator_tag;

                constexpr iterator() = default;

                constexpr iterator& operator=(CharT) SCN_NOEXCEPT
                {
                    return *this;
                }

                constexpr iterator& operator*() SCN_NOEXCEPT
                {
                    return *this;
                }

                constexpr iterator& operator++() SCN_NOEXCEPT
                {
                    return *this;
                }
                constexpr iterator operator++(int) SCN_NOEXCEPT
                {
                    return *this;
                }
            };
        };

        static_assert(ranges::range<null_output_range<char>>);
        static_assert(ranges::output_range<null_output_range<char>, char>);

        template <typename InputR, typename OutputR>
        using copy_result =
            ranges::in_out_result<ranges::borrowed_iterator_t<InputR>,
                                  ranges::borrowed_iterator_t<OutputR>>;

        template <typename InputR, typename OutputR>
        SCN_NODISCARD copy_result<InputR, OutputR> copy(InputR&& input,
                                                        OutputR&& output)
        {
            if constexpr (ranges::contiguous_range<InputR> &&
                          ranges::contiguous_range<OutputR> &&
                          ranges::sized_range<InputR> &&
                          ranges::sized_range<OutputR>) {
                // Optimization for contiguous + sized ranges -> memmove
                const auto input_size =
                    static_cast<size_t>(ranges::size(input));
                const auto output_size =
                    static_cast<size_t>(ranges::size(output));
                const auto items_to_copy = static_cast<std::ptrdiff_t>(
                    (std::min)(input_size, output_size));
                std::memmove(ranges::data(output), ranges::data(input),
                             input_size * sizeof(detail::char_t<InputR>));
                return {ranges::begin(input) + items_to_copy,
                        ranges::begin(output) + items_to_copy};
            }
            else if constexpr (detail::is_specialization_of_v<
                                   OutputR, null_output_range>) {
                // Optimization for null_output_range
                return {ranges::next(ranges::begin(input), ranges::end(input)),
                        ranges::begin(output)};
            }
            else {
                auto src = ranges::begin(input);
                auto dst = ranges::begin(output);
                for (; src != ranges::end(input) && dst != ranges::end(output);
                     ++src, (void)++dst) {
                    *dst = *src;
                }
                return {src, dst};
            }
        }

        template <typename I, typename T>
        struct iterator_value_result {
            SCN_NO_UNIQUE_ADDRESS I iterator;
            SCN_NO_UNIQUE_ADDRESS T value;
        };

        template <typename Container>
        class back_insert_view
            : ranges::view_interface<back_insert_view<Container>> {
        public:
            friend class iterator;

            using container_type = Container;
            using value_type = typename Container::value_type;

            constexpr back_insert_view() = default;
            constexpr explicit back_insert_view(Container& c) SCN_NOEXCEPT
                : m_container(&c)
            {
            }

            auto begin() SCN_NOEXCEPT
            {
                return iterator{*this};
            }

            auto end() SCN_NOEXCEPT
            {
                return ranges_std::unreachable_sentinel_t{};
            }

            Container& container() SCN_NOEXCEPT
            {
                return *m_container;
            }
            const Container& container() const SCN_NOEXCEPT
            {
                return *m_container;
            }

            class iterator {
                struct proxy {
                    proxy& operator=(const value_type& val)
                    {
                        c->push_back(val);
                        return *this;
                    }
                    proxy& operator=(value_type&& val)
                    {
                        c->push_back(SCN_MOVE(val));
                        return *this;
                    }

                    Container* c;
                };

            public:
                using value_type = typename Container::value_type;
                using difference_type = std::ptrdiff_t;
                using pointer = void;
                using reference = void;
                using iterator_category = std::output_iterator_tag;

                constexpr iterator() SCN_NOEXCEPT = default;
                constexpr explicit iterator(back_insert_view& r) SCN_NOEXCEPT
                    : m_range{&r}
                {
                }

                proxy operator*() SCN_NOEXCEPT
                {
                    return {m_range->m_container};
                }

                iterator& operator++() SCN_NOEXCEPT
                {
                    return *this;
                }
                iterator operator++(int) SCN_NOEXCEPT
                {
                    return *this;
                }

            private:
                back_insert_view* m_range{nullptr};
            };

        private:
            Container* m_container;
        };

        template <typename Container>
        auto back_insert(Container& c) SCN_NOEXCEPT
        {
            return back_insert_view<Container>{c};
        }

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
                : sv(ranges::data(r), ranges::size(r))
            {
            }

            template <typename Range,
                      std::enable_if_t<ranges::borrowed_range<Range> &&
                                       ranges::contiguous_range<Range> &&
                                       ranges::sized_range<Range>>* = nullptr>
            void assign(Range&& r)
            {
                sv = string_view_type{ranges::data(r), ranges::size(r)};
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

            contiguous_range_factory(const string_view_wrapper<CharT>& svw)
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

                return {_get_string_view()->data(), _get_string_view()->size()};
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
                                           ranges::size(range));
                    m_stores_string = false;
                }
                else if constexpr (std::is_same_v<detail::remove_cvref_t<Range>,
                                                  std::basic_string<CharT>>) {
                    _construct_string(SCN_FWD(range));
                    m_stores_string = true;
                }
                else {
                    _construct_string(ranges::begin(range), ranges::end(range));
                    m_stores_string = true;
                }
            }

            string_type* _get_string()
            {
                return reinterpret_cast<string_type*>(m_memory);
            }
            const string_type* _get_string() const
            {
                return reinterpret_cast<const string_type*>(m_memory);
            }

            string_view_type* _get_string_view()
            {
                return reinterpret_cast<string_view_type*>(m_memory);
            }
            const string_view_type* _get_string_view() const
            {
                return reinterpret_cast<const string_view_type*>(m_memory);
            }

            void _copy(const contiguous_range_factory& other)
            {
                if (other.m_stores_string) {
                    _construct_string(other.view().data(), other.view().size());
                }
                else {
                    _construct_string_view(other.view().data(),
                                           other.view().size());
                }
            }

            void _move(contiguous_range_factory&& other)
            {
                if (other.m_stores_string) {
                    _construct_string(SCN_MOVE(other.get_allocated_string()));
                }
                else {
                    _construct_string_view(other.view().data(),
                                           other.view().size());
                }
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

#if SCN_STD_RANGES
namespace std::ranges {
    template <typename Container>
    inline constexpr bool enable_view<scn::impl::back_insert_view<Container>> =
        true;
    template <typename Container>
    inline constexpr bool
        enable_borrowed_range<scn::impl::back_insert_view<Container>> = true;

    template <typename CharT>
    inline constexpr bool
        enable_borrowed_range<scn::impl::null_output_range<CharT>> = true;
}  // namespace std::ranges

#else
namespace nano {
    template <typename Container>
    inline constexpr bool enable_view<scn::impl::back_insert_view<Container>> =
        true;
    template <typename Container>
    inline constexpr bool
        enable_borrowed_range<scn::impl::back_insert_view<Container>> = true;

    template <typename CharT>
    inline constexpr bool
        enable_borrowed_range<scn::impl::null_output_range<CharT>> = true;
}  // namespace nano
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    static_assert(ranges::range<impl::back_insert_view<std::string>>);
    static_assert(ranges::view<impl::back_insert_view<std::string>>);

    SCN_END_NAMESPACE
}  // namespace scn
