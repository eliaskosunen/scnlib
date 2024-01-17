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

#include <scn/detail/scanner.h>
#include <scn/impl/algorithms/read.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
struct reader_error_handler {
    constexpr void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        m_msg = msg;
    }
    explicit constexpr operator bool() const
    {
        return m_msg == nullptr;
    }

    const char* m_msg{nullptr};
};

template <typename SourceRange>
eof_expected<simple_borrowed_iterator_t<SourceRange>> skip_classic_whitespace(
    SourceRange&& range,
    bool allow_exhaustion = false)
{
    if (!allow_exhaustion) {
        auto it = read_while_classic_space(range);
        if (auto e = eof_check(ranges::subrange{it, ranges::end(range)});
            SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        return it;
    }

    return read_while_classic_space(SCN_FWD(range));
}

template <typename SourceCharT, typename DestCharT>
scan_error transcode_impl(std::basic_string_view<SourceCharT> src,
                          std::basic_string<DestCharT>& dst)
{
    dst.clear();
    transcode_valid_to_string(src, dst);
    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_error transcode_if_necessary(
    const contiguous_range_factory<SourceCharT>& source,
    std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        dest.assign(source.view());
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_error transcode_if_necessary(
    contiguous_range_factory<SourceCharT>&& source,
    std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        if (source.stores_allocated_string()) {
            dest.assign(SCN_MOVE(source.get_allocated_string()));
        }
        else {
            dest.assign(source.view());
        }
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_error transcode_if_necessary(string_view_wrapper<SourceCharT> source,
                                  std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        dest.assign(source.view());
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

template <typename Derived, typename CharT>
class reader_base {
public:
    using char_type = CharT;

    constexpr reader_base() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    scan_error check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        get_derived().check_specs_impl(specs, eh);
        if (SCN_UNLIKELY(!eh)) {
            return {scan_error::invalid_format_string, eh.m_msg};
        }
        return {};
    }

private:
    Derived& get_derived()
    {
        return static_cast<Derived&>(*this);
    }
    const Derived& get_derived() const
    {
        return static_cast<const Derived&>(*this);
    }
};

template <typename CharT>
class reader_impl_for_monostate {
public:
    constexpr reader_impl_for_monostate() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    static scan_error check_specs(const detail::format_specs&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>>
    read_default(Range&&, monostate&, detail::locale_ref)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Range>
    scan_expected<simple_borrowed_iterator_t<Range>> read_specs(
        Range&&,
        const detail::format_specs&,
        monostate&,
        detail::locale_ref)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
