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

#include <scn/detail/error.h>
#include <scn/detail/format_string_parser.h>
#include <scn/detail/istream_range.h>
#include <scn/detail/result.h>
#include <scn/detail/visitor.h>
#include <scn/detail/vscan.h>
#include <scn/impl/reader/reader.h>

#include <istream>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace {
        template <typename CharT>
        constexpr bool is_simple_single_argument_format_string(
            std::basic_string_view<CharT> format)
        {
            if (format.size() != 2) {
                return false;
            }
            return format[0] == CharT{'{'} && format[1] == CharT{'}'};
        }

        template <typename SourceRange, typename CharT>
        vscan_result<SourceRange> scan_simple_single_argument(
            SourceRange source,
            basic_scan_args<basic_scan_context<SourceRange, CharT>> args,
            basic_scan_arg<basic_scan_context<SourceRange, CharT>> arg,
            detail::locale_ref loc = {})
        {
            if (!arg) {
                return {SCN_MOVE(source),
                        {scan_error::invalid_format_string,
                         "Argument #0 not found"}};
            }

            auto reader = impl::default_arg_reader<
                basic_scan_context<SourceRange, CharT>>{SCN_MOVE(source),
                                                        SCN_MOVE(args), loc};

            if (auto result = visit_scan_arg(SCN_MOVE(reader), arg); result) {
                return {{*result, reader.range.end()}, {}};
            }
            else {
                return {reader.range, result.error()};
            }
        }

        template <typename Context, typename ID, typename Handler>
        auto get_arg(Context& ctx, ID id, Handler& handler) ->
            typename Context::arg_type
        {
            auto arg = ctx.arg(id);
            if (!arg) {
                handler.on_error("Failed to find argument with ID");
            }
            return arg;
        }

        struct auto_id {};

        template <typename Context, typename CharT>
        class specs_handler : public detail::specs_setter<CharT> {
        public:
            using arg_type = typename Context::arg_type;

            constexpr specs_handler(detail::basic_format_specs<CharT>& specs,
                                    basic_scan_parse_context<CharT>& parse_ctx,
                                    Context& ctx)
                : detail::specs_setter<CharT>(specs),
                  m_parse_ctx(parse_ctx),
                  m_ctx(ctx)
            {
            }

        private:
            constexpr arg_type get_arg(auto_id)
            {
                return get_arg(m_ctx, m_parse_ctx.next_arg_id(), *this);
            }

            constexpr arg_type get_arg(std::size_t arg_id)
            {
                m_parse_ctx.check_arg_id(arg_id);
                return get_arg(m_ctx, arg_id, *this);
            }

            basic_scan_parse_context<CharT>& m_parse_ctx;
            Context& m_ctx;
        };

        template <typename SourceRange, typename CharT>
        struct format_handler {
            using char_type = CharT;
            using range_type = SourceRange;
            using context_type = basic_scan_context<range_type, char_type>;
            using parse_context_type = basic_scan_parse_context<char_type>;
            using format_type = std::basic_string_view<char_type>;
            using args_type = basic_scan_args<context_type>;

            format_handler(range_type source,
                           format_type format,
                           args_type args,
                           detail::locale_ref loc,
                           std::size_t argcount)
                : parse_ctx{format},
                  ctx{SCN_MOVE(source), SCN_MOVE(args), SCN_MOVE(loc)},
                  args_count{argcount}
            {
            }

            void on_literal_text(const CharT* begin, const CharT* end)
            {
                for (; begin != end; ++begin) {
                    auto it = ranges::begin(ctx.range());
                    if (it == ranges::end(ctx.range())) {
                        return on_error("Unexpected end of source");
                    }
                    auto next = *it;
                    if (next != *begin) {
                        return on_error("Unexpected literal char in source");
                    }
                    ctx.advance_to(ranges::next(it));
                }
            }

            constexpr std::size_t on_arg_id()
            {
                return parse_ctx.next_arg_id();
            }
            constexpr std::size_t on_arg_id(std::size_t id)
            {
                parse_ctx.check_arg_id(id);
                return id;
            }

            template <typename Visitor>
            void on_visit_scan_arg(Visitor&& visitor,
                                   typename context_type::arg_type arg)
            {
                if (!*this || !arg) {
                    return;
                }

                visit_scan_arg(SCN_FWD(visitor), arg)
                    .transform([&](auto it) SCN_NOEXCEPT {
                        // force formatting
                        ctx.advance_to(it);
                    })
                    .transform_error([&](auto err)
                                         SCN_NOEXCEPT { on_error(err); });
            }

            void on_replacement_field(std::size_t arg_id, const CharT*)
            {
                auto arg = get_arg(ctx, arg_id, *this);
                on_visit_scan_arg(
                    impl::default_arg_reader<context_type>{
                        ctx.range(), ctx.args(), ctx.locale()},
                    arg);
            }

            const CharT* on_format_specs(std::size_t arg_id,
                                         const CharT* begin,
                                         const CharT* end)
            {
                auto arg = get_arg(ctx, arg_id, *this);

                if (arg.type() == detail::arg_type::custom_type) {
                    parse_ctx.advance_to(begin);
                    on_visit_scan_arg(
                        impl::custom_reader<context_type>{parse_ctx, ctx}, arg);
                    return parse_ctx.begin();
                }

                auto specs = detail::basic_format_specs<CharT>{};
                detail::specs_checker<specs_handler<context_type, CharT>>
                    handler{specs_handler<context_type, CharT>{specs, parse_ctx,
                                                               ctx},
                            arg.type()};

                begin = detail::parse_format_specs(begin, end, handler);
                if (begin == end || *begin != CharT{'}'}) {
                    on_error("Missing '}' in format string");
                    return parse_ctx.begin();
                }
                parse_ctx.advance_to(begin);

                on_visit_scan_arg(
                    impl::arg_reader<context_type>{ctx.range(), specs,
                                                   ctx.locale()},
                    arg);
                return parse_ctx.begin();
            }

            void check_args_exhausted()
            {
                on_error(parse_ctx.check_args_exhausted(ctx.args().size()));
            }

            void on_error(const char* msg)
            {
                error = scan_error{scan_error::invalid_format_string, msg};
            }
            void on_error(scan_error err)
            {
                if (err != scan_error::good) {
                    error = err;
                }
            }

            explicit constexpr operator bool() const
            {
                return static_cast<bool>(error);
            }
            SCN_NODISCARD scan_error get_error() const
            {
                return error;
            }

            parse_context_type parse_ctx;
            context_type ctx;
            scan_error error;
            std::size_t args_count;
        };

        template <typename SourceRange, typename CharT>
        vscan_result<SourceRange> vscan_impl(
            SourceRange source,
            std::basic_string_view<CharT> format,
            basic_scan_args<basic_scan_context<SourceRange, CharT>> args,
            detail::locale_ref loc = {})
        {
            const auto argcount = args.size();
            if (is_simple_single_argument_format_string(format) &&
                argcount == 1) {
                auto arg = args.get(0);
                return scan_simple_single_argument(SCN_MOVE(source),
                                                   SCN_MOVE(args), arg);
            }

            auto handler = format_handler<SourceRange, CharT>{
                SCN_MOVE(source), format, SCN_MOVE(args), SCN_MOVE(loc),
                argcount};
            detail::parse_format_string<false>(format, handler);
            return {handler.ctx.range(), handler.error};
        }

        template <typename SourceRange, typename CharT>
        vscan_result<SourceRange> vscan_value_impl(
            SourceRange source,
            basic_scan_arg<basic_scan_context<SourceRange, CharT>> arg)
        {
            return scan_simple_single_argument(SCN_MOVE(source), {}, arg);
        }

        template <typename CharT>
        vscan_result<basic_istreambuf_subrange<CharT>> vscan_and_sync_impl(
            basic_istreambuf_subrange<CharT> source,
            std::basic_string_view<CharT> format,
            scan_args_for<basic_istreambuf_subrange<CharT>, CharT> args)
        {
            auto result = vscan_impl(source, format, args);
            source.sync(result.range.begin());
            return result;
        }
    }  // namespace

#define SCN_DEFINE_VSCAN(Range, CharT)                                         \
    vscan_result<Range> vscan(Range source,                                    \
                              std::basic_string_view<CharT> format,            \
                              scan_args_for<Range, CharT> args)                \
    {                                                                          \
        return vscan_impl(SCN_MOVE(source), format, args);                     \
    }                                                                          \
                                                                               \
    template <typename Locale, typename>                                       \
    vscan_result<Range> vscan(Locale& loc, Range source,                       \
                              std::basic_string_view<CharT> format,            \
                              scan_args_for<Range, CharT> args)                \
    {                                                                          \
        return vscan_impl(SCN_MOVE(source), format, args,                      \
                          detail::locale_ref{loc});                            \
    }                                                                          \
    template vscan_result<Range> vscan<std::locale, void>(                     \
        std::locale & loc, Range source, std::basic_string_view<CharT> format, \
        scan_args_for<Range, CharT> args);                                     \
                                                                               \
    vscan_result<Range> vscan_value(Range source,                              \
                                    scan_arg_for<Range, CharT> arg)            \
    {                                                                          \
        return vscan_value_impl(SCN_MOVE(source), arg);                        \
    }

    SCN_DEFINE_VSCAN(std::string_view, char)
    SCN_DEFINE_VSCAN(std::wstring_view, wchar_t)

#if SCN_USE_IOSTREAMS
    SCN_DEFINE_VSCAN(istreambuf_subrange, char)
    SCN_DEFINE_VSCAN(wistreambuf_subrange, wchar_t)
#endif

    SCN_DEFINE_VSCAN(erased_subrange, char)
    SCN_DEFINE_VSCAN(werased_subrange, wchar_t)

#undef SCN_DEFINE_VSCAN

#if SCN_USE_IOSTREAMS
    vscan_result<istreambuf_subrange> vscan_and_sync(
        istreambuf_subrange source,
        std::string_view format,
        scan_args_for<istreambuf_subrange, char> args)
    {
        return vscan_and_sync_impl(source, format, args);
    }

    vscan_result<wistreambuf_subrange> vscan_and_sync(
        wistreambuf_subrange source,
        std::wstring_view format,
        scan_args_for<wistreambuf_subrange, wchar_t> args)
    {
        return vscan_and_sync_impl(source, format, args);
    }
#endif

    SCN_END_NAMESPACE
}  // namespace scn
