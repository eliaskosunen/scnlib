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

#include <scn/detail/ranges.h>
#include <scn/impl/algorithms/find_whitespace.h>
#include <scn/impl/unicode/unicode_whitespace.h>
#include <scn/impl/util/bits.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            bool has_nonascii_char_64(std::string_view source)
            {
                SCN_EXPECT(source.size() <= 8);
                uint64_t word{};
                std::memcpy(&word, source.data(), source.size());

                return has_byte_greater(word, 127) != 0;
            }

            template <typename Cb>
            std::string_view::iterator find_impl_ascii(std::string_view sv,
                                                       Cb cb)
            {
                return ranges::find_if(sv, cb);
            }

            template <typename Cb>
            std::string_view::iterator find_impl_unicode_invalid(
                std::string_view sv,
                Cb cb)
            {
                auto it = sv.begin();
                while (it != sv.end()) {
                    auto res = get_next_code_point(
                        detail::make_string_view_from_iterators<char>(
                            it, sv.end()));
                    if (cb(res.value)) {
                        return it;
                    }
                    it = res.iterator;
                }
                return it;
            }

            template <typename Cb>
            std::string_view::iterator find_impl_unicode_valid(
                std::string_view sv,
                const std::array<char32_t, 8>& codepoints,
                Cb cb)
            {
                for (size_t i = 0; i < codepoints.size(); ++i) {
                    if (cb(codepoints[i])) {
                        return sv.begin() + simdutf::utf8_length_from_utf32(
                                                codepoints.data(), i);
                    }
                }
                return sv.end();
            }

            std::string_view::iterator find_classic_space_impl(
                std::string_view source)
            {
                auto it = source.begin();
                while (it != source.end()) {
                    auto sv = detail::make_string_view_from_iterators<char>(
                                  it, source.end())
                                  .substr(0, 8);

                    if (!has_nonascii_char_64(sv)) {
                        it = find_impl_ascii(
                            sv, [](char ch) { return is_ascii_space(ch); });
                        if (it != sv.end()) {
                            return it;
                        }
                        continue;
                    }

                    std::array<char32_t, 8> codepoints{};
                    auto ret = simdutf::convert_utf8_to_utf32(
                        &*it, sv.size(), codepoints.data());
                    if (SCN_UNLIKELY(ret == 0)) {
                        it = find_impl_unicode_invalid(
                            sv, [](char32_t cp) { return is_cp_space(cp); });
                        if (it != sv.end()) {
                            return it;
                        }
                        continue;
                    }

                    it = find_impl_unicode_valid(
                        sv, codepoints,
                        [](char32_t cp) { return is_cp_space(cp); });
                    if (it != sv.end()) {
                        return it;
                    }
                }

                return source.end();
            }

            std::string_view::iterator find_classic_nonspace_impl(
                std::string_view source)
            {
                auto it = source.begin();
                while (it != source.end()) {
                    auto sv = detail::make_string_view_from_iterators<char>(
                                  it, source.end())
                                  .substr(0, 8);

                    if (!has_nonascii_char_64(sv)) {
                        it = find_impl_ascii(
                            sv, [](char ch) { return !is_ascii_space(ch); });
                        if (it != sv.end()) {
                            return it;
                        }
                        continue;
                    }

                    std::array<char32_t, 8> codepoints{};
                    auto ret = simdutf::convert_utf8_to_utf32(
                        &*it, sv.size(), codepoints.data());
                    if (SCN_UNLIKELY(ret == 0)) {
                        it = find_impl_unicode_invalid(
                            sv, [](char32_t cp) { return !is_cp_space(cp); });
                        if (it != sv.end()) {
                            return it;
                        }
                        continue;
                    }

                    it = find_impl_unicode_valid(
                        sv, codepoints,
                        [](char32_t cp) { return !is_cp_space(cp); });
                    if (it != sv.end()) {
                        return it;
                    }
                }

                return source.end();
            }

            bool is_decimal_digit(char ch) SCN_NOEXCEPT
            {
                static constexpr std::array<bool, 256> lookup = {
                    {false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     true,  true,  true,  true,  true,  true,  true,  true,
                     true,  true,  false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false,
                     false, false, false, false, false, false, false, false}};

                return lookup[static_cast<size_t>(
                    static_cast<unsigned char>(ch))];
            }

            std::string_view::iterator find_nondecimal_digit_simple_impl(
                std::string_view source)
            {
                return ranges::find_if(source, [](char ch) SCN_NOEXCEPT {
                    return !is_decimal_digit(ch);
                });
            }
        }  // namespace

        std::string_view::iterator find_classic_space_narrow_fast(
            std::string_view source)
        {
            return find_classic_space_impl(source);
        }

        std::string_view::iterator find_classic_nonspace_narrow_fast(
            std::string_view source)
        {
            return find_classic_nonspace_impl(source);
        }

        std::string_view::iterator find_nondecimal_digit_narrow_fast(
            std::string_view source)
        {
            return find_nondecimal_digit_simple_impl(source);
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
