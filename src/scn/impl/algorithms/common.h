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

#include <scn/impl/algorithms/eof_check.h>
#include <scn/util/meta.h>

#include <algorithm>
#include <cstring>

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
                            input_size * sizeof(ranges::range_value_t<InputR>));
                return {ranges::begin(input) + items_to_copy,
                        ranges::begin(output) + items_to_copy};
            }
            else if constexpr (detail::is_specialization_of_v<
                                   OutputR, null_output_range> &&
                               ranges::common_range<InputR>) {
                // Optimization for null_output_range
                // common_range required to be able to return input.end()
                return {ranges::end(input), ranges::begin(output)};
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
