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
//
//
// The contents of this file are heavily influenced by fast_float:
//     https://github.com/fastfloat/fast_float/blob/main/include/fast_float/ascii_number.h

#include <scn/impl/reader/integer_reader.h>
#include <scn/impl/util/bits.h>

#include <limits>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
namespace {
uint64_t get_eight_digits_word(const char* input)
{
    uint64_t val{};
    std::memcpy(&val, input, sizeof(uint64_t));
    if constexpr (SCN_IS_BIG_ENDIAN) {
        val = byteswap(val);
    }
    return val;
}

constexpr uint32_t parse_eight_decimal_digits_unrolled_fast(uint64_t word)
{
    constexpr uint64_t mask = 0x000000FF000000FF;
    constexpr uint64_t mul1 = 0x000F424000000064;  // 100 + (1000000ULL << 32)
    constexpr uint64_t mul2 = 0x0000271000000001;  // 1 + (10000ULL << 32)
    word -= 0x3030303030303030;
    word = (word * 10) + (word >> 8);  // val = (val * 2561) >> 8;
    word = (((word & mask) * mul1) + (((word >> 16) & mask) * mul2)) >> 32;
    return static_cast<uint32_t>(word);
}

constexpr bool is_word_made_of_eight_decimal_digits_fast(uint64_t word)
{
    return !((((word + 0x4646464646464646) | (word - 0x3030303030303030)) &
              0x8080808080808080));
}

void loop_parse_if_eight_decimal_digits(const char*& p,
                                        const char* const end,
                                        uint64_t& val)
{
    while (
        std::distance(p, end) >= 8 &&
        is_word_made_of_eight_decimal_digits_fast(get_eight_digits_word(p))) {
        val = val * 100'000'000 + parse_eight_decimal_digits_unrolled_fast(
                                      get_eight_digits_word(p));
        p += 8;
    }
}

const char* parse_decimal_integer_fast_impl(const char* begin,
                                            const char* const end,
                                            uint64_t& val)
{
    loop_parse_if_eight_decimal_digits(begin, end, val);

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= 10) {
            break;
        }
        val = 10ull * val + static_cast<uint64_t>(digit);
        ++begin;
    }

    return begin;
}

constexpr size_t maxdigits_u64_table[] = {
    64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16, 16,
    15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13};

constexpr size_t maxdigits_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return maxdigits_u64_table[static_cast<size_t>(base - 2)];
}

static constexpr uint64_t min_safe_u64_table[] = {
    9223372036854775808ull,  12157665459056928801ull, 4611686018427387904,
    7450580596923828125,     4738381338321616896,     3909821048582988049,
    9223372036854775808ull,  12157665459056928801ull, 10000000000000000000ull,
    5559917313492231481,     2218611106740436992,     8650415919381337933,
    2177953337809371136,     6568408355712890625,     1152921504606846976,
    2862423051509815793,     6746640616477458432,     15181127029874798299ull,
    1638400000000000000,     3243919932521508681,     6221821273427820544,
    11592836324538749809ull, 876488338465357824,      1490116119384765625,
    2481152873203736576,     4052555153018976267,     6502111422497947648,
    10260628712958602189ull, 15943230000000000000ull, 787662783788549761,
    1152921504606846976,     1667889514952984961,     2386420683693101056,
    3379220508056640625,     4738381338321616896};

constexpr size_t min_safe_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return min_safe_u64_table[static_cast<size_t>(base - 2)];
}

template <typename T>
constexpr bool check_integer_overflow(uint64_t val,
                                      size_t digits_count,
                                      int base,
                                      bool is_negative)
{
    auto max_digits = maxdigits_u64(base);
    if (digits_count > max_digits) {
        return true;
    }
    if (digits_count == max_digits && val < min_safe_u64(base)) {
        return true;
    }
    if constexpr (!std::is_same_v<T, uint64_t>) {
        if (val > static_cast<uint64_t>(std::numeric_limits<T>::max()) +
                      static_cast<uint64_t>(is_negative)) {
            SCN_UNLIKELY_ATTR
            return true;
        }
    }

    return false;
}

template <typename T>
constexpr T store_result(uint64_t u64val, bool is_negative)
{
    if (is_negative) {
        SCN_MSVC_PUSH
        SCN_MSVC_IGNORE(4146)
        return static_cast<T>(
            -std::numeric_limits<T>::max() -
            static_cast<T>(u64val - std::numeric_limits<T>::max()));
        SCN_MSVC_POP
    }

    return static_cast<T>(u64val);
}

template <typename T>
auto parse_decimal_integer_fast(std::string_view input,
                                T& val,
                                bool is_negative) -> scan_expected<const char*>
{
    uint64_t u64val{};
    auto ptr = parse_decimal_integer_fast_impl(
        input.data(), input.data() + input.size(), u64val);

    auto digits_count = static_cast<size_t>(ptr - input.data());
    if (SCN_UNLIKELY(
            check_integer_overflow<T>(u64val, digits_count, 10, is_negative))) {
        return unexpected_scan_error(scan_error::value_out_of_range,
                                     "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return ptr;
}

template <typename CharT, typename T>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           T& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    uint64_t u64val{};
    const CharT* begin = input.data();
    const CharT* const end = input.data() + input.size();

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= base) {
            break;
        }
        u64val =
            static_cast<uint64_t>(base) * u64val + static_cast<uint64_t>(digit);
        ++begin;
    }

    auto digits_count = static_cast<size_t>(begin - input.data());
    if (SCN_UNLIKELY(check_integer_overflow<T>(u64val, digits_count, base,
                                               is_negative))) {
        return unexpected_scan_error(scan_error::value_out_of_range,
                                     "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return begin;
}
}  // namespace

template <typename CharT, typename T>
auto parse_integer_value(std::basic_string_view<CharT> source,
                         T& value,
                         sign_type sign,
                         int base)
    -> scan_expected<typename std::basic_string_view<CharT>::iterator>
{
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(std::is_signed_v<T> || sign == sign_type::plus_sign);
    SCN_EXPECT(sign != sign_type::default_sign);
    SCN_EXPECT(base > 0);

    if (char_to_int(source[0]) >= base) {
        SCN_UNLIKELY_ATTR
        return unexpected_scan_error(scan_error::invalid_scanned_value,
                                     "Invalid integer value");
    }

    // Skip leading zeroes
    auto start = source.data();
    const auto end = source.data() + source.size();
    {
        for (; start != end; ++start) {
            if (*start != CharT{'0'}) {
                break;
            }
        }
        if (SCN_UNLIKELY(start == end || char_to_int(*start) >= base)) {
            value = 0;
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), start));
        }
    }

    if constexpr (std::is_same_v<CharT, char>) {
        if (base == 10) {
            SCN_TRY(ptr, parse_decimal_integer_fast(
                             detail::make_string_view_from_pointers(start, end),
                             value, sign == sign_type::minus_sign));
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), ptr));
        }
    }

    SCN_TRY(ptr, parse_regular_integer(
                     detail::make_string_view_from_pointers(start, end), value,
                     base, sign == sign_type::minus_sign));
    return ranges::next(source.begin(), ranges::distance(source.data(), ptr));
}

template <typename T>
void parse_integer_value_exhaustive_valid(std::string_view source, T& value)
{
    SCN_EXPECT(!source.empty());

    bool negative_sign = false;
    if constexpr (std::is_signed_v<T>) {
        if (source.front() == '-') {
            source = source.substr(1);
            negative_sign = true;
        }
    }
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(char_to_int(source.front()) < 10);

    const char* p = source.data();
    const char* const end = source.data() + source.size();

    uint64_t u64val{};
    while (std::distance(p, end) >= 8) {
        SCN_EXPECT(is_word_made_of_eight_decimal_digits_fast(
            get_eight_digits_word(p)));
        u64val =
            u64val * 100'000'000 +
            parse_eight_decimal_digits_unrolled_fast(get_eight_digits_word(p));
        p += 8;
    }

    while (p != end) {
        const auto digit = char_to_int(*p);
        SCN_EXPECT(digit < 10);
        u64val = 10ull * u64val + static_cast<uint64_t>(digit);
        ++p;
    }
    SCN_EXPECT(p == end);

    {
        auto digits_count = static_cast<size_t>(p - source.data());
        SCN_UNUSED(digits_count);
        SCN_EXPECT(check_integer_overflow<T>(u64val, digits_count, 10,
                                             negative_sign) == false);
    }

    value = store_result<T>(u64val, negative_sign);
}

#define SCN_DEFINE_INTEGER_READER_TEMPLATE(CharT, IntT)                      \
    template auto parse_integer_value(std::basic_string_view<CharT> source,  \
                                      IntT& value, sign_type sign, int base) \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   signed char&);
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, short)
template void parse_integer_value_exhaustive_valid(std::string_view, short&);
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int)
template void parse_integer_value_exhaustive_valid(std::string_view, int&);
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long)
template void parse_integer_value_exhaustive_valid(std::string_view, long&);
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   long long&);
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned char&);
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned short&);
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned int&);
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long&);
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long long&);
#endif

#undef SCN_DEFINE_INTEGER_READER_TEMPLATE
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
