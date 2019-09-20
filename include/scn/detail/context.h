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
        basic_context(R&& r, args_type args)
            : LocaleRef{}, m_range(std::forward<R>(r)), m_args(std::move(args))
        {
        }
        template <typename R>
        basic_context(R&& r, args_type args, LocaleRef&& loc)
            : LocaleRef(std::move(loc)),
              m_range(std::forward<R>(r)),
              m_args(std::move(args))
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

        template <typename ParseCtx>
        expected<arg_type> next_arg(ParseCtx& pctx)
        {
            return do_get_arg(pctx.next_arg_id());
        }
        template <typename ParseCtx>
        expected<arg_type> arg(ParseCtx& pctx, std::ptrdiff_t id)
        {
            return pctx.check_arg_id(id) ? do_get_arg(id) : arg_type{};
        }
        template <typename ParseCtx>
        expected<arg_type> arg(ParseCtx&, basic_string_view<char_type>)
        {
            return arg_type{};
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
        expected<arg_type> do_get_arg(std::ptrdiff_t id)
        {
            auto a = m_args.get(id);
            if (!a && !m_args.check_id(id - 1)) {
                return error(error::invalid_argument,
                             "Argument id out of range");
            }
            return a;
        }

        range_type m_range;
        args_type m_args;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_CONTEXT_H
