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
#include <scn/impl/reader/integer_reader.h>
#include <scn/impl/reader/reader.h>
#include <scn/impl/util/contiguous_context.h>

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

template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    std::basic_string_view<CharT> source,
    basic_scan_args<basic_scan_context<CharT>> args,
    basic_scan_arg<basic_scan_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Argument #0 not found");
    }

    auto reader =
        impl::default_arg_reader<impl::basic_contiguous_scan_context<CharT>>{
            ranges::subrange<const CharT*>{source.data(),
                                           source.data() + source.size()},
            SCN_MOVE(args), loc};
    SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
    return ranges::distance(source.data(), it);
}
template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    detail::basic_scan_buffer<CharT>& source,
    basic_scan_args<basic_scan_context<CharT>> args,
    basic_scan_arg<basic_scan_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     "Argument #0 not found");
    }

    if (SCN_LIKELY(source.is_contiguous())) {
        auto reader = impl::default_arg_reader<
            impl::basic_contiguous_scan_context<CharT>>{source.get_contiguous(),
                                                        SCN_MOVE(args), loc};
        SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
        return ranges::distance(source.get_contiguous().begin(), it);
    }

    auto reader = impl::default_arg_reader<basic_scan_context<CharT>>{
        source.get(), SCN_MOVE(args), loc};
    SCN_TRY(it, visit_scan_arg(SCN_MOVE(reader), arg));
    return it.position();
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

template <typename Context>
class specs_handler : public detail::specs_setter {
public:
    using char_type = typename Context::char_type;
    using arg_type = typename Context::arg_type;

    constexpr specs_handler(detail::format_specs& specs,
                            basic_scan_parse_context<char_type>& parse_ctx,
                            Context& ctx)
        : detail::specs_setter(specs), m_parse_ctx(parse_ctx), m_ctx(ctx)
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

    basic_scan_parse_context<char_type>& m_parse_ctx;
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
            const auto args_count_lower64 = args_count >= 64 ? 64 : args_count;
            const uint64_t mask = args_count_lower64 == 64
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

        const auto mask = static_cast<uint8_t>(1u << last_args_count) - 1;
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
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error("Invalid out-of-range argument ID");
            return false;
        }

        if (SCN_LIKELY(id < 64)) {
            return (visited_args_lower64 >> id) & 1ull;
        }

        id -= 64;
        return (visited_args_upper[id / 8] >> (id % 8)) & 1ull;
    }

    void set_arg_as_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error("Invalid out-of-range argument ID");
            return;
        }

        if (SCN_UNLIKELY(has_arg_been_visited(id))) {
            return on_error("Argument with this ID has already been scanned");
        }

        if (SCN_LIKELY(id < 64)) {
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

template <typename CharT>
struct simple_context_wrapper {
    using context_type = basic_scan_context<CharT>;

    simple_context_wrapper(detail::basic_scan_buffer<CharT>& source,
                           basic_scan_args<basic_scan_context<CharT>> args,
                           detail::locale_ref loc)
        : ctx(source.get().begin(), SCN_MOVE(args), loc)
    {
    }

    basic_scan_context<CharT>& get()
    {
        return ctx;
    }
    basic_scan_context<CharT>& get_custom()
    {
        return ctx;
    }

    basic_scan_context<CharT> ctx;
};

template <typename CharT>
struct contiguous_context_wrapper {
    using context_type = impl::basic_contiguous_scan_context<CharT>;

    contiguous_context_wrapper(ranges::subrange<const CharT*> source,
                               basic_scan_args<basic_scan_context<CharT>> args,
                               detail::locale_ref loc)
        : contiguous_ctx(source, args, loc)
    {
    }

    impl::basic_contiguous_scan_context<CharT>& get()
    {
        return contiguous_ctx;
    }
    basic_scan_context<CharT>& get_custom()
    {
        if (!buffer) {
            buffer.emplace(detail::make_string_view_from_pointers(
                ranges::data(contiguous_ctx.underlying_range()),
                ranges::data(contiguous_ctx.underlying_range()) +
                    ranges::size(contiguous_ctx.underlying_range())));
        }
        auto it = buffer->get().begin();
        it.batch_advance_to(contiguous_ctx.begin_position());
        custom_ctx.emplace(it, contiguous_ctx.args(), contiguous_ctx.locale());
        return *custom_ctx;
    }

    impl::basic_contiguous_scan_context<CharT> contiguous_ctx;
    std::optional<detail::basic_scan_string_buffer<CharT>> buffer{std::nullopt};
    std::optional<basic_scan_context<CharT>> custom_ctx{std::nullopt};
};

template <bool Contiguous, typename CharT>
using context_wrapper_t = std::conditional_t<Contiguous,
                                             contiguous_context_wrapper<CharT>,
                                             simple_context_wrapper<CharT>>;

template <bool Contiguous, typename CharT>
struct format_handler : format_handler_base {
    using context_wrapper_type = context_wrapper_t<Contiguous, CharT>;
    using context_type = typename context_wrapper_type::context_type;
    using char_type = typename context_type::char_type;
    using format_type = std::basic_string_view<char_type>;

    using parse_context_type = typename context_type::parse_context_type;
    using args_type = basic_scan_args<basic_scan_context<char_type>>;

    template <typename Source>
    format_handler(Source&& source,
                   format_type format,
                   args_type args,
                   detail::locale_ref loc,
                   std::size_t argcount)
        : format_handler_base{argcount},
          parse_ctx{format},
          ctx{SCN_FWD(source), SCN_MOVE(args), SCN_MOVE(loc)}
    {
    }

    void on_literal_text(const char_type* begin, const char_type* end)
    {
        for (; begin != end; ++begin) {
            auto it = get_ctx().begin();
            if (impl::is_range_eof(it, get_ctx().end())) {
                SCN_UNLIKELY_ATTR
                return on_error("Unexpected end of source");
            }

            if (auto [after_space_it, cp, is_space] = impl::is_first_char_space(
                    detail::make_string_view_from_pointers(begin, end));
                cp == detail::invalid_code_point) {
                return on_error("Invalid encoding in format string");
            }
            else if (is_space) {
                get_ctx().advance_to(
                    impl::read_while_classic_space(get_ctx().range()));
                begin = detail::to_address(std::prev(after_space_it));
                continue;
            }

            if (*it != *begin) {
                SCN_UNLIKELY_ATTR
                return on_error("Unexpected literal character in source");
            }
            get_ctx().advance_to(ranges::next(it));
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
            get_ctx().advance_to(*r);
        }
    }

    void on_replacement_field(std::size_t arg_id, const char_type*)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        on_visit_scan_arg(
            impl::default_arg_reader<context_type>{
                get_ctx().range(), get_ctx().args(), get_ctx().locale()},
            arg);
    }

    const char_type* on_format_specs(std::size_t arg_id,
                                     const char_type* begin,
                                     const char_type* end)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        if (arg.type() == detail::arg_type::custom_type) {
            parse_ctx.advance_to(begin);
            on_visit_scan_arg(
                impl::custom_reader<basic_scan_context<char_type>>{
                    parse_ctx, get_custom_ctx()},
                arg);
            return parse_ctx.begin();
        }

        auto specs = detail::format_specs{};
        detail::specs_checker<specs_handler<context_type>> handler{
            specs_handler<context_type>{specs, parse_ctx, get_ctx()},
            arg.type()};

        begin = detail::parse_format_specs(begin, end, handler);
        if (begin == end || *begin != char_type{'}'}) {
            SCN_UNLIKELY_ATTR
            on_error("Missing '}' in format string");
            return parse_ctx.begin();
        }
        if (SCN_UNLIKELY(!handler)) {
            return parse_ctx.begin();
        }
        parse_ctx.advance_to(begin);

        on_visit_scan_arg(
            impl::arg_reader<context_type>{get_ctx().range(), specs,
                                           get_ctx().locale()},
            arg);
        return parse_ctx.begin();
    }

    context_type& get_ctx()
    {
        return ctx.get();
    }
    auto& get_custom_ctx()
    {
        return ctx.get_custom();
    }

    parse_context_type parse_ctx;
    context_wrapper_type ctx;
};

template <typename CharT, typename Handler>
scan_expected<std::ptrdiff_t> vscan_parse_format_string(
    std::basic_string_view<CharT> format,
    Handler& handler)
{
    const auto beg = handler.get_ctx().begin();
    detail::parse_format_string<false>(format, handler);
    if (SCN_UNLIKELY(!handler)) {
        return unexpected(handler.error);
    }
    return ranges_polyfill::pos_distance(beg, handler.get_ctx().begin());
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    std::basic_string_view<CharT> source,
    std::basic_string_view<CharT> format,
    basic_scan_args<basic_scan_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(source, SCN_MOVE(args), arg);
    }

    auto handler = format_handler<true, CharT>{
        ranges::subrange<const CharT*>{source.data(),
                                       source.data() + source.size()},
        format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
    return vscan_parse_format_string(format, handler);
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    detail::basic_scan_buffer<CharT>& buffer,
    std::basic_string_view<CharT> format,
    basic_scan_args<basic_scan_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(buffer, SCN_MOVE(args), arg);
    }

    if (buffer.is_contiguous()) {
        auto handler = format_handler<true, CharT>{buffer.get_contiguous(),
                                                   format, SCN_MOVE(args),
                                                   SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }

    SCN_UNLIKELY_ATTR
    {
        auto handler = format_handler<false, CharT>{
            buffer, format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }
}

template <typename Source, typename CharT>
scan_expected<std::ptrdiff_t> vscan_value_internal(
    Source&& source,
    basic_scan_arg<basic_scan_context<CharT>> arg)
{
    return scan_simple_single_argument(SCN_FWD(source), {}, arg);
}
}  // namespace

namespace detail {
template <typename T>
auto scan_int_impl(std::string_view source, T& value, int base)
    -> scan_expected<std::string_view::iterator>
{
    SCN_TRY(beg, impl::skip_classic_whitespace(source).transform_error(
                     impl::make_eof_scan_error));
    auto reader = impl::reader_impl_for_int<char>{};
    return reader.read_default_with_base(ranges::subrange{beg, source.end()},
                                         value, base);
}

template <typename T>
auto scan_int_exhaustive_valid_impl(std::string_view source) -> T
{
    T value{};
    impl::parse_integer_value_exhaustive_valid(source, value);
    return value;
}
}  // namespace detail

scan_error vinput(std::string_view format, scan_args args)
{
    auto buffer = detail::make_file_scan_buffer(stdin);
    auto n = vscan_internal(buffer, format, args);
    if (n) {
        buffer.sync(*n);
        return {};
    }
    buffer.sync_all();
    return n.error();
}

namespace detail {
scan_expected<std::ptrdiff_t> vscan_impl(std::string_view source,
                                         std::string_view format,
                                         scan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(scan_buffer& source,
                                         std::string_view format,
                                         scan_args args)
{
    auto n = vscan_internal(source, format, args);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

scan_expected<std::ptrdiff_t> vscan_impl(std::wstring_view source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(wscan_buffer& source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    auto n = vscan_internal(source, format, args);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

#if !SCN_DISABLE_LOCALE
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::string_view source,
                                                   std::string_view format,
                                                   scan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   scan_buffer& source,
                                                   std::string_view format,
                                                   scan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::wstring_view source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   wscan_buffer& source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::string_view,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                scan_buffer&,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::wstring_view,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                wscan_buffer&,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
#endif

scan_expected<std::ptrdiff_t> vscan_value_impl(std::string_view source,
                                               basic_scan_arg<scan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(scan_buffer& source,
                                               basic_scan_arg<scan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::wstring_view source,
    basic_scan_arg<wscan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(
    wscan_buffer& source,
    basic_scan_arg<wscan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    if (SCN_LIKELY(n)) {
        source.sync(*n);
    }
    else {
        source.sync_all();
    }
    return n;
}

#if !SCN_DISABLE_TYPE_SCHAR
template auto scan_int_impl(std::string_view, signed char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> signed char;
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
template auto scan_int_exhaustive_valid_impl(std::string_view) -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
template auto scan_int_impl(std::string_view, unsigned char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned char;
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
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
template auto scan_int_impl(std::string_view, unsigned long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned long;
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
