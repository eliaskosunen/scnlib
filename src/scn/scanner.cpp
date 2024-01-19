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

#include <scn/detail/xchar.h>
#include <scn/impl/reader/common.h>
#include <scn/impl/reader/reader.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename T, typename Context>
scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs)
{
    if constexpr (!detail::is_type_disabled<T>) {
        return impl::arg_reader<Context>{ctx.range(), specs, {}}(val);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename Range>
scan_expected<ranges::iterator_t<Range>> internal_skip_classic_whitespace(
    Range r,
    bool allow_exhaustion)
{
    return impl::skip_classic_whitespace(r, allow_exhaustion)
        .transform_error(impl::make_eof_scan_error);
}

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context)                         \
    template scan_expected<Context::iterator> scanner_scan_for_builtin_type( \
        T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(signed char, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(short, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(int, Context)                  \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long, Context)                 \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long long, Context)            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)       \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(float, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(double, Context)               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long double, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string_view, Context)     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring_view, Context)    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)       \
    template scan_expected<ranges::iterator_t<Context::range_type>> \
    internal_skip_classic_whitespace(Context::range_type, bool);

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
