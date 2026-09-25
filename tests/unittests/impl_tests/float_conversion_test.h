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

#include "../float_test_suite.h"
#include "reader_test_common.h"

#include <scn/impl.h>

template <typename Traits, bool Enabled>
struct supports_hex_helper;

template <typename Traits>
struct supports_hex_helper<Traits, true> {
    static constexpr bool value = Traits::support_hexfloat;
};

template <typename Traits>
struct supports_hex_helper<Traits, false> {
    static constexpr bool value = false;
};

template <bool Localized, typename Traits, typename CharT, typename FloatT>
struct float_conversion_interface_base {
    using char_type = CharT;
    using float_type = FloatT;

    static constexpr bool enabled = Traits::template enabled<CharT, FloatT>;
    static constexpr bool supports_nan = enabled;
    static constexpr bool supports_inf = enabled;
    static constexpr bool supports_hex =
        supports_hex_helper<Traits, enabled>::value;

    static scn::scan_expected<void> test(std::basic_string_view<CharT> source,
                                         FloatT& parsed)
    {
        if constexpr (enabled) {
            scn::detail::format_specs specs{};
            specs.localized = Localized;
            auto convert =
                scn::impl::float_conversion::basic_convert_float<CharT,
                                                                 Traits>{};
            if (auto res = convert.convert_specs(source, parsed, specs, {});
                res) {
                if (*res != source.end()) {
                    return scn::detail::unexpected_scan_error(
                        scn::scan_error::length_too_short,
                        "Entire input was not exhausted");
                }
                return {};
            }
            else {
                if (convert.can_fall_back()) {
                    GTEST_MESSAGE_("", testing::TestPartResult::kSkip)
                        << "Test case failed, but it would fall back.";
                    return {};
                }
                return scn::unexpected(res.error());
            }
        }
        else {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }
};

template <template <bool, typename, typename> class Interface>
struct make_classic_float_conversion_interface {
    template <typename CharT, typename FloatT>
    using type = Interface<false, CharT, FloatT>;
};

template <template <bool, typename, typename> class Interface>
struct make_localized_float_conversion_interface {
    template <typename CharT, typename FloatT>
    using type = Interface<true, CharT, FloatT>;
};
