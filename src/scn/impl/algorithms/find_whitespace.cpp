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

#include <scn/impl/algorithms/find_whitespace.h>

#include <scn/impl/util/ascii_ctype.h>
#include <scn/impl/util/bits.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            template <typename F>
            std::string_view::iterator find_simple_until_impl(
                std::string_view source,
                F until)
            {
                for (auto it = source.begin(); it != source.end(); ++it) {
                    if (until(*it)) {
                        return it;
                    }
                }
                return source.end();
            }

            std::string_view::iterator find_classic_space_simple_impl(
                std::string_view source)
            {
                return find_simple_until_impl(
                    source, [](char ch) { return is_ascii_space(ch); });
            }

            std::string_view::iterator find_classic_nonspace_simple_impl(
                std::string_view source)
            {
                return find_simple_until_impl(
                    source, [](char ch) { return !is_ascii_space(ch); });
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
                return find_simple_until_impl(
                    source, [](char ch) { return !is_decimal_digit(ch); });
            }

            template <typename GetMask, typename Fallback>
            std::string_view::iterator find_64_match_impl(
                std::string_view source,
                GetMask get_mask,
                Fallback fallback)
            {
                constexpr uint64_t match_mask = ~0ul / 255 * 0x80ull;
                size_t i = 0;
                for (; i + 8 <= source.size(); i += 8) {
                    uint64_t word{};
                    memcpy(&word, source.data() + i, 8);

                    const auto masked = get_mask(word);
                    if (masked == 0) {
                        continue;
                    }
                    const auto next_i =
                        get_index_of_first_matching_byte(masked, match_mask);
                    return source.begin() + i + next_i;
                }

                return fallback({source.data() + i, source.size() - i});
            }

            std::string_view::iterator find_classic_space_64_impl(
                std::string_view source)
            {
                const auto get_mask = [](uint64_t word) -> uint64_t {
                    constexpr uint64_t space_mask =
                        ~0ul / 255 * static_cast<uint64_t>(' ');

                    // Has non-zero byte on space (' ') chars
                    uint64_t space_masked = has_zero_byte(word ^ space_mask);
                    // Has non-zero (0x80) byte on otherspace chars
                    uint64_t otherspaces_masked =
                        has_byte_between(word, '\t', '\r');

                    return space_masked | otherspaces_masked;
                };

                return find_64_match_impl(source, get_mask,
                                          find_classic_space_simple_impl);
            }

            template <typename GetMask, typename Fallback>
            std::string_view::iterator find_64_nonmatch_impl(
                std::string_view source,
                GetMask get_mask,
                Fallback fallback)
            {
                size_t i = 0;
                for (; i + 8 <= source.size(); i += 8) {
                    uint64_t word{};
                    memcpy(&word, source.data() + i, 8);

                    const auto masked = get_mask(word);
                    const auto next_i =
                        get_index_of_first_nonmatching_byte(masked);
                    if (next_i != 8) {
                        return source.begin() + i + next_i;
                    }
                }

                return fallback({source.data() + i, source.size() - i});
            }

            std::string_view::iterator find_classic_nonspace_64_impl(
                std::string_view source)
            {
                const auto get_mask = [](uint64_t word) -> uint64_t {
                    constexpr uint64_t space_mask =
                        ~0ull / 255 * static_cast<uint64_t>(' ');

                    // Has non-zero byte on space (' ') chars
                    uint64_t space_masked = has_zero_byte(word ^ space_mask);
                    // Has non-zero (0x80) byte on otherspace chars
                    uint64_t otherspaces_masked =
                        has_byte_between(word, '\t', '\r');

                    return space_masked | otherspaces_masked;
                };

                return find_64_nonmatch_impl(source, get_mask,
                                             find_classic_nonspace_simple_impl);
            }

            std::string_view::iterator find_nondecimal_digit_64_impl(
                std::string_view source)
            {
                const auto get_mask = [](uint64_t word) -> uint64_t {
                    return has_byte_between(word, '0', '9');
                };

                return find_64_nonmatch_impl(source, get_mask,
                                             find_nondecimal_digit_simple_impl);
            }
        }  // namespace

        std::string_view::iterator find_classic_space_narrow_fast(
            std::string_view source)
        {
            if constexpr (sizeof(void*) == 8) {
                return find_classic_space_64_impl(source);
            } else {
                return find_classic_space_simple_impl(source);
            }
        }

        std::string_view::iterator find_classic_nonspace_narrow_fast(
            std::string_view source)
        {
            if constexpr (sizeof(void*) == 8) {
                return find_classic_nonspace_64_impl(source);
            } else {
                return find_classic_nonspace_simple_impl(source);
            }
        }

        std::string_view::iterator find_nondecimal_digit_narrow_fast(
            std::string_view source)
        {
            if constexpr (sizeof(void*) == 8) {
                return find_nondecimal_digit_64_impl(source);
            } else {
                return find_nondecimal_digit_simple_impl(source);
            }
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
