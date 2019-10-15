// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_CONTEXT_H
#define SCN_DETAIL_CONTEXT_H

#include "args.h"
#include "locale.h"
#include "range.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename WrappedRange,
              typename LocaleRef =
                  basic_default_locale_ref<typename WrappedRange::char_type>>
    class basic_context : public LocaleRef {
    public:
        using range_type = WrappedRange;
        using iterator = typename range_type::iterator;
        using sentinel = typename range_type::sentinel;
        using char_type = typename range_type::char_type;
        using locale_type = LocaleRef;
        using args_type = basic_args<basic_context>;
        using arg_type = basic_arg<basic_context>;

        template <typename T>
        using scanner_type = scanner<char_type, T>;
        template <typename... Args>
        using arg_store_type = arg_store<basic_context, Args...>;

        template <typename R>
        basic_context(R&& r) : LocaleRef{}, m_range(std::forward<R>(r))
        {
        }
        template <typename R>
        basic_context(R&& r, LocaleRef&& loc)
            : LocaleRef(std::move(loc)), m_range(std::forward<R>(r))
        {
        }

        SCN_NODISCARD iterator& begin()
        {
            return m_range.begin();
        }
        const sentinel& end() const
        {
            return m_range.end();
        }

        range_type& range()
        {
            return m_range;
        }
        const range_type& range() const
        {
            return m_range;
        }

        LocaleRef& locale() noexcept
        {
            return static_cast<LocaleRef&>(*this);
        }
        const LocaleRef& locale() const noexcept
        {
            return static_cast<const LocaleRef&>(*this);
        }

    private:
        range_type m_range;
    };

    template <typename Context>
    auto get_arg(const basic_args<Context>& args, std::ptrdiff_t id)
        -> expected<basic_arg<Context>>
    {
        auto a = args.get(id);
        if (!a) {
            return error(error::invalid_format_string,
                         "Argument id out of range");
        }
        return a;
    }
    template <typename Context, typename ParseCtx>
    auto get_arg(const basic_args<Context>& args,
                 ParseCtx& pctx,
                 std::ptrdiff_t id) -> expected<basic_arg<Context>>
    {
        return pctx.check_arg_id(id) ? get_arg(args, id)
                                     : error(error::invalid_format_string,
                                             "Argument id out of range");
    }
    template <typename Context, typename ParseCtx>
    auto get_arg(const basic_args<Context>&,
                 ParseCtx&,
                 basic_string_view<typename Context::char_type>)
        -> expected<basic_arg<Context>>
    {
        return error(error::invalid_format_string, "Argument id out of range");
    }

    template <typename Context, typename ParseCtx>
    auto next_arg(const basic_args<Context>& args, ParseCtx& pctx)
        -> expected<basic_arg<Context>>
    {
        return get_arg(args, pctx.next_arg_id());
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_CONTEXT_H
