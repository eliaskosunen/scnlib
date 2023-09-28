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
#include <scn/detail/result.h>
#include <scn/detail/visitor.h>
#include <scn/detail/vscan.h>
#include <scn/detail/xchar.h>
#include <scn/impl/reader/reader.h>

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
        detail::vscan_impl_result<SourceRange> scan_simple_single_argument(
            SourceRange source,
            basic_scan_args<basic_scan_context<SourceRange, CharT>> args,
            basic_scan_arg<basic_scan_context<SourceRange, CharT>> arg,
            detail::locale_ref loc = {})
        {
            if (SCN_UNLIKELY(!arg)) {
                return unexpected_scan_error(scan_error::invalid_format_string,
                                             "Argument #0 not found");
            }

            auto reader = impl::default_arg_reader<
                basic_scan_context<SourceRange, CharT>>{SCN_MOVE(source),
                                                        SCN_MOVE(args), loc};

            return visit_scan_arg(SCN_MOVE(reader), arg);
        }

        template <typename Context, typename ID, typename Handler>
        auto get_arg(Context& ctx, ID id, Handler& handler) ->
            typename Context::arg_type
        {
            auto arg = ctx.arg(id);
            if (SCN_UNLIKELY(!arg)) {
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
                if (args_count >= 64) {
                    visited_args_upper.resize((args_count - 64) / 8);
                }
            }

            void on_literal_text(const CharT* begin, const CharT* end)
            {
                for (; begin != end; ++begin) {
                    auto it = ranges::begin(ctx.range());
                    if (impl::is_range_eof(it, ranges::end(ctx.range()))) {
                        SCN_UNLIKELY_ATTR
                        return on_error("Unexpected end of source");
                    }

                    if (auto [after_space_it, is_space] =
                            impl::is_first_char_space(
                                std::basic_string_view<CharT>{begin, end});
                        is_space) {
                        ctx.advance_to(
                            impl::read_while_classic_space(ctx.range()));
                        return;
                    }

                    if (*it != *begin) {
                        SCN_UNLIKELY_ATTR
                        return on_error(
                            "Unexpected literal character in source");
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
                    SCN_UNLIKELY_ATTR
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
                set_arg_as_visited(arg_id);

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
                set_arg_as_visited(arg_id);

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
                    SCN_UNLIKELY_ATTR
                    on_error("Missing '}' in format string");
                    return parse_ctx.begin();
                }
                if (SCN_UNLIKELY(!handler)) {
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
                {
                    const auto args_count_lower64 =
                        args_count >= 64 ? 64 : args_count;
                    const uint64_t mask =
                        args_count_lower64 == 64
                            ? std::numeric_limits<uint64_t>::max()
                            : (1ull << args_count_lower64) - 1;

                    if (visited_args_lower64 != mask) {
                        return on_error("Argument list not exhausted");
                    }
                }

                if (args_count < 64) {
                    return;
                }

                auto last_args_count = args_count - 64;
                for (auto it = visited_args_upper.begin();
                     it != visited_args_upper.end() - 1; ++it) {
                    if (*it != std::numeric_limits<uint8_t>::max()) {
                        return on_error("Argument list not exhausted");
                    }
                    last_args_count -= 8;
                }

                const auto mask =
                    static_cast<uint8_t>(1u << last_args_count) - 1;
                if (visited_args_upper.back() != mask) {
                    return on_error("Argument list not exhausted");
                }
            }

            void on_error(const char* msg)
            {
                SCN_UNLIKELY_ATTR
                error = scan_error{scan_error::invalid_format_string, msg};
            }
            void on_error(scan_error err)
            {
                if (SCN_UNLIKELY(err != scan_error::good)) {
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

            SCN_NODISCARD bool has_arg_been_visited(size_t id)
            {
                if (id >= args_count) {
                    on_error("Invalid out-of-range argument ID");
                    return false;
                }

                if (id < 64) {
                    return (visited_args_lower64 >> id) & 1ull;
                }

                id -= 64;
                return (visited_args_upper[id / 8] >> (id % 8)) & 1ull;
            }

            void set_arg_as_visited(size_t id)
            {
                if (id >= args_count) {
                    on_error("Invalid out-of-range argument ID");
                    return;
                }

                if (has_arg_been_visited(id)) {
                    return on_error(
                        "Argument with this ID has already been scanned");
                }

                if (id < 64) {
                    visited_args_lower64 |= (1ull << id);
                    return;
                }

                id -= 64;
                visited_args_upper[id / 8] |= (1ull << (id % 8));
            }

            parse_context_type parse_ctx;
            context_type ctx;
            scan_error error;
            std::size_t args_count;
            uint64_t visited_args_lower64{0};
            std::vector<uint8_t> visited_args_upper{};
        };

        template <typename SourceRange, typename CharT>
        detail::vscan_impl_result<SourceRange> vscan_internal(
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
            if (SCN_UNLIKELY(!handler)) {
                return unexpected(handler.error);
            }
            return handler.ctx.current();
        }

        template <typename SourceRange, typename CharT>
        detail::vscan_impl_result<SourceRange> vscan_value_internal(
            SourceRange source,
            basic_scan_arg<basic_scan_context<SourceRange, CharT>> arg)
        {
            return scan_simple_single_argument(SCN_MOVE(source), {}, arg);
        }
    }  // namespace

    namespace detail {
#define SCN_DEFINE_VSCAN(Range, CharT)                                         \
    vscan_impl_result<Range> vscan_impl(Range source,                          \
                                        std::basic_string_view<CharT> format,  \
                                        scan_args_for<Range, CharT> args)      \
    {                                                                          \
        return vscan_internal(SCN_MOVE(source), format, args);                 \
    }                                                                          \
                                                                               \
    template <typename Locale>                                                 \
    vscan_impl_result<Range> vscan_localized_impl(                             \
        const Locale& loc, Range source, std::basic_string_view<CharT> format, \
        scan_args_for<Range, CharT> args)                                      \
    {                                                                          \
        return vscan_internal(SCN_MOVE(source), format, args,                  \
                              detail::locale_ref{loc});                        \
    }                                                                          \
    template vscan_impl_result<Range> vscan_localized_impl<std::locale>(       \
        const std::locale& loc, Range source,                                  \
        std::basic_string_view<CharT> format,                                  \
        scan_args_for<Range, CharT> args);                                     \
                                                                               \
    vscan_impl_result<Range> vscan_value_impl(Range source,                    \
                                              scan_arg_for<Range, CharT> arg)  \
    {                                                                          \
        return vscan_value_internal(SCN_MOVE(source), arg);                    \
    }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
