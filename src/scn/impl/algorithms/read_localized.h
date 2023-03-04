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

#include <scn/impl/algorithms/common.h>
#include <scn/impl/algorithms/read_nocopy.h>
#include <scn/impl/locale.h>
#include <scn/impl/unicode/unicode.h>
#include <scn/util/expected.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        struct localized_single_character_widener;

        template <>
        struct localized_single_character_widener<char> {
            using codecvt_facet_type =
                std::codecvt<wchar_t, char, std::mbstate_t>;

            template <typename InputR>
            using return_type = scan_expected<
                iterator_value_result<ranges::borrowed_iterator_t<InputR>,
                                      wchar_t>>;

        public:
            explicit localized_single_character_widener(detail::locale_ref loc)
                : m_cvt_facet(get_facet<codecvt_facet_type>(loc)),
                  m_is_classic(loc.get<std::locale>().name() == "C")
            {
            }

            template <typename InputR>
            return_type<InputR> operator()(InputR&& input);

        private:
            struct impl_base;
            struct codecvt_impl;
            struct unicode_impl;

            template <typename InputR, typename Impl>
            return_type<InputR> run(InputR&& input, Impl& impl);

            const codecvt_facet_type& m_cvt_facet;
            bool m_is_classic;
        };

        struct localized_single_character_widener<char>::impl_base {
            enum class result_code { ok, in_progress, error };

            result_code result{result_code::in_progress};
            wchar_t output_char{};

        protected:
            char buf[5]{};
            char* buf_it{buf};
        };

        struct localized_single_character_widener<char>::codecvt_impl
            : impl_base {
            codecvt_impl(const codecvt_facet_type& f) : facet(f) {}

            template <typename InputIt>
            void operator()(InputIt it)
            {
                *buf_it++ = *it;

                const char* input_next{};
                wchar_t* output_next{};

                std::mbstate_t mb{};
                result = result_map(facet.in(mb, buf, buf_it, input_next,
                                             &output_char, (&output_char) + 1,
                                             output_next));
            }

        private:
            result_code result_map(std::codecvt_base::result code)
            {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wswitch-default")
                switch (code) {
                    case std::codecvt_base::ok: {
                        if (output_char == 0) {
                            return result_code::in_progress;
                        }
                        return result_code::ok;
                    }
                    case std::codecvt_base::partial:
                    case std::codecvt_base::error:
                        return result_code::error;
                        return result_code::error;
                    case std::codecvt_base::noconv:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
                SCN_GCC_POP
                SCN_UNREACHABLE;
            }

            const codecvt_facet_type& facet;
        };

        struct localized_single_character_widener<char>::unicode_impl
            : impl_base {
            template <typename InputIt>
            void operator()(InputIt it)
            {
                *buf_it++ = *it;

                if (cp_len == static_cast<std::size_t>(-1)) {
                    auto cp_len_tmp =
                        code_point_length_by_starting_code_unit(buf[0]);
                    if (!cp_len_tmp) {
                        result = impl_base::result_code::error;
                        return;
                    }
                    cp_len = *cp_len_tmp;
                }

                const auto buf_len = static_cast<std::size_t>(buf_it - buf);
                if (cp_len != buf_len) {
                    result = impl_base::result_code::in_progress;
                    return;
                }

                const auto buf_sv = std::string_view{buf, buf_len};
                if (!validate_unicode(buf_sv)) {
                    result = impl_base::result_code::error;
                    return;
                }
                std::size_t ret =
                    transcode_valid(buf_sv, span<wchar_t>{&output_char, 1});
                if (ret == 0) {
                    result = impl_base::result_code::error;
                    return;
                }
                result = impl_base::result_code::ok;
            }

            std::size_t cp_len{static_cast<size_t>(-1)};
        };

        template <typename InputR>
        auto localized_single_character_widener<char>::operator()(
            InputR&& input) -> return_type<InputR>
        {
            static_assert(std::is_same_v<ranges::range_value_t<InputR>, char>);
            SCN_EXPECT(ranges::begin(input) != ranges::end(input));

            if (m_is_classic) {
                unicode_impl impl{};
                return run(SCN_FWD(input), impl);
            }
            codecvt_impl impl{m_cvt_facet};
            return run(SCN_FWD(input), impl);
        }

        template <typename InputR, typename Impl>
        auto localized_single_character_widener<char>::run(InputR&& input,
                                                           Impl& impl)
            -> return_type<InputR>
        {
            auto input_it = ranges::begin(input);

            while (impl.result == impl_base::result_code::in_progress &&
                   input_it != ranges::end(input)) {
                impl(input_it);
                ++input_it;
            }
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wswitch-default")
            switch (impl.result) {
                case impl_base::result_code::ok:
                    return {{input_it, impl.output_char}};
                case impl_base::result_code::in_progress:
                case impl_base::result_code::error:
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "Failed to convert character from "
                        "narrow to wide: Invalid input");
            }
            SCN_GCC_POP
            SCN_UNREACHABLE;
        }

        template <>
        struct localized_single_character_widener<wchar_t> {
        public:
            explicit constexpr localized_single_character_widener(
                detail::locale_ref) SCN_NOEXCEPT
            {
            }

            template <
                typename InputR,
                typename = std::enable_if_t<
                    std::is_same_v<ranges::range_value_t<InputR>, wchar_t>>>
            scan_expected<
                iterator_value_result<ranges::borrowed_iterator_t<InputR>,
                                      wchar_t>>
            operator()(InputR&& input)
            {
                SCN_EXPECT(ranges::begin(input) != ranges::end(input));

                auto it = ranges::begin(input);
                auto ch = *it++;
                return {{it, ch}};
            }
        };

        template <typename InputR, typename OutputR, typename OutputWidenedR>
        scan_expected<ranges::in_out_out_result<
            ranges::borrowed_iterator_t<InputR>,
            ranges::borrowed_iterator_t<OutputR>,
            ranges::borrowed_iterator_t<OutputWidenedR>>>
        read_until_localized_copy(InputR&& input,
                                  OutputR&& output,
                                  OutputWidenedR&& output_widened,
                                  detail::locale_ref loc,
                                  std::ctype_base::mask mask,
                                  bool mask_match)
        {
            using char_type = ranges::range_value_t<InputR>;
            static_assert(
                std::is_same_v<char_type, ranges::range_value_t<OutputR>>);
            static_assert(
                std::is_same_v<wchar_t, ranges::range_value_t<OutputWidenedR>>);

            auto input_it = ranges::begin(input);
            auto output_it = ranges::begin(output);
            auto output_widened_it = ranges::begin(output_widened);

            auto widen = localized_single_character_widener<char_type>{loc};
            SCN_UNUSED(widen);  // not really, gcc 9 is just buggy

            const auto& ctype = get_facet<std::ctype<wchar_t>>(loc);

            while (input_it != ranges::end(input) &&
                   output_it != ranges::end(output) &&
                   output_widened_it != ranges::end(output_widened)) {
                wchar_t next_char{};
                auto next_it = input_it;

                if constexpr (std::is_same_v<char_type, char>) {
                    auto result =
                        widen(ranges::subrange{input_it, ranges::end(input)});
                    if (!result) {
                        return unexpected(result.error());
                    }

                    next_it = result->iterator;
                    next_char = result->value;
                }
                else {
                    next_char = *input_it;
                    ++next_it;
                }

                if (ctype.is(mask, next_char) == mask_match) {
                    return {{input_it, output_it, output_widened_it}};
                }

                {
                    auto [in, out] = impl::copy(
                        ranges::subrange{input_it, next_it},
                        ranges::subrange{output_it, ranges::end(output)});
                    if (in != next_it) {
                        SCN_ENSURE(out == ranges::end(output));
                        return {{input_it, output_it, output_widened_it}};
                    }
                    output_it = out;
                }
                {
                    const auto next_char_end = &next_char + 1;
                    auto [in, out] = impl::copy(
                        ranges::subrange{&next_char, next_char_end},
                        ranges::subrange{output_widened_it,
                                         ranges::end(output_widened)});
                    if (in != next_char_end) {
                        SCN_ENSURE(out == ranges::end(output_widened));
                        return {{input_it, output_it, output_widened_it}};
                    }
                    output_widened_it = out;
                }

                input_it = next_it;
            }
            return {{input_it, output_it, output_widened_it}};
        }

        template <typename InputR>
        scan_expected<ranges::borrowed_iterator_t<InputR>>
        read_until_localized_skip(InputR&& input,
                                  detail::locale_ref loc,
                                  std::ctype_base::mask mask,
                                  bool mask_match)
        {
            return read_until_localized_copy(
                       SCN_FWD(input),
                       null_output_range<ranges::range_value_t<InputR>>{},
                       null_output_range<wchar_t>{}, loc, mask, mask_match)
                .transform([](auto result) SCN_NOEXCEPT { return result.in; });
        }

        template <typename Range>
        scan_expected<read_nocopy_result<Range>> read_until_localized_nocopy(
            Range&& range,
            detail::locale_ref loc,
            std::ctype_base::mask mask,
            bool mask_match)
        {
            static_assert(range_supports_nocopy<Range>());
            using char_type = ranges::range_value_t<Range>;

            auto make_result = [&](auto it) -> read_nocopy_result<Range> {
                const auto n = static_cast<size_t>(
                    ranges::distance(ranges::begin(range), it));
                return {it, {range_nocopy_data(range), n}};
            };
            auto it = ranges::begin(range);

            auto widen = localized_single_character_widener<char_type>{loc};
            SCN_UNUSED(widen);

            const auto& ctype = get_facet<std::ctype<wchar_t>>(loc);

            for (; it != ranges::end(range); ++it) {
                wchar_t next_char{};

                if constexpr (std::is_same_v<char_type, char>) {
                    auto result =
                        widen(ranges::subrange{it, ranges::end(range)});
                    if (!result) {
                        return unexpected(result.error());
                    }

                    next_char = result->value;
                }
                else {
                    next_char = *it;
                }

                if (ctype.is(mask, next_char) == mask_match) {
                    return make_result(it);
                }
            }
            return make_result(it);
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
