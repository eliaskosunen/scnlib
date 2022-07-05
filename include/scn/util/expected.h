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

#pragma once

#include <scn/detail/error.h>
#include <scn/util/expected_impl.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    static_assert(std::is_trivially_destructible_v<scan_error>);
    static_assert(std::is_trivially_copy_constructible_v<scan_error>);
    static_assert(std::is_trivially_copy_assignable_v<scan_error>);
    static_assert(std::is_trivially_move_constructible_v<scan_error>);
    static_assert(std::is_trivially_move_assignable_v<scan_error>);
    static_assert(std::is_trivially_copyable_v<scan_error>);

    template <typename T>
    using scan_expected = expected<T, scan_error>;

    template <typename... Args>
    auto unexpected_scan_error(Args&&... args)
    {
        return unexpected(scan_error{SCN_FWD(args)...});
    }

    namespace detail {
        template <typename T>
        class always_success_expected
            : public expected<T, always_success_error> {
        public:
            using expected<T, always_success_error>::expected;

            constexpr bool has_value() const SCN_NOEXCEPT
            {
                return true;
            }
            constexpr explicit operator bool() const SCN_NOEXCEPT
            {
                return true;
            }

            [[noreturn]] always_success_error error() const
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        };

        template <typename T>
        always_success_expected(T) -> always_success_expected<T>;
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
