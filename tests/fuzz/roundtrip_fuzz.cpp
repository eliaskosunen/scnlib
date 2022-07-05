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

namespace scn::fuzz {
    namespace {
        template <typename T, typename Result>
        void check_roundtrip(const T& value,
                             const T& original,
                             const Result& result)
        {
            if (!result) {
                throw std::runtime_error("Failed to read");
            }
            if (value != original) {
                throw std::runtime_error("Roundtrip failure");
            }
            if (!result.range().empty()) {
                throw std::runtime_error("Unparsed input");
            }
        }

        template <typename CharT, typename T, typename Source>
        void do_roundtrip(const T& original_value, const Source& source)
        {
            {
                const auto [result, value] =
                    scn::scan<T>(source, get_default_format_string<CharT>());
                check_roundtrip(value, original_value, result);
            }
            {
                const auto [result, value] = scn::scan_value<T>(source);
                check_roundtrip(value, original_value, result);
            }
            {
                const auto [result, value] = scn::scan<T>(
                    global_locale, source, get_default_format_string<CharT>());
                check_roundtrip(value, original_value, result);
            }
        }

        template <typename T, typename Source>
        T bitcast_for_roundtrip(Source data)
        {
#if 1
            std::array<unsigned char, 16> tmp_buffer{};
            {
                const auto n = std::min(
                    data.size() * sizeof(ranges::range_value_t<Source>),
                    size_t{16});
                std::memcpy(tmp_buffer.data(), data.data(), n);
            }

            T value{};
            std::memcpy(&value, tmp_buffer.data(), sizeof(T));
            return value;
#else
            std::array<unsigned char, 16> buffer{};
            const auto n = std::min(data.size(), size_t{16});
            std::copy_n(data.begin(), n, buffer.begin());

            T value{};
            std::memcpy(&value, buffer.data(), sizeof(T));
            return value;
#endif
        }

        template <typename T>
        using widen_to_64 = std::
            conditional_t<std::is_signed_v<T>, long long, unsigned long long>;

        template <typename CharT, typename T, typename Source>
        void roundtrip_for_type(Source data)
        {
            SCN_EXPECT(data.size() >= sizeof(T));

            const auto original_value = bitcast_for_roundtrip<T>(data);
            std::basic_ostringstream<CharT> oss;
            oss << static_cast<widen_to_64<T>>(original_value);

            const auto source_str = SCN_MOVE(oss).str();
            do_roundtrip<CharT>(original_value, source_str);

            auto source_iss = populate_iss(source_str);
            do_roundtrip<CharT>(original_value, source_iss);

            auto source_erased = populate_erased(source_str);
            do_roundtrip<CharT>(original_value, source_erased);
        }

        template <typename Source>
        void roundtrip_for_source(Source source)
        {
            using char_type = ranges::range_value_t<Source>;

            roundtrip_for_type<char_type, signed char>(source);
            roundtrip_for_type<char_type, short>(source);
            roundtrip_for_type<char_type, int>(source);
            roundtrip_for_type<char_type, long>(source);
            roundtrip_for_type<char_type, long long>(source);
            roundtrip_for_type<char_type, unsigned char>(source);
            roundtrip_for_type<char_type, unsigned short>(source);
            roundtrip_for_type<char_type, unsigned>(source);
            roundtrip_for_type<char_type, unsigned long>(source);
            roundtrip_for_type<char_type, unsigned long long>(source);
        }

        void run(span<const uint8_t> data)
        {
            if (data.size() < sizeof(long long)) {
                return;
            }
            data = data.first(sizeof(long long));

            auto [sv, wsv_direct, _1, _2] = make_input_views(data);
            roundtrip_for_source(sv);
            roundtrip_for_source(wsv_direct);
        }
    }  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::fuzz::run({data, size});
    return 0;
}
