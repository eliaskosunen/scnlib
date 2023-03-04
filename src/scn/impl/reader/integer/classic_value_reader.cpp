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
        }  // namespace

        template <typename CharT>
        auto int_classic_value_reader<CharT>::parse_sign_unsigned(
            string_view_type& source) -> scan_error
        {
            if (SCN_UNLIKELY(source[0] == CharT{'-'})) {
                return scan_error{scan_error::invalid_scanned_value,
                                  "Unexpected sign '-' when scanning an "
                                  "unsigned integer"};
            }

            if (source[0] == CharT{'+'}) {
                source = source.substr(1);
            }

            if (SCN_UNLIKELY(source.empty())) {
                return scan_error{scan_error::invalid_scanned_value,
                                  "Expected number after sign"};
            }
            return {};
        }

        template <typename CharT>
        auto int_classic_value_reader<CharT>::parse_sign_signed(
            string_view_type& source) -> scan_expected<sign_type>
        {
            sign_type sign = plus_sign;

            if (source[0] == CharT{'-'}) {
                if (SCN_UNLIKELY((m_options & only_unsigned) != 0)) {
                    // 'u' option -> negative values disallowed
                    return unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "Parsed negative value, when 'u' format "
                        "option was given");
                }
                sign = minus_sign;
                source = source.substr(1);
            }
            else if (source[0] == CharT{'+'}) {
                source = source.substr(1);
            }

            if (SCN_UNLIKELY(source.empty())) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Expected number after potential sign character sign");
            }

            return {sign};
        }

        template <typename CharT>
        auto int_classic_value_reader<CharT>::parse_base_prefix(
            string_view_type source) -> base_prefix_result
        {
            SCN_EXPECT(!source.empty());

            auto it = source.begin();
            if (*it != CharT{'0'}) {
                // All base prefixes start with '0'
                return {it, base_prefix_state::base_not_determined};
            }
            // *it is now '0'

            ++it;
            if (it == source.end()) {
                // Only '0' parsed
                return {it, base_prefix_state::zero_parsed};
            }

            // Check for possible base prefix
            int base_prefix = 0;
            switch (static_cast<char>(*it)) {
                case 'b':
                case 'B':
                    base_prefix = 2;
                    break;
                case 'o':
                case 'O':
                    base_prefix = 8;
                    break;
                case 'x':
                case 'X':
                    base_prefix = 16;
                    break;
                default:
                    break;
            }
            if (base_prefix != 0) {
                // Starts with "0_", skip the '_'
                ++it;
                return {it, base_prefix_state::base_determined_from_prefix,
                        static_cast<int8_t>(base_prefix)};
            }

            // Starts with "0_", where '_' is not a base prefix char.
            // If '_' is an octal digit, base is 8.
            // Else, parse only '0'
            if (_char_to_int(*it) < 8) {
                // Octal digit
                return {it, base_prefix_state::base_determined_from_zero_prefix,
                        8};
            }
            return {it, base_prefix_state::base_not_determined};
        }

        template <typename CharT>
        auto int_classic_value_reader<CharT>::parse_and_determine_base(
            string_view_type& source) -> scan_expected<determine_base_result>
        {
            auto result = parse_base_prefix(source);
            if (result.state == base_prefix_state::zero_parsed) {
                source = detail::make_string_view_from_iterators<CharT>(
                    result.iterator, source.end());
                return {determine_base_result::zero_parsed};
            }

            if (m_base == 0) {
                // Format string was 'i' or default ->
                // base needs to be detected
                if (result.state == base_prefix_state::base_not_determined) {
                    // None detected -> base 10
                    m_base = 10;
                }
                else {
                    m_base = result.parsed_base;
                }
            }
            else if ((m_options & allow_base_prefix) != 0) {
                if (result.state ==
                        base_prefix_state::base_determined_from_prefix &&
                    result.parsed_base != m_base) {
                    // Parsed base prefix for different base in the format "0_",
                    // go back a single char
                    source = detail::make_string_view_from_iterators<CharT>(
                        --result.iterator, source.end());
                    return {determine_base_result::keep_parsing};
                }
            }
            else if (result.state != base_prefix_state::base_not_determined) {
                return unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Parsed a base prefix, even though one isn't allowed");
            }

            SCN_ENSURE(m_base != 0);

            source = detail::make_string_view_from_iterators<CharT>(
                result.iterator, source.end());
            return {determine_base_result::keep_parsing};
        }

        template <typename CharT>
        scan_error int_classic_value_reader<CharT>::check_thousands_separators(
            const std::string& thousands_separators) const
        {
            const auto return_error = []() {
                return scan_error{scan_error::invalid_scanned_value,
                                  "Invalid thousands separator grouping"};
            };

            if (thousands_separators[0] > 3) {
                return return_error();
            }
            for (auto it = thousands_separators.begin() + 1;
                 it != thousands_separators.end(); ++it) {
                if (*it != 3) {
                    return return_error();
                }
            }

            return {};
        }

        namespace {
            inline uint64_t power_of_10(int pw)
            {
                static constexpr std::array<uint64_t, 20> powers{
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
                return powers[static_cast<std::size_t>(pw)];
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
            constexpr bool can_do_fast64_multiple_times()
            {
                return count_digits(std::numeric_limits<T>::max()) > 8;
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        struct int_classic_value_reader<CharT>::int_reader_state {
            using utype = std::make_unsigned_t<T>;

            // Max for uint
            static constexpr auto uint_max = static_cast<utype>(-1);
            // Max for sint
            static constexpr auto int_max =
                static_cast<utype>(uint_max / 2);  // >> 1
            // Absolute value of min for sint
            static constexpr auto abs_int_min = static_cast<utype>(int_max + 1);

            int_reader_state(int8_t base, sign_type s)
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

        template <typename CharT>
        template <typename T>
        auto int_classic_value_reader<CharT>::do_single_char(
            CharT ch,
            int_reader_state<T>& state) -> std::pair<bool, scan_error>
        {
            using utype = typename int_reader_state<T>::utype;

            const auto digit = static_cast<utype>(_char_to_int(ch));
            if (digit >= state.ubase) {
                return {false, {}};
            }

            if (state.accumulator > state.cutoff() ||
                (state.accumulator == state.cutoff() &&
                 digit > state.cutlim())) {
                if (state.sign == minus_sign) {
                    return {false,
                            {scan_error::value_out_of_range,
                             "Out of range: integer overflow"}};
                }
                return {false, scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer underflow"}};
            }

            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")

            state.accumulator *= state.ubase;
            state.accumulator += digit;

            SCN_GCC_POP

            return {true, {}};
        }

        template <typename CharT>
        template <typename T>
        auto int_classic_value_reader<CharT>::do_single_char_with_thsep(
            int_reader_state<T>& state,
            typename string_view_type::iterator it,
            typename string_view_type::iterator& after_last_thsep_it,
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

            if (state.accumulator > state.cutoff() ||
                (state.accumulator == state.cutoff() &&
                 digit > state.cutlim())) {
                if (state.sign == minus_sign) {
                    return {false,
                            {scan_error::value_out_of_range,
                             "Out of range: integer overflow"}};
                }
                return {false, scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer underflow"}};
            }

            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")

            state.accumulator *= state.ubase;
            state.accumulator += digit;

            SCN_GCC_POP

            return {true, {}};
        }

        namespace {
            template <typename T,
                      typename State,
                      typename Iterator,
                      typename Sentinel>
            scan_error do_read_decimal_fast64(State& state,
                                              Iterator& it,
                                              Sentinel end,
                                              bool& stop_fast)
            {
                SCN_EXPECT(end - it >= 8);
                uint64_t word{};
                std::memcpy(&word, &*it, 8);

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

                    it += digits_in_word;
                }

                {
                    // Bit-twiddle ascii decimal chars to become an actual
                    // integer

                    constexpr uint64_t mask = 0x000000FF000000FFull;
                    constexpr uint64_t mul1 = 100 + (1000000ull << 32);
                    constexpr uint64_t mul2 = 1 + (10000ull << 32);

                    word -= 0x3030303030303030ull;
                    word = (word * 10) + (word >> 8);
                    word = (((word & mask) * mul1) +
                            (((word >> 16) & mask) * mul2)) >>
                           32;
                }

                SCN_ASSUME(word <= 9999'9999ull);

                using utype = typename State::utype;
                const utype accumulator_multiplier =
                    can_do_fast64_multiple_times<T>()
                        ? static_cast<utype>(
                              power_of_10(static_cast<int>(digits_in_word)))
                        : utype{1};

                if (state.accumulator == 0) {
                    if (word > static_cast<uint64_t>(state.limit)) {
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }
                }
                else {
                    SCN_EXPECT(accumulator_multiplier != 0 &&
                               accumulator_multiplier != 1);
                    // accumulator * multiplier + word > limit -> overflow
                    // accumulator * multiplier > limit - word
                    // accumulator > (limit - word) / multiplier
                    if (state.accumulator >
                        ((state.limit - static_cast<utype>(word)) /
                         accumulator_multiplier)) {
                        return scan_error{scan_error::value_out_of_range,
                                          "Out of range: integer overflow"};
                    }
                }

                if (state.accumulator > 0) {
                    SCN_EXPECT(can_do_fast64_multiple_times<T>());

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

                return {};
            }
        }  // namespace

        template <typename CharT>
        template <typename T>
        auto int_classic_value_reader<CharT>::do_read(string_view_type source,
                                                      T& value,
                                                      sign_type sign)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            SCN_EXPECT(!source.empty());
            SCN_EXPECT(std::is_signed_v<T> || sign == plus_sign);
            SCN_EXPECT(m_base > 0);

            int_reader_state<T> state{m_base, sign};
            auto it = source.begin();

            if (_char_to_int(*it) >= m_base) {
                return unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid integer value");
            }

            if ((m_options & allow_thsep) == 0) {
                // No thsep
                bool stop_reading = false;
                if constexpr (std::is_same_v<CharT, char> &&
                              sizeof(void*) == 8) {
                    if (state.ubase == 10) {
                        while (source.end() - it >= 8 && !stop_reading) {
                            if (auto err = do_read_decimal_fast64<T>(
                                    state, it, source.end(), stop_reading);
                                !err) {
                                return unexpected(err);
                            }
                            if constexpr (!can_do_fast64_multiple_times<T>()) {
                                break;
                            }
                        }
                    }
                }

                for (; !stop_reading && it != source.end(); ++it) {
                    if (const auto [keep_going, err] =
                            do_single_char(*it, state);
                        !err) {
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
                            do_single_char_with_thsep(state, it,
                                                      after_last_thsep_it,
                                                      thousands_separators);
                        !err) {
                        return unexpected(err);
                    }
                    else if (!keep_going) {
                        break;
                    }
                }

                if (!thousands_separators.empty()) {
                    if (auto e =
                            check_thousands_separators(thousands_separators);
                        !e) {
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
                    SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned
                    value = static_cast<T>(-static_cast<T>(state.accumulator));
                    SCN_MSVC_POP
                }
            }
            else {
                value = static_cast<T>(state.accumulator);
            }

            return it;
        }

        template <typename CharT>
        template <typename T>
        auto int_classic_value_reader<CharT>::read(string_view_type source,
                                                   T& value)
            -> scan_expected<ranges::iterator_t<string_view_type>>
        {
            SCN_EXPECT(!source.empty());

            auto parse_sign =
                [&](string_view_type& src) -> scan_expected<sign_type> {
                if constexpr (std::is_unsigned_v<T>) {
                    if (auto e = parse_sign_unsigned(src); !e) {
                        return unexpected(e);
                    }
                    return plus_sign;
                }
                else {
                    return parse_sign_signed(src);
                }
            };

            auto sign = parse_sign(source);
            return sign
                .and_then(
                    [&](auto) { return parse_and_determine_base(source); })
                .and_then(
                    [&](auto base_result)
                        -> scan_expected<ranges::iterator_t<string_view_type>> {
                        if (base_result == determine_base_result::zero_parsed) {
                            value = T{0};
                            return {source.begin()};
                        }
                        if (source.empty()) {
                            return unexpected_scan_error(
                                scan_error::invalid_scanned_value,
                                "Invalid integer value");
                        }
                        return do_read(source, value, *sign);
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
