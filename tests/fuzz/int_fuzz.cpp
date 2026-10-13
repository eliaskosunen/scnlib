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

#include "fuzz.h"

#include <charconv>
#include <cstdio>
#include <cstdlib>

namespace scn::fuzz {
template <typename CharT, typename Source>
void do_basic_run_for_source(Source&& source,
                             std::basic_string_view<CharT> format_string)
{
    do_basic_run_for_type<CharT, signed char>(source, format_string);
    do_basic_run_for_type<CharT, unsigned char>(source, format_string);
    do_basic_run_for_type<CharT, int>(source, format_string);
    do_basic_run_for_type<CharT, unsigned>(source, format_string);
    do_basic_run_for_type<CharT, long long>(source, format_string);
    do_basic_run_for_type<CharT, unsigned long long>(source, format_string);

#if SCN_HAS_INT128
    do_basic_run_for_type<CharT, scn::int128>(source, format_string);
    do_basic_run_for_type<CharT, scn::uint128>(source, format_string);
#endif
}

namespace {
[[noreturn]] void fail(std::string_view token, int base, const char* what)
{
    std::fprintf(stderr, "%s: token \"%.*s\", base %d\n", what,
                 static_cast<int>(token.size()), token.data(), base);
    std::abort();
}

// Checks scnlib against std::from_chars, on tokens that both must handle the
// same way: an optional '-' (signed types only), followed by digits valid in
// the base. scnlib must give the same value, consume the whole token, and
// report overflow the same way.
template <typename T>
void check_against_from_chars(std::string_view token, int base)
{
    T expected{};
    const auto expected_result = std::from_chars(
        token.data(), token.data() + token.size(), expected, base);
    SCN_ENSURE(expected_result.ptr == token.data() + token.size() ||
               expected_result.ec == std::errc::result_out_of_range);

    const auto format = base == 2    ? std::string{"{:b}"}
                        : base == 8  ? std::string{"{:o}"}
                        : base == 10 ? std::string{"{:d}"}
                        : base == 16 ? std::string{"{:x}"}
                                     : "{:r" + std::to_string(base) + "}";
    const auto result = scn::scan<T>(token, scn::runtime_format(format));

    if (expected_result.ec == std::errc::result_out_of_range) {
        const auto expected_code =
            token.front() == '-' ? scn::scan_error::value_negative_overflow
                                 : scn::scan_error::value_positive_overflow;
        if (result || result.error().code() != expected_code) {
            fail(token, base, "Overflow not reported like std::from_chars");
        }
    }
    else if (!result || result->value() != expected ||
             !result->range().empty()) {
        fail(token, base,
             "Value or consumed length differs from std::from_chars");
    }
}

void run_from_chars_comparison(const uint8_t* data, size_t size)
{
    constexpr size_t max_digits = 80;
    if (size < 3) {
        return;
    }
    const auto base = 2 + static_cast<int>(data[0] % 35);
    const auto type_index = data[1] % 4;
    const auto negative = (data[1] & 0x80) != 0;

    constexpr std::string_view digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string token{};
    const bool is_signed = type_index == 0 || type_index == 2;
    if (negative && is_signed) {
        token.push_back('-');
    }
    for (size_t i = 2; i < size && i < max_digits + 2; ++i) {
        token.push_back(digits[data[i] % static_cast<size_t>(base)]);
    }

    switch (type_index) {
        case 0:
            return check_against_from_chars<int64_t>(token, base);
        case 1:
            return check_against_from_chars<uint64_t>(token, base);
        case 2:
            return check_against_from_chars<int32_t>(token, base);
        default:
            return check_against_from_chars<uint32_t>(token, base);
    }
}
}  // namespace

namespace {
void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    run_from_chars_comparison(data, size);

    const auto inputs = make_input_views(data, size);

    const auto f =
        format_strings_type<char>{"{}",   "{:i}", "{:d}",   "{:L}", "{:x}",
                                  "{:b}", "{:o}", "{:B36}", "{:'}", "{:'L}"};
    do_basic_run(inputs.narrow, f);

    const auto wf = format_strings_type<wchar_t>{
        L"{}",   L"{:i}", L"{:d}",   L"{:L}", L"{:x}",
        L"{:b}", L"{:o}", L"{:B36}", L"{:'}", L"{:'L}"};
    do_basic_run(inputs.wide_copied, wf);
    do_basic_run(inputs.wide_reinterpreted, wf);
    if (!inputs.wide_transcoded.empty()) {
        do_basic_run(inputs.wide_transcoded, wf);
    }
}
}  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::fuzz::run(data, size);
    return 0;
}
