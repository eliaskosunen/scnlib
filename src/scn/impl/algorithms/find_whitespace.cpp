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
            template <typename R>
            bool has_nonascii_char_64(R source)
            {
                SCN_EXPECT(source.size() <= 8);
                uint64_t word{};
                std::memcpy(&word, source.data(), source.size());

                return has_byte_greater(word, 127) != 0;
            }

            template <typename R, typename Cb>
            auto find_impl_ascii(R sv, Cb cb)
            {
                return ranges::find_if(sv, cb);
            }

            template <typename Cb>
            auto find_impl_unicode_invalid(std::string_view sv, Cb cb)
            {
                auto it = sv.data();
                while (it != sv.data() + sv.size()) {
                    auto tmp = std::string_view{
                        detail::to_address(it),
                        static_cast<size_t>(ranges::distance(
                            it, detail::to_address(sv.end())))};
                    auto res = get_next_code_point(tmp);
                    if (cb(res.value)) {
                        break;
                    }
                    it += ranges::distance(tmp.data(),
                                           detail::to_address(res.iterator));
                }
                return sv.begin() + ranges::distance(sv.data(), it);
            }

            template <typename Cb>
            auto find_impl_unicode_valid(
                std::string_view sv,
                const std::array<char32_t, 8>& codepoints,
                Cb cb)
            {
                for (size_t i = 0; i < codepoints.size(); ++i) {
                    if (cb(codepoints[i])) {
                        return sv.begin() + static_cast<std::ptrdiff_t>(
                                                simdutf::utf8_length_from_utf32(
                                                    codepoints.data(), i));
                    }
                }
                return sv.end();
            }

            template <typename CuCb, typename CpCb>
            std::string_view::iterator
            find_classic_impl(std::string_view source, CuCb cu_cb, CpCb cp_cb)
            {
                auto it = source.data();
                const auto end = source.data() + source.size();

                while (it != end) {
                    auto sv =
                        std::string_view{
                            it, static_cast<size_t>(ranges::distance(
                                    it, detail::to_address(source.end())))}
                            .substr(0, 8);

                    if (!has_nonascii_char_64(sv)) {
                        auto i = find_impl_ascii(sv, cu_cb);
                        it = detail::to_address(i);
                        if (i != sv.end()) {
                            break;
                        }
                        continue;
                    }

                    std::array<char32_t, 8> codepoints{};
                    auto ret = simdutf::convert_utf8_to_utf32(
                        detail::to_address(it), sv.size(), codepoints.data());
                    if (SCN_UNLIKELY(ret == 0)) {
                        auto i = find_impl_unicode_invalid(sv, cp_cb);
                        it = detail::to_address(i);
                        if (i != sv.end()) {
                            break;
                        }
                        continue;
                    }

                    auto i = find_impl_unicode_valid(sv, codepoints, cp_cb);
                    it = detail::to_address(i);
                    if (i != sv.end()) {
                        break;
                    }
                }

                return source.begin() + ranges::distance(source.data(), it);
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
            return find_classic_impl(
                source, [](char ch) { return is_ascii_space(ch); },
                [](char32_t cp) { return is_cp_space(cp); });
        }

        std::string_view::iterator find_classic_nonspace_narrow_fast(
            std::string_view source)
        {
            return find_classic_impl(
                source, [](char ch) { return !is_ascii_space(ch); },
                [](char32_t cp) { return !is_cp_space(cp); });
        }

        std::string_view::iterator find_nondecimal_digit_narrow_fast(
            std::string_view source)
        {
            return find_nondecimal_digit_simple_impl(source);
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
