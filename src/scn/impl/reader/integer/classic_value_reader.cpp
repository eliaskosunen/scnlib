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

#include <scn/impl/reader/integer/classic_value_reader.h>

#include <scn/impl/util/bits.h>

#include <limits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        namespace {
            SCN_NODISCARD uint8_t _char_to_int(char ch)
            {
                static constexpr uint8_t digits_arr[] = {
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
                    255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,
                    17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
                    29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255,
                    255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
                    21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                    33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255};
                return digits_arr[static_cast<unsigned char>(ch)];
            }
            SCN_NODISCARD uint8_t _char_to_int(wchar_t ch)
            {
#if WCHAR_MIN < 0
                if (ch >= 0 && ch <= 255) {
#else
                if (ch <= 255) {
#endif
                    return _char_to_int(static_cast<char>(ch));
                }
                return 255;
            }

            enum sign_type : uint8_t { plus_sign, minus_sign };

            template <typename CharT>
            std::pair<bool, sign_type> get_sign(
                std::basic_string_view<CharT> source)
            {
                SCN_EXPECT(!source.empty());

                switch (source[0]) {
                    case CharT{'-'}:
                        return {true, minus_sign};

                    case CharT{'+'}:
                        return {true, plus_sign};

                    default:
                        return {false, plus_sign};
                }

                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }

            enum class base_prefix_state {
                base_not_determined,
                base_determined_from_prefix,
                base_determined_from_zero_prefix,
                zero_parsed,
            };

            struct base_prefix_result {
                base_prefix_state state;
                int offset;
                int parsed_base{0};
            };

            template <typename CharT>
            base_prefix_result get_base_prefix(
                std::basic_string_view<CharT> source)
            {
                SCN_EXPECT(!source.empty());

                if (source[0] != CharT{'0'}) {
                    // Non-'0' initial character, surely not a base prefix
                    return {base_prefix_state::base_not_determined, 0};
                }
                if (source.size() == 1) {
                    // Only '0' parsed -> it's a zero
                    return {base_prefix_state::zero_parsed, 1};
                }

                switch (source[1]) {
                    case CharT{'x'}:
                    case CharT{'X'}:
                        // Starts with "0x"/"0X" -> hex
                        return {base_prefix_state::base_determined_from_prefix,
                                2, 16};

                    case CharT{'o'}:
                    case CharT{'O'}:
                        // Starts with "0o"/"0O" -> oct
                        return {base_prefix_state::base_determined_from_prefix,
                                2, 8};
                        break;

                    case CharT{'b'}:
                    case CharT{'B'}:
                        // Starts with "0b"/"0B" -> bin
                        return {base_prefix_state::base_determined_from_prefix,
                                2, 2};
                        break;

                    case CharT{'0'}:
                    case CharT{'1'}:
                    case CharT{'2'}:
                    case CharT{'3'}:
                    case CharT{'4'}:
                    case CharT{'5'}:
                    case CharT{'6'}:
                    case CharT{'7'}:
                        // Starts with "0_" where _ is a valid octal digit ->
                        // C-like octal prefix
                        return {
                            base_prefix_state::base_determined_from_zero_prefix,
                            1, 8};

                    default:
                        // "0_", where _ is some other char -> not a base prefix
                        return {base_prefix_state::base_not_determined, 0};
                }
            }

            enum class prefix_parse_result {
                zero_parsed,
                keep_parsing,
            };

            template <typename T, typename CharT>
            scan_expected<std::pair<prefix_parse_result, sign_type>>
            parse_prefix(std::basic_string_view<CharT>& source,
                         unsigned options,
                         int& base)
            {
                SCN_EXPECT(!source.empty());

                const auto sign_result = get_sign(source);
                if constexpr (std::is_signed_v<T>) {
                    if (sign_result.second == minus_sign &&
                        (options &
                         int_classic_value_reader_base::only_unsigned) != 0) {
                        // 'u' option -> negative values disallowed
                        SCN_UNLIKELY_ATTR
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Parsed negative value, when 'u' format options "
                            "was given");
                    }
                }
                else {
                    if (SCN_UNLIKELY(sign_result.second == minus_sign)) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Unexpected sign '-' when scanning an unsigned "
                            "integer");
                    }
                }

                if (sign_result.first) {
                    source = source.substr(1);

                    if (SCN_UNLIKELY(source.empty())) {
                        return unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Expected number after sign");
                    }
                }

                auto base_result = get_base_prefix(source);

                auto do_return = [&](prefix_parse_result r) {
                    source =
                        source.substr(static_cast<size_t>(base_result.offset));
                    return std::make_pair(r, sign_result.second);
                };

                if (base_result.state == base_prefix_state::zero_parsed) {
                    return do_return(prefix_parse_result::zero_parsed);
                }

                if (base == 0) {
                    // Format string was 'i' or default ->
                    // base needs to be detected
                    // Use parsed_base, if one could be determined,
                    // default to base 10
                    base = base_result.state ==
                                   base_prefix_state::base_not_determined
                               ? 10
                               : base_result.parsed_base;
                    SCN_ENSURE(base != 0);
                    return do_return(prefix_parse_result::keep_parsing);
                }

                if ((options &
                     int_classic_value_reader_base::allow_base_prefix) != 0) {
                    if (base_result.state ==
                            base_prefix_state::base_determined_from_prefix &&
                        base_result.parsed_base != base) {
                        // Parsed base prefix for different base in the format
                        // "0_", go back a single char
                        --base_result.offset;
                    }
                }
                else if (SCN_UNLIKELY(base_result.state !=
                                      base_prefix_state::base_not_determined)) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Parsed a base prefix, even though one isn't allowed");
                }

                return do_return(prefix_parse_result::keep_parsing);
            }

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
        }  // namespace

        template <typename T>
        struct int_reader_state {
            using utype = std::make_unsigned_t<T>;

            // Max for uint
            static constexpr auto uint_max = static_cast<utype>(-1);
            // Max for sint
            static constexpr auto int_max =
                static_cast<utype>(uint_max / 2);  // >> 1
            // Absolute value of min for sint
            static constexpr auto abs_int_min = static_cast<utype>(int_max + 1);

            int_reader_state(int base, sign_type s)
                : ubase(static_cast<utype>(base)),
                  sign(s),
                  limit([&]() -> utype {
                      if constexpr (std::is_signed_v<T>) {
                          if (sign == minus_sign) {
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
            const sign_type sign;
            const utype limit;
            const std::pair<utype, utype> cutlimits;
        };

        namespace {
            template <typename UType, typename T>
            const char* do_single_char_impl(UType digit,
                                            int_reader_state<T>& state)
            {
                auto overflow_error_msg = [sign = state.sign]() {
                    if (sign == minus_sign) {
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
        }  // namespace

        template <typename CharT, typename T>
        auto do_single_char(CharT ch, int_reader_state<T>& state)
            -> std::pair<bool, scan_error>
        {
            using utype = typename int_reader_state<T>::utype;

            const auto digit = static_cast<utype>(_char_to_int(ch));
            if (digit >= state.ubase) {
                return {false, {}};
            }

            if (auto msg = do_single_char_impl(digit, state);
                SCN_UNLIKELY(msg != nullptr)) {
                return {false, {scan_error::value_out_of_range, msg}};
            }
            return {true, {}};
        }

        template <typename CharT, typename T>
        auto do_single_char_with_thsep(
            int_reader_state<T>& state,
            typename std::basic_string_view<CharT>::iterator it,
            typename std::basic_string_view<CharT>::iterator&
                after_last_thsep_it,
            std::string& thousands_separators) -> std::pair<bool, scan_error>
        {
            using utype = typename int_reader_state<T>::utype;

            const auto digit = static_cast<utype>(_char_to_int(*it));
            if (digit >= state.ubase) {
                if (*it == ',') {
                    thousands_separators.push_back(
                        static_cast<char>(it - after_last_thsep_it));
                    after_last_thsep_it = it + 1;
                    return {true, {}};
                }

                return {false, {}};
            }

            if (auto msg = do_single_char_impl(digit, state);
                SCN_UNLIKELY(msg != nullptr)) {
                return {false, {scan_error::value_out_of_range, msg}};
            }
            return {true, {}};
        }

        namespace {
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

            template <typename CharT, typename T>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            parse_zero(std::basic_string_view<CharT> source, T& value)
            {
                value = 0;
                return source.begin();
            }

            scan_error check_thousands_separators(
                const std::string& thousands_separators)
            {
                const auto return_error = []() {
                    return scan_error{scan_error::invalid_scanned_value,
                                      "Invalid thousands separator grouping"};
                };

                if (thousands_separators[0] > 3) {
                    SCN_UNLIKELY_ATTR
                    return return_error();
                }
                for (auto it = thousands_separators.begin() + 1;
                     it != thousands_separators.end(); ++it) {
                    if (*it != 3) {
                        SCN_UNLIKELY_ATTR
                        return return_error();
                    }
                }

                return {};
            }

            template <typename CharT, typename T>
            scan_expected<ranges::iterator_t<std::basic_string_view<CharT>>>
            do_read(const int_classic_value_reader_base& reader,
                    std::basic_string_view<CharT> source,
                    T& value,
                    sign_type sign)
            {
                if (SCN_UNLIKELY(source.empty())) {
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Invalid integer value");
                }

                SCN_EXPECT(std::is_signed_v<T> || sign == plus_sign);
                SCN_EXPECT(reader.base > 0);

                int_reader_state<T> state{reader.base, sign};
                auto it = source.begin();

                if (_char_to_int(*it) >= reader.base) {
                    SCN_UNLIKELY_ATTR
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Invalid integer value");
                }

                if ((reader.options &
                     int_classic_value_reader_base::allow_thsep) == 0) {
                    // No thsep
                    bool stop_reading = false;
                    SCN_UNUSED(stop_reading);

                    if constexpr (std::is_same_v<CharT, char> &&
                                  can_do_fast64_at_least_once<T>() &&
                                  !SCN_IS_32BIT && !SCN_IS_BIG_ENDIAN &&
                                  SCN_HAS_BITS_CTZ) {
                        if (state.ubase == 10) {
                            while (source.end() - it >= 8 && !stop_reading) {
                                if (auto err = do_read_decimal_fast64<T>(
                                        state, it, source.end(), stop_reading);
                                    SCN_UNLIKELY(!err)) {
                                    return unexpected(err);
                                }
                                if constexpr (!can_do_fast64_multiple_times<
                                                  T>()) {
                                    break;
                                }
                            }
                        }
                    }

                    for (; /*!stop_reading &&*/ it != source.end(); ++it) {
                        if (const auto [keep_going, err] =
                                do_single_char(*it, state);
                            SCN_UNLIKELY(!err)) {
                            return unexpected(err);
                        }
                        else if (!keep_going) {
                            break;
                        }
                    }
                }
                else {
                    // Allow thsep
                    std::string thousands_separators{};
                    auto after_last_thsep_it = it;

                    for (; it != source.end(); ++it) {
                        if (const auto [keep_going, err] =
                                do_single_char_with_thsep<CharT>(
                                    state, it, after_last_thsep_it,
                                    thousands_separators);
                            SCN_UNLIKELY(!err)) {
                            return unexpected(err);
                        }
                        else if (!keep_going) {
                            break;
                        }
                    }

                    if (!thousands_separators.empty()) {
                        if (auto e = check_thousands_separators(
                                thousands_separators);
                            SCN_UNLIKELY(!e)) {
                            return unexpected(e);
                        }
                    }
                }

                if (sign == minus_sign) {
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

                return it;
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto int_classic_value_reader<CharT>::read(string_view_type source,
                                                   T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            SCN_EXPECT(!source.empty());

            return parse_prefix<T>(source, options, base)
                .and_then([&](auto result) {
                    if (result.first == prefix_parse_result::zero_parsed) {
                        return parse_zero(source, value);
                    }

                    return do_read(*this, source, value, result.second);
                });
        }

#define SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, IntT)    \
    template auto int_classic_value_reader<CharT>::read(string_view_type, \
                                                        IntT&)            \
        ->scan_expected<ranges::iterator_t<string_view_type>>;

#define SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE(CharT)                  \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, signed char)    \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, short)          \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, int)            \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, long)           \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, long long)      \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned char)  \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned short) \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned int)   \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long)  \
    SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL(CharT, unsigned long long)

        SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE(char)
        SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE(wchar_t)

#undef SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE
#undef SCN_DEFINE_INT_CLASSIC_VALUE_READER_TEMPLATE_IMPL
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
