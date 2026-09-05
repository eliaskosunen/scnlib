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

#include <scn/impl.h>
#include <iomanip>

namespace {

template <typename T>
void run_single(
    T expected,
    std::function<std::string(scn::detail::type_identity_t<T>)> to_string,
    std::function<std::optional<scn::detail::type_identity_t<T>>(
        std::string_view)> from_string)
{
    auto str = to_string(expected);
    if (auto parsed = from_string(str)) {
        if (std::isnan(expected)) {
            if (!std::isnan(*parsed)) {
                throw std::runtime_error("NaN-ness of value not preserved");
            }
        }
        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
        else if (*parsed != expected) {
            throw std::runtime_error("Non-roundtrip parse with input: " + str);
        }
        SCN_GCC_COMPAT_POP
    }
    else {
        throw std::runtime_error("Failed to parse input: " + str);
    }
}

template <typename T>
void run(const uint8_t* data, size_t size)
{
    SCN_EXPECT(size == sizeof(T));

    if constexpr (std::numeric_limits<T>::digits == 64) {
        // x87 long double, check for invalid bit patterns
        struct x87_long_double {
            uint64_t significand{};
            uint16_t exponent_and_sign{};

            uint16_t exponent() const
            {
                return exponent_and_sign & 0x7ffu;
            }
            bool sign() const
            {
                return (exponent_and_sign & 0x8000u) != 0;
            }
            bool integer_bit() const
            {
                return (significand & (1ull << 63)) != 0;
            }
            uint64_t fraction() const
            {
                return significand & 0x7fffffffffffffffull;
            }
        };
        x87_long_double repr{};
        std::memcpy(&repr, data, sizeof(x87_long_double));

        if (repr.exponent() == 0x7fffu && !repr.integer_bit()) {
            // Pseudo-NaN
            return;
        }
        if (repr.exponent() == 0u && repr.integer_bit()) {
            // Pseudo-denormal
            return;
        }
        if (!repr.integer_bit()) {
            // Unnormal
            return;
        }
    }

    T expected_float{};
    std::memcpy(&expected_float, data, sizeof(T));

    auto to_string_with_ostream = [](auto f) {
        return [f](T value) -> std::string {
            std::ostringstream strm;
            f(strm);
            strm << value;
            return strm.str();
        };
    };

    auto from_string_with_scan = [](std::string_view str) -> std::optional<T> {
        if (auto res = scn::scan<T>(str, "{}")) {
            return res->value();
        }
        return std::nullopt;
    };
    auto from_string_with_custom_convert =
        [expected_float](std::string_view str) -> std::optional<T> {
        T parsed{};
        auto convert = scn::impl::float_conversion::basic_convert_float<
            char, scn::impl::float_conversion::convert_custom_traits>{};
        if (auto res = convert.convert_default(str, parsed)) {
            return parsed;
        }
        if (convert.can_fall_back()) {
            return expected_float;
        }
        return std::nullopt;
    };

    auto to_string_regular = to_string_with_ostream([](std::ostream& s) {
        s.precision(std::numeric_limits<T>::max_digits10 + 1);
    });
    run_single(expected_float, to_string_regular, from_string_with_scan);
    run_single(expected_float, to_string_regular,
               from_string_with_custom_convert);

    auto to_string_hex = to_string_with_ostream([](std::ostream& s) {
        s << std::hexfloat;
        s.precision(std::numeric_limits<T>::digits / 4 + 1);
    });
    run_single(expected_float, to_string_hex, from_string_with_scan);
    run_single(expected_float, to_string_hex, from_string_with_custom_convert);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == sizeof(float)) {
        run<float>(data, size);
    }
    if (size == sizeof(double)) {
        run<double>(data, size);
    }
    if (size == sizeof(long double)) {
        run<long double>(data, size);
    }

    return 0;
}
