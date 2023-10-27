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

#include <scn/detail/erased_range.h>
#include <scn/detail/istream_range.h>
#include <scn/util/meta.h>
#include <scn/util/span.h>
#include <scn/util/string_view.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    struct invalid_input_range {};

    namespace detail::_scan_map_input_range_impl {
        template <typename CharT>
        inline constexpr bool is_valid_char_type =
            std::is_same_v<std::remove_const_t<CharT>, char> ||
            std::is_same_v<std::remove_const_t<CharT>, wchar_t>;

        struct fn {
        private:
            // string_view -> string_view
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    std::basic_string_view<CharT>>
            impl(std::basic_string_view<CharT> r, priority_tag<4>) SCN_NOEXCEPT
            {
                return r;
            }

            // string& -> string_view
            template <typename CharT, typename Allocator>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    std::basic_string_view<CharT>>
            impl(const std::
                     basic_string<CharT, std::char_traits<CharT>, Allocator>& r,
                 priority_tag<4>) SCN_NOEXCEPT
            {
                return {r.data(), r.size()};
            }

            // CharT(&)[] -> string_view
            template <typename CharT, std::size_t N>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    std::basic_string_view<CharT>>
            impl(const CharT (&r)[N], priority_tag<4>) SCN_NOEXCEPT
            {
                return {r, N - 1};
            }

#if SCN_USE_IOSTREAMS
            // istreambuf_view& -> istreambuf_subrange
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    basic_istreambuf_subrange<CharT>>
            impl(const basic_istreambuf_view<CharT>& r, priority_tag<4>)
                SCN_NOEXCEPT_P(std::is_nothrow_constructible_v<
                               basic_istreambuf_subrange<CharT>,
                               basic_istreambuf_view<CharT>&>)
            {
                return {r};
            }
#endif

            // erased_range& -> erased_subrange
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    basic_erased_subrange<CharT>>
            impl(const basic_erased_range<CharT>& r, priority_tag<4>)
                SCN_NOEXCEPT_P(std::is_nothrow_constructible_v<
                               basic_erased_subrange<CharT>,
                               basic_erased_range<CharT>&>)
            {
                return {r};
            }

#if SCN_USE_IOSTREAMS
            // istreambuf_subrange -> self
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    basic_istreambuf_subrange<CharT>>
            impl(basic_istreambuf_subrange<CharT> r, priority_tag<3>)
                SCN_NOEXCEPT_P(std::is_nothrow_move_constructible_v<
                               basic_istreambuf_subrange<CharT>>)
            {
                return r;
            }
            template <typename Iterator,
                      typename Sentinel,
                      ranges::subrange_kind Kind,
                      typename CharT = ranges_std::iter_value_t<Iterator>>
            static std::enable_if_t<
                is_valid_char_type<CharT> &&
                    std::is_same_v<
                        Iterator,
                        typename basic_istreambuf_view<CharT>::iterator> &&
                    std::is_same_v<
                        Sentinel,
                        typename basic_istreambuf_view<CharT>::sentinel>,
                basic_istreambuf_subrange<CharT>>
            impl(ranges::subrange<Iterator, Sentinel, Kind> r, priority_tag<3>)
                SCN_NOEXCEPT_P(std::is_nothrow_constructible_v<
                               basic_istreambuf_subrange<CharT>,
                               Iterator,
                               Sentinel>)
            {
                return {r.begin(), r.end()};
            }
#endif

            // erased_subrange -> self
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    basic_erased_subrange<CharT>>
            impl(basic_erased_subrange<CharT> r, priority_tag<3>)
                SCN_NOEXCEPT_P(std::is_nothrow_move_constructible_v<
                               basic_erased_subrange<CharT>>)
            {
                return r;
            }
            template <typename Iterator,
                      typename Sentinel,
                      ranges::subrange_kind Kind,
                      typename CharT = ranges_std::iter_value_t<Iterator>>
            static std::enable_if_t<
                is_valid_char_type<CharT> &&
                    std::is_same_v<
                        Iterator,
                        typename basic_erased_range<CharT>::iterator> &&
                    std::is_same_v<
                        Sentinel,
                        typename basic_erased_range<CharT>::sentinel>,
                basic_erased_subrange<CharT>>
            impl(ranges::subrange<Iterator, Sentinel, Kind> r, priority_tag<3>)
                SCN_NOEXCEPT_P(std::is_nothrow_constructible_v<
                               basic_erased_subrange<CharT>,
                               Iterator,
                               Sentinel>)
            {
                return {r.begin(), r.end()};
            }

            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wnoexcept")

            // contiguous + sized + valid-char -> string_view
            template <typename Range, typename CharT = detail::char_t<Range>>
            static std::enable_if_t<is_valid_char_type<CharT> &&
                                        ranges::contiguous_range<Range> &&
                                        ranges::sized_range<Range>,
                                    std::basic_string_view<CharT>>
            impl(const Range& r, priority_tag<2>) SCN_NOEXCEPT_P(
                noexcept(ranges::data(r)) && noexcept(ranges::size(r)) &&
                std::is_nothrow_constructible_v<std::basic_string_view<CharT>,
                                                decltype(ranges::data(r)),
                                                decltype(ranges::size(r))>)
            {
                return {ranges::data(r),
                        static_cast<std::size_t>(ranges::size(r))};
            }
            // !contiguous + random-access + iterator can be made into a ptr
            // for MSVC debug iterators
            //   -> string_view
            template <typename Range, typename CharT = detail::char_t<Range>>
            static std::enable_if_t<is_valid_char_type<CharT> &&
                                        !ranges::contiguous_range<Range> &&
                                        ranges::random_access_range<Range> &&
                                        can_make_address_from_iterator<
                                            ranges::iterator_t<Range>>::value,
                                    std::basic_string_view<CharT>>
            impl(const Range& r, priority_tag<2>) SCN_NOEXCEPT_P(
                noexcept(ranges::begin(r)) && noexcept(ranges::end(r)) &&
                std::is_nothrow_constructible_v<std::basic_string_view<CharT>,
                                                const CharT*,
                                                std::size_t>)
            {
                return make_string_view_from_pointers<CharT>(
                    to_address(ranges::begin(r)), to_address(ranges::end(r)));
            }

            SCN_GCC_POP

            // forward + proper char type -> erased
            template <typename Range, typename CharT = detail::char_t<Range>>
            static std::enable_if_t<is_valid_char_type<CharT> &&
                                        ranges::forward_range<Range>,
                                    basic_erased_range<CharT>>
            impl(const Range& r, priority_tag<1>)
            {
                return basic_erased_range<CharT>{r};
            }

            // other -> error
            template <typename T>
            static invalid_input_range impl(const T&,
                                            priority_tag<0>) SCN_NOEXCEPT
            {
                if constexpr (std::is_same_v<T, stdin_range_marker>) {
                    static_assert(
                        dependent_false<T>::value,
                        "\n"
                        "stdin_range_marker cannot be used as an "
                        "source range type to scn::scan.\n"
                        "To read from stdin, use scn::input or scn::prompt, "
                        "and do not provide an explicit source range.");
                }

                return {};
            }

        public:
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wnoexcept")
            template <typename Range>
            auto operator()(const Range& r) const
                SCN_NOEXCEPT_P(noexcept(fn::impl(r, priority_tag<4>{})))
                    -> decltype(fn::impl(r, priority_tag<4>{}))
            {
                return fn::impl(r, priority_tag<4>{});
            }
            SCN_GCC_POP
        };
    }  // namespace detail::_scan_map_input_range_impl
    namespace detail {
        inline constexpr auto scan_map_input_range_impl =
            _scan_map_input_range_impl::fn{};

        template <typename Range>
        using mapped_source_range =
            decltype(scan_map_input_range_impl(SCN_DECLVAL(const Range&)));

        template <typename Range>
        inline constexpr bool is_scannable_range =
            !std::is_same_v<mapped_source_range<Range>, invalid_input_range>;

        template <typename CharT>
        std::basic_string_view<CharT> decay_source_range(
            std::basic_string_view<CharT>);

#if SCN_USE_IOSTREAMS
        template <typename CharT>
        basic_istreambuf_subrange<CharT> decay_source_range(
            basic_istreambuf_subrange<CharT>);
        template <typename CharT>
        basic_istreambuf_subrange<CharT> decay_source_range(
            const basic_istreambuf_view<CharT>&);
#endif

        template <typename CharT>
        basic_erased_subrange<CharT> decay_source_range(
            basic_erased_subrange<CharT>);
        template <typename CharT>
        basic_erased_subrange<CharT> decay_source_range(
            const basic_erased_range<CharT>&);

        template <typename R>
        using decayed_source_range =
            decltype(decay_source_range(SCN_DECLVAL(R)));

        template <typename R>
        using decayed_mapped_source_range =
            decayed_source_range<mapped_source_range<R>>;

        /**
         * Map a range type given to a generic scanning function (like `scan`)
         * into something that can be given to a type-erased scanning function
         * (like `vscan`).
         *
         * Maps
         *  - `string_view` and other contiguous+sized ranges to `string_view`
         *  - `istreambuf_view` to `istreambuf_subrange`
         *  - `erased_view` to `erased_subrange`
         *  - any other forward range to `erased_view`
         *  - errors (static_assert) on other, invalid range types
         */
        template <typename Range>
        auto scan_map_input_range(const Range& r) SCN_NOEXCEPT_P(noexcept(
            detail::scan_map_input_range_impl(SCN_DECLVAL(const Range&))))
        {
            static_assert(
                detail::is_scannable_range<Range>,
                "\n"
                "Unsupported range type given as input to a scanning "
                "function.\n"
                "A range needs to model forward_range and have a valid "
                "character type (char or wchar_t) to be scannable.\n"
                "Examples of scannable ranges are std::string, "
                "std::string_view, "
                "std::vector<char>, and scn::istreambuf_view.\n"
                "See the scn documentation for more details.");

            return detail::scan_map_input_range_impl(r);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
