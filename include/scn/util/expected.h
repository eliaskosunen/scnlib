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

/**
 * An `expected<T, scan_error>`.
 *
 * Not a type alias to shorten template names
 *
 * \ingroup result
 */
template <typename T>
struct scan_expected : public expected<T, scan_error> {
    using expected<T, scan_error>::expected;

    scan_expected(const expected<T, scan_error>& other)
        : expected<T, scan_error>(other)
    {
    }
    scan_expected(expected<T, scan_error>&& other)
        : expected<T, scan_error>(SCN_MOVE(other))
    {
    }
};

template <typename... Args>
auto unexpected_scan_error(Args&&... args)
{
    return unexpected(scan_error{SCN_FWD(args)...});
}

namespace detail {
template <typename T>
struct is_expected_impl<scan_expected<T>> : std::true_type {};
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

#define SCN_TRY_IMPL_CONCAT(a, b)  a##b
#define SCN_TRY_IMPL_CONCAT2(a, b) SCN_TRY_IMPL_CONCAT(a, b)
#define SCN_TRY_TMP                SCN_TRY_IMPL_CONCAT2(_scn_try_tmp_, __LINE__)

#define SCN_TRY_ASSIGN(init, x)                        \
    auto&& SCN_TRY_TMP = (x);                          \
    if (SCN_UNLIKELY(!SCN_TRY_TMP)) {                  \
        return ::scn::unexpected(SCN_TRY_TMP.error()); \
    }                                                  \
    init = *SCN_FWD(SCN_TRY_TMP);
#define SCN_TRY(name, x) SCN_TRY_ASSIGN(auto name, x)

#define SCN_TRY_ERR(name, x)          \
    auto&& SCN_TRY_TMP = (x);         \
    if (SCN_UNLIKELY(!SCN_TRY_TMP)) { \
        return SCN_TRY_TMP.error();   \
    }                                 \
    auto name = *SCN_FWD(SCN_TRY_TMP);
