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

#include <scn/vscan_impl.h>

#if !SCN_DISABLE_IOSTREAM
#include <scn/detail/istream_range.h>

#include <iostream>
#include <mutex>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wexit-time-destructors")
        scn::istreambuf_view& internal_narrow_stdin()
        {
            static scn::istreambuf_view range{std::cin};
            return range;
        }
        scn::wistreambuf_view& internal_wide_stdin()
        {
            static scn::wistreambuf_view range{std::wcin};
            return range;
        }
        SCN_CLANG_POP

        namespace {
            std::mutex stdin_mutex;

            bool is_global_stdin_view(istreambuf_view& view)
            {
                return &view == &detail::internal_narrow_stdin();
            }
            bool is_global_stdin_view(wistreambuf_view& view)
            {
                return &view == &detail::internal_wide_stdin();
            }

            template <typename CharT>
            detail::vscan_impl_result<basic_istreambuf_subrange<CharT>>
            vscan_and_sync_internal(
                basic_istreambuf_subrange<CharT> source,
                std::basic_string_view<CharT> format,
                scan_args_for<basic_istreambuf_subrange<CharT>, CharT> args)
            {
                std::unique_lock<std::mutex> stdin_lock{stdin_mutex,
                                                        std::defer_lock};
                auto& view = static_cast<basic_istreambuf_view<CharT>&>(
                    source.begin().view());
                if (is_global_stdin_view(view)) {
                    stdin_lock.lock();
                }

                auto result = vscan_internal(source, format, args);
                if (SCN_LIKELY(result)) {
                    view.sync(*result);
                }
                return result;
            }
        }  // namespace

        SCN_DEFINE_VSCAN(istreambuf_subrange, char)
        SCN_DEFINE_VSCAN(wistreambuf_subrange, wchar_t)

        vscan_impl_result<istreambuf_subrange> vscan_and_sync_impl(
            istreambuf_subrange source,
            std::string_view format,
            scan_args_for<istreambuf_subrange, char> args)
        {
            return vscan_and_sync_internal(source, format, args);
        }

        vscan_impl_result<wistreambuf_subrange> vscan_and_sync_impl(
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args)
        {
            return vscan_and_sync_internal(source, format, args);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
#endif
