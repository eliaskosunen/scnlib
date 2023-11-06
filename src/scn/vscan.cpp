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

#if !SCN_DISABLE_IOSTREAM
#include <iostream>
#include <mutex>
#endif

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

        struct format_handler_base {
            format_handler_base(size_t argcount) : args_count(argcount)
            {
                if (SCN_UNLIKELY(args_count >= 64)) {
                    visited_args_upper.resize((args_count - 64) / 8);
                }
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

            std::size_t args_count;
            scan_error error{};
            uint64_t visited_args_lower64{0};
            std::vector<uint8_t> visited_args_upper{};
        };

        template <typename SourceRange, typename CharT>
        struct format_handler : format_handler_base {
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
                : format_handler_base{argcount},
                  parse_ctx{format},
                  ctx{SCN_MOVE(source), SCN_MOVE(args), SCN_MOVE(loc)}
            {
            }

            void on_literal_text(const CharT* begin, const CharT* end)
            {
                for (; begin != end; ++begin) {
                    auto it = ranges::begin(ctx.range());
                    if (impl::is_range_eof(it, ranges::end(ctx.range()))) {
                        SCN_UNLIKELY_ATTR
                        return on_error("Unexpected end of source");
                    }

                    if (auto [after_space_it, cp, is_space] =
                            impl::is_first_char_space(
                                detail::make_string_view_from_pointers(begin,
                                                                       end));
                        cp == detail::invalid_code_point) {
                        return on_error("Invalid encoding in format string");
                    }
                    else if (is_space) {
                        ctx.advance_to(
                            impl::read_while_classic_space(ctx.range()));
                        begin = detail::to_address(std::prev(after_space_it));
                        continue;
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

                auto r = visit_scan_arg(SCN_FWD(visitor), arg);
                if (SCN_UNLIKELY(!r)) {
                    on_error(r.error());
                }
                else {
                    ctx.advance_to(*r);
                }
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

            parse_context_type parse_ctx;
            context_type ctx;
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

#if !SCN_DISABLE_IOSTREAM
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
#endif  // !SCN_DISABLE_IOSTREAM

        template <typename T>
        auto scan_int_impl(std::string_view source, T& value, int base)
            -> scan_expected<std::string_view::iterator>
        {
            SCN_TRY(beg, impl::skip_classic_whitespace(source));

            auto reader = impl::integer_reader<char>{0, base};
            SCN_TRY(_, reader.read_source(tag_type<T>{},
                                          ranges::subrange(beg, source.end())));
            SCN_TRY(n, reader.parse_value(value));
            return ranges::next(beg, n);
        }

        template <typename T>
        auto scan_int_exhaustive_valid_impl(std::string_view source) -> T
        {
            T value{};
            impl::parse_int_value_exhaustive_valid(source, value);
            return value;
        }
    }  // namespace detail

    namespace detail {

#if !SCN_DISABLE_LOCALE
#define SCN_DEFINE_VSCAN_LOCALIZED(Range, CharT)                               \
    template <typename Locale>                                                 \
    vscan_impl_result<Range> vscan_localized_impl(                             \
        const Locale& loc, Range source, std::basic_string_view<CharT> format, \
        scan_args_for<Range, CharT> args)                                      \
    {                                                                          \
        return vscan_internal(SCN_MOVE(source), format, args,                  \
                              detail::locale_ref{loc});                        \
    }                                                                          \
    template vscan_impl_result<Range> vscan_localized_impl<std::locale>(       \
        const std::locale&, Range, std::basic_string_view<CharT>,              \
        scan_args_for<Range, CharT>);
#else
#define SCN_DEFINE_VSCAN_LOCALIZED(...) /* vscan_localized_impl disabled */
#endif

#define SCN_DEFINE_VSCAN(Range, CharT)                                        \
    vscan_impl_result<Range> vscan_impl(Range source,                         \
                                        std::basic_string_view<CharT> format, \
                                        scan_args_for<Range, CharT> args)     \
    {                                                                         \
        return vscan_internal(SCN_MOVE(source), format, args);                \
    }                                                                         \
    vscan_impl_result<Range> vscan_value_impl(Range source,                   \
                                              scan_arg_for<Range, CharT> arg) \
    {                                                                         \
        return vscan_value_internal(SCN_MOVE(source), arg);                   \
    }                                                                         \
    SCN_DEFINE_VSCAN_LOCALIZED(Range, CharT)

        SCN_DEFINE_VSCAN(std::string_view, char)
        SCN_DEFINE_VSCAN(std::wstring_view, wchar_t)
#if !SCN_DISABLE_IOSTREAM
        SCN_DEFINE_VSCAN(istreambuf_subrange, char)
        SCN_DEFINE_VSCAN(wistreambuf_subrange, wchar_t)
#endif
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DEFINE_VSCAN(erased_subrange, char)
        SCN_DEFINE_VSCAN(werased_subrange, wchar_t)
#endif

#if !SCN_DISABLE_TYPE_SCHAR
        template auto scan_int_impl(std::string_view, signed char&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
        template auto scan_int_impl(std::string_view, short&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
        template auto scan_int_impl(std::string_view, int&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
        template auto scan_int_impl(std::string_view, long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view) -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
        template auto scan_int_impl(std::string_view, long long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
        template auto scan_int_impl(std::string_view, unsigned char&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
        template auto scan_int_impl(std::string_view, unsigned short&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
        template auto scan_int_impl(std::string_view, unsigned int&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
        template auto scan_int_impl(std::string_view, unsigned long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
        template auto scan_int_impl(std::string_view, unsigned long long&, int)
            -> scan_expected<std::string_view::iterator>;
        template auto scan_int_exhaustive_valid_impl(std::string_view)
            -> unsigned long long;
#endif

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
