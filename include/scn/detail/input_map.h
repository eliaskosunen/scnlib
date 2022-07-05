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

            // erased_range& -> erased_view (subrange)
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

            // erased_view (subrange) -> self
            template <typename CharT>
            static std::enable_if_t<is_valid_char_type<CharT>,
                                    basic_erased_subrange<CharT>>
            impl(basic_erased_subrange<CharT> r, priority_tag<3>)
                SCN_NOEXCEPT_P(std::is_nothrow_move_constructible_v<
                               basic_erased_subrange<CharT>>)
            {
                return r;
            }

            // contiguous + sized + valid-char + borrowed -> string_view
            template <typename Range,
                      typename CharT = ranges::range_value_t<Range>>
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

            // forward + proper char type -> erased
            template <typename Range,
                      typename CharT = ranges::range_value_t<Range>>
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
        inline constexpr bool is_scannable_range =
            !std::is_same_v<decltype(scan_map_input_range_impl(
                                SCN_DECLVAL(const Range&))),
                            invalid_input_range>;
    }  // namespace detail

    template <
        typename Range,
        typename std::enable_if_t<detail::is_scannable_range<Range>>* = nullptr>
    auto scan_map_input_range(const Range& r) SCN_NOEXCEPT_P(
        noexcept(detail::scan_map_input_range_impl(SCN_DECLVAL(const Range&))))
    {
        return detail::scan_map_input_range_impl(r);
    }

    template <typename Range,
              typename std::enable_if_t<!detail::is_scannable_range<Range>>* =
                  nullptr>
    invalid_input_range scan_map_input_range(const Range&) SCN_NOEXCEPT
    {
        static_assert(detail::dependent_false<Range>::value,
                      "\n"
                      "Unsupported range type given as input to a scanning "
                      "function.\n"
                      "Generally, only string-like values, "
                      "scn::istream_char_ranges, or scn::erased_ranges can be "
                      "scanned from.\n"
                      "Convert your range type to a "
                      "std::string_view or std::string, or erase its type with "
                      "scn::erase_range().\n"
                      "See the scn documentation for more details.");

        return {};
    }

    SCN_END_NAMESPACE
}  // namespace scn
