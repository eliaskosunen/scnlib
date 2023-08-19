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

#include <scn/impl/reader/integer_reader.h>
#include <scn/impl/util/bits.h>

#include <limits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            constexpr std::array<uint64_t, 20> powers_of_ten{
                {1ull,
                 10ull,
                 100ull,
                 1000ull,
                 10'000ull,
                 100'000ull,
                 1'000'000ull,
                 10'000'000ull,
                 100'000'000ull,
                 1'000'000'000ull,
                 10'000'000'000ull,
                 100'000'000'000ull,
                 1'000'000'000'000ull,
                 10'000'000'000'000ull,
                 100'000'000'000'000ull,
                 1'000'000'000'000'000ull,
                 10'000'000'000'000'000ull,
                 100'000'000'000'000'000ull,
                 1'000'000'000'000'000'000ull,
                 10'000'000'000'000'000'000ull}};
            constexpr uint64_t power_of_10(int pw)
            {
                return powers_of_ten[static_cast<std::size_t>(pw)];
            }

            constexpr int count_digits(uint64_t x)
            {
                if (x >= 10'000'000'000ull) {
                    if (x >= 100'000'000'000'000ull) {
                        if (x >= 10'000'000'000'000'000ull) {
                            if (x >= 100'000'000'000'000'000ull) {
                                if (x >= 1'000'000'000'000'000'000ull) {
                                    return 19;
                                }
                                return 18;
                            }
                            return 17;
                        }
                        if (x >= 1'000'000'000'000'000ull) {
                            return 16;
                        }
                        return 15;
                    }
                    if (x >= 1'000'000'000'000ull) {
                        if (x >= 10'000'000'000'000ull) {
                            return 14;
                        }
                        return 13;
                    }
                    if (x >= 100'000'000'000ull) {
                        return 12;
                    }
                    return 11;
                }
                if (x >= 100'000ull) {
                    if (x >= 10'000'000ull) {
                        if (x >= 100'000'000ull) {
                            if (x >= 1'000'000'000ull) {
                                return 10;
                            }
                            return 9;
                        }
                        return 8;
                    }
                    if (x >= 1'000'000ull) {
                        return 7;
                    }
                    return 6;
                }
                if (x >= 100) {
                    if (x >= 1000) {
                        if (x >= 10000) {
                            return 5;
                        }
                        return 4;
                    }
                    return 3;
                }
                if (x >= 10) {
                    return 2;
                }
                return 1;
            }

            template <typename T>
            constexpr std::pair<T, T> div(T l, T r) SCN_NOEXCEPT
            {
                return {l / r, l % r};
            }

            template <typename T>
            constexpr bool can_do_fast64_at_least_once()
            {
                return count_digits(std::numeric_limits<T>::max()) >= 8;
            }

            template <typename T>
            constexpr bool can_do_fast64_multiple_times()
            {
                return count_digits(std::numeric_limits<T>::max()) > 8;
            }

            template <typename T>
            struct int_reader_state {
                using utype = std::make_unsigned_t<T>;

                // Max for uint
                static constexpr auto uint_max = static_cast<utype>(-1);
                // Max for sint
                static constexpr auto int_max =
                    static_cast<utype>(uint_max / 2);  // >> 1
                // Absolute value of min for sint
                static constexpr auto abs_int_min =
                    static_cast<utype>(int_max + 1);

                int_reader_state(int base, numeric_reader_base::sign s)
                    : ubase(static_cast<utype>(base)),
                      sign(s),
                      limit([&]() -> utype {
                          if constexpr (std::is_signed_v<T>) {
                              if (sign ==
                                  numeric_reader_base::sign::minus_sign) {
                                  return abs_int_min;
                              }
                              return int_max;
                          }
                          return uint_max;
                      }()),
                      cutlimits(scn::impl::div(limit, ubase))
                {
                }

                constexpr auto cutoff() const
                {
                    return cutlimits.first;
                }
                constexpr auto cutlim() const
                {
                    return cutlimits.second;
                }

                utype accumulator{};
                const utype ubase;
                const numeric_reader_base::sign sign;
                const utype limit;
                const std::pair<utype, utype> cutlimits;
            };

            template <typename UType, typename T>
            const char* do_single_char_impl(UType digit,
                                            int_reader_state<T>& state)
            {
                auto overflow_error_msg = [sign = state.sign]() {
                    if (sign == numeric_reader_base::sign::minus_sign) {
                        return "Out of range: integer overflow";
                    }
                    return "Out of range: integer underflow";
                };

                if (SCN_UNLIKELY(state.accumulator > state.cutoff() ||
                                 (state.accumulator == state.cutoff() &&
                                  digit > state.cutlim()))) {
                    SCN_UNLIKELY_ATTR
                    return overflow_error_msg();
                }

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")

                state.accumulator *= state.ubase;
                state.accumulator += digit;

                SCN_GCC_POP

                return nullptr;
            }

            template <typename CharT, typename T>
            auto do_single_char(CharT ch, int_reader_state<T>& state)
                -> std::pair<bool, scan_error>
            {
                using utype = typename int_reader_state<T>::utype;

                const auto digit =
                    static_cast<utype>(numeric_reader_base::char_to_int(ch));
                if (digit >= state.ubase) {
                    return {false, {}};
                }

                if (auto msg = do_single_char_impl(digit, state);
                    SCN_UNLIKELY(msg != nullptr)) {
                    return {false, {scan_error::value_out_of_range, msg}};
                }
                return {true, {}};
            }

            constexpr std::array<uint64_t, 9> accumulator_multipliers{{
                0,
                power_of_10(1),
                power_of_10(2),
                power_of_10(3),
                power_of_10(4),
                power_of_10(5),
                power_of_10(6),
                power_of_10(7),
                power_of_10(8),
            }};

            template <typename T,
                      typename State,
                      typename Iterator,
                      typename Sentinel>
            SCN_MAYBE_UNUSED scan_error do_read_decimal_fast64(State& state,
                                                               Iterator& it,
                                                               Sentinel end,
                                                               bool& stop_fast)
            {
                SCN_EXPECT(end - it >= 8);
                uint64_t word{};
                std::memcpy(&word, &*it, 8);

#if 1
                constexpr std::size_t digits_in_word = 8;

                // Check if word is all decimal digits, from:
                // https://lemire.me/blog/2018/09/30/quickly-identifying-a-sequence-of-digits-in-a-string-of-characters/
                if ((word & (word + 0x0606060606060606ull) &
                     0xF0F0F0F0F0F0F0F0ull) != 0x3030303030303030ull) {
                    // Bail out, use simpler loop
                    stop_fast = true;
                    return {};
                }
#else
                // Alternative (slower) approach, where we count the number of
                // decimal digits in the string, and mask them out
                std::size_t digits_in_word{};

                {
                    // Check for non-decimal characters,
                    // mask them out if there's any in the end
                    // (replace with '0' = 0x30)

                    const auto masked = has_byte_between(word, '0', '9');
                    digits_in_word =
                        get_index_of_first_nonmatching_byte(masked);
                    if (digits_in_word != 8) {
                        stop_fast = true;
                    }
                    if (digits_in_word == 0) {
                        return {};
                    }

                    const uint64_t shift = 8 * (8 - digits_in_word);
                    word <<= shift;
                    const uint64_t mask = (~0ull) << shift;
                    word = (mask & word) | (~mask & 0x3030303030303030ull);
                }
#endif

                it += digits_in_word;

                {
                    // Bit-twiddle ascii decimal chars to become an actual
                    // integer, from:
                    // https://lemire.me/blog/2022/01/21/swar-explained-parsing-eight-digits/

                    constexpr uint64_t mask = 0x000000FF000000FFull;
                    constexpr uint64_t mul1 = 100 + (1000000ull << 32);
                    constexpr uint64_t mul2 = 1 + (10000ull << 32);

                    word -= 0x3030303030303030ull;
                    word = (word * 10) + (word >> 8);
                    word = (((word & mask) * mul1) +
                            (((word >> 16) & mask) * mul2)) >>
                           32;
                }

                using utype = typename State::utype;

#if SCN_HAS_BUILTIN_OVERFLOW
                if (state.accumulator == 0) {
                    if (static_cast<uint64_t>(
                            std::numeric_limits<utype>::max()) <
                            1'0000'0000ull &&
                        word > static_cast<uint64_t>(state.limit)) {
                        SCN_UNLIKELY_ATTR
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }

                    state.accumulator = static_cast<utype>(word);
                }
                else {
                    SCN_EXPECT(can_do_fast64_multiple_times<T>());
                    const utype accumulator_multiplier = static_cast<utype>(
                        accumulator_multipliers[digits_in_word]);

                    if (SCN_UNLIKELY(
                            __builtin_mul_overflow(state.accumulator,
                                                   accumulator_multiplier,
                                                   &state.accumulator) ||
                            __builtin_add_overflow(state.accumulator,
                                                   static_cast<utype>(word),
                                                   &state.accumulator))) {
                        SCN_UNLIKELY_ATTR
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }
                }
#else
                const utype accumulator_multiplier =
                    can_do_fast64_multiple_times<T>()
                        ? static_cast<utype>(
                              accumulator_multipliers[digits_in_word])
                        : 1;
                SCN_EXPECT(accumulator_multiplier != 0);

                if (state.accumulator == 0) {
                    if (static_cast<uint64_t>(state.limit) >= 1'0000'0000ull &&
                        word > static_cast<uint64_t>(state.limit)) {
                        SCN_UNLIKELY_ATTR
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }
                }
                else {
                    SCN_EXPECT(accumulator_multiplier != 1);
                    // accumulator * multiplier + word > limit -> overflow
                    // accumulator * multiplier > limit - word
                    // accumulator > (limit - word) / multiplier
                    if (state.accumulator >
                        ((state.limit - static_cast<utype>(word)) /
                         accumulator_multiplier)) {
                        SCN_UNLIKELY_ATTR
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }
                }

                if (state.accumulator > 0) {
                    SCN_EXPECT(can_do_fast64_multiple_times<T>());
                    SCN_EXPECT(accumulator_multiplier != 1);

                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wconversion")

                    state.accumulator =
                        state.accumulator * accumulator_multiplier +
                        static_cast<utype>(word);

                    SCN_GCC_POP
                }
                else {
                    state.accumulator = static_cast<utype>(word);
                }
#endif  // SCN_HAS_BUILTIN_OVERFLOW

                return {};
            }

            template <typename T>
            void store_value(const int_reader_state<T>& state,
                             T& value,
                             numeric_reader_base::sign sign)
            {
                if (sign == numeric_reader_base::sign::minus_sign) {
                    if (SCN_UNLIKELY(state.accumulator == state.abs_int_min)) {
                        value = std::numeric_limits<T>::min();
                    }
                    else {
                        SCN_MSVC_PUSH
                        SCN_MSVC_IGNORE(
                            4146)  // unary minus applied to unsigned
                        value =
                            static_cast<T>(-static_cast<T>(state.accumulator));
                        SCN_MSVC_POP
                    }
                }
                else {
                    value = static_cast<T>(state.accumulator);
                }
            }
            template <typename T>
            void store_value_if_out_of_range(const scan_error& err,
                                             T& value,
                                             numeric_reader_base::sign sign)
            {
                if (err.code() != scan_error::value_out_of_range) {
                    return;
                }

                if (sign == numeric_reader_base::sign::minus_sign) {
                    value = std::numeric_limits<T>::min();
                }
                else {
                    value = std::numeric_limits<T>::max();
                }
            }

        }  // namespace

        template <typename CharT>
        template <typename T>
        scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
        integer_reader<CharT>::parse_value_impl(T& value)
        {
            auto source = this->m_buffer.view();

            SCN_EXPECT(!source.empty());
            SCN_EXPECT(std::is_signed_v<T> ||
                       m_sign == numeric_reader_base::sign::plus_sign);
            SCN_EXPECT(m_sign != numeric_reader_base::sign::default_sign);
            SCN_EXPECT(m_base > 0);

            int_reader_state<T> state{m_base, m_sign};
            auto it = source.begin();

            if (this->char_to_int(*it) >= m_base) {
                SCN_UNLIKELY_ATTR
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid integer value");
            }

            if constexpr (std::is_same_v<CharT, char> &&
                          can_do_fast64_at_least_once<T>() && !SCN_IS_32BIT &&
                          !SCN_IS_BIG_ENDIAN && SCN_HAS_BITS_CTZ) {
                if (state.ubase == 10) {
                    for (bool stop_reading = false;
                         source.end() - it >= 8 && !stop_reading;) {
                        if (auto err = do_read_decimal_fast64<T>(
                                state, it, source.end(), stop_reading);
                            SCN_UNLIKELY(!err)) {
                            store_value_if_out_of_range(err, value, m_sign);
                            return unexpected(err);
                        }
                        if constexpr (!can_do_fast64_multiple_times<T>()) {
                            break;
                        }
                    }
                }
            }

            for (; /*!stop_reading &&*/ it != source.end(); ++it) {
                if (const auto [keep_going, err] = do_single_char(*it, state);
                    SCN_UNLIKELY(!err)) {
                    store_value_if_out_of_range(err, value, m_sign);
                    return unexpected(err);
                }
                else if (!keep_going) {
                    break;
                }
            }

            store_value(state, value, m_sign);
            return it;
        }

#define SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, IntT)     \
    template auto integer_reader<CharT>::parse_value_impl(IntT&) \
        ->scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>;

#define SCN_DEFINE_INTEGER_READER_TEMPLATE(CharT)                  \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL(CharT, unsigned long long)

        SCN_DEFINE_INTEGER_READER_TEMPLATE(char)
        SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t)

#undef SCN_DEFINE_INTEGER_READER_TEMPLATE
#undef SCN_DEFINE_INTEGER_READER_TEMPLATE_IMPL
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
