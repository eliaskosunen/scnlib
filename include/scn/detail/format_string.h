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

#include <scn/detail/format_string_parser.h>

/**
 * \defgroup format-string Format strings
 *
 * \brief Format string description
 *
 * The format string syntax is heavily influenced by {fmt} and
 * `std::format`, and is largely compatible with it. Scanning functions,
 * such as `scn::scan` and
 * `scn::input`, use the format string syntax described in this section.
 *
 * Format strings consist of:
 *
 *  * Replacement fields, which are surrounded by curly braces `{}`.
 *
 *  * Non-whitespace characters (except `{}`; for literal braces, use
 *    `{{` and `}}`), which consume exactly one identical character from the
 *    input
 *
 *  * Whitespace characters, which consume any and all available consecutive
 *    whitespace from the input.
 *
 * Literal characters are matched by code point one-to-one, with no
 * normalization being done.
 * `Ä` (U+00C4, UTF-8 0xc3 0x84) only matches another U+00C4, and not, for
 * example, U+00A8 (DIAERESIS) and U+0041 (LATIN CAPITAL LETTER A).
 *
 * Characters (code points) are considered to be whitespace characters by
 * the Unicode Pattern_White_Space property, as defined by UAX31-R3a.
 * These code points are:
 *
 *  * ASCII whitespace characters ("\t\n\v\f\r ")
 *  * U+0085 (next line)
 *  * U+200E and U+200F (LEFT-TO-RIGHT MARK and RIGHT-TO-LEFT MARK)
 *  * U+2028 and U+2029 (LINE SEPARATOR and PARAGRAPH SEPARATOR)
 *
 * The grammar for a replacement field is as follows:
 *
 * \code
 * replacement-field   ::= '{' [arg-id] [':' format-spec] '}'
 * arg-id              ::= positive-integer
 *
 * format-spec         ::= [width] ['L'] [type]
 * width               ::= positive-integer
 * type                ::= 'a' | 'A' | 'b' | 'B' | 'c' | 'd' |
 *                         'e' | 'E' | 'f' | 'F' | 'g' | 'G' |
 *                         'o' | 'p' | 's' | 'x' | 'X' | 'i' | 'u'
 * \endcode
 *
 * \section arg-ids Argument IDs
 *
 * The `arg-id` specifier can be used to index arguments manually.
 * If manual indexing is used, all of the indices in a format string must be
 * stated explicitly. The same `arg-id` can appear in the format string
 * only once, and must refer to a valid argument.
 *
 * \code{.cpp}
 * // Format string equivalent to "{0} to {1}"
 * auto a = scn::scan<int, int>("2 to 300", "{} to {}");
 * // a->values() == (2, 300)
 *
 * // Manual indexing
 * auto b = scn::scan<int, int>("2 to 300", "{1} to {0}");
 * // b->values() == (3, 200)
 *
 * // INVALID:
 * // Automatic and manual indexing is mixed
 * auto c = scn::scan<int, int>("2 to 300", "{} to {0}");
 *
 * // INVALID:
 * // Same argument is referred to multiple times
 * auto d = scn::scan<int, int>("2 to 300", "{0} to {0}");
 *
 * // INVALID:
 * // {2} does not refer to an argument
 * auto e = scn::scan<int, int>("2 to 300", "{0} to {2}");
 * \endcode
 *
 * \subsection width Width
 *
 * Width specifies the maximum number of characters that will be read from
 * the source range. It can be any unsigned integer. When using the `'c'`
 * type specifier for strings, specifying the width is required.
 *
 * \code{.cpp}
 * auto r = scn::scan<std::string>("abcde", "{:3}");
 * // r->value() == "abc"
 * \endcode
 *
 * For the purposes of width calculation, the same algorithm is used that in
 * {fmt}. Every code point has a width of one, except the following ones
 * have a width of 2:
 *
 * * any code point with the East_Asian_Width="W" or East_Asian_Width="F"
 *   Derived Extracted Property as described by UAX#44
 * * U+4DC0 – U+4DFF (Yijing Hexagram Symbols)
 * * U+1F300 – U+1F5FF (Miscellaneous Symbols and Pictographs)
 * * U+1F900 – U+1F9FF (Supplemental Symbols and Pictographs)
 *
 * \section localized Localized
 *
 * The `L` flag enables localized scanning.
 * Its effects are different for each type it is used with:
 *
 *  * For integers, it enables locale-specific thousands separators
 *  * For floating-point numbers, it enables locale-specific thousands and
 *    radix (decimal) separators
 *  * For booleans, it enables locale-specific textual representations (for
 *    `true` and `false`)
 *  * For other types, it has no effect
 *
 * \section type Type specifier
 *
 * The type specifier determines how the data is to be scanned.
 * The type of the argument to be scanned determines what flags are valid.
 *
 * \subsection type-string Type specifier: strings
 *
 * <table>
 * <caption id="type-string-table">
 * String types (`std::basic_string` and `std::basic_string_view`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>none, `s`</td>
 * <td>
 * Copies from the input until a whitespace character is encountered.
 * Preceding whitespace is skipped.
 * </td>
 * </tr>
 * <tr>
 * <td>`c`</td>
 * <td>
 * Copies from the input until the field width is exhausted. Does not skip
 * preceding whitespace. Errors if no field width is provided.
 * </td>
 * </tr>
 * <tr>
 * <td>`[...]`</td>
 * <td>
 * Character set matching: copies from the input until a character not specified
 * in the set is encountered. Character ranges can be specified with `-`, and
 * the entire selection can be inverted with a prefix `^`. Matches and supports
 * arbitrary Unicode code points. Does not skip preceding whitespace.
 * </td>
 * </tr>
 * <tr>
 * <td>`/<regex>/<flags>`</td>
 * <td>
 * Regular expression matching: copies from the input until the input does not
 * match the regex.
 * Does not skip preceding whitespace.
 * \see regex
 * </td>
 * </tr>
 * </table>
 *
 * \note `std::basic_string_view` can only be scanned, if the source is
 * contiguous.
 *
 * \subsection type-int Type specifier: integers
 *
 * Integer values are scanned as if by using `std::from_chars`, except:
 *  * A positive `+` sign and a base prefix (like `0x`) are always
 *    allowed to be present
 *  * Preceding whitespace is skipped.
 *
 * <table>
 * <caption id="type-int-table">
 * Integer types (`signed` and `unsigned` variants of `char`, `short`,
 * `int`, `long`, and `long long`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`b`, `B`</td>
 * <td>
 * `std::from_chars` with base `2`. The base prefix is `0b` or `0B`.
 * </td>
 * </tr>
 * <tr>
 * <td>`o`, `O`</td>
 * <td>
 * `std::from_chars` with base `8`. The base prefix is `0o` or `0O`, or just
 * `0`.
 * </td>
 * </tr>
 * <tr>
 * <td>`x`, `X`</td>
 * <td>
 * `std::from_chars` with base `16`. The base prefix is `0x` or `0X`.
 * </td>
 * </tr>
 * <tr>
 * <td>`d`</td>
 * <td>
 * `std::from_chars` with base `10`. No base prefix allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`u`</td>
 * <td>
 * `std::from_chars` with base `10`. No base prefix or `-` sign allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`i`</td>
 * <td>
 * Detect the base from a possible prefix, defaulting to decimal (base-10).
 * </td>
 * </tr>
 * <tr>
 * <td>`rXX` (where XX = [2, 36])</td>
 * <td>
 * Custom base, without a base prefix (r stands for radix).
 * </td>
 * </tr>
 * <tr>
 * <td>`c`</td>
 * <td>
 * Copies a character (code unit) from the input.
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * Same as `d`.
 * </td>
 * </tr>
 * </table>
 *
 * \subsection type-char Type specifier: characters
 *
 * <table>
 * <caption id="type-char-table">
 * Character types (`char` and `wchar_t`), and code points (`char32_t`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>none, `c`</td>
 * <td>
 * Copies a character (code point for `char32_t`, code unit otherwise) from the
 * input.
 * </td>
 * </tr>
 * <tr>
 * <td>`b`, `B`, `d`, `i`, `o`, `O`, `u`, `x`, `X`</td>
 * <td>
 * Same as for integers, see above \ref type-int. Not allowed for `char32_t`.
 * </td>
 * </tr>
 * </table>
 *
 * \note When scanning characters (`char` and `wchar_t`), the source range is
 * read a single code unit at a time, and encoding is not respected.
 *
 * \subsection type-float Type specifier: floating-point values
 *
 * Floating-point values are scanned as if by using `std::from_chars`, except:
 *  * A positive `+` sign and a base prefix (like `0x`) are always
 *    allowed to be present
 *  * Preceding whitespace is skipped.
 *
 * <table>
 * <caption id="type-float-table">
 * Floating-point types (`float`, `double`, and `long double`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`a`, `A`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::hex`.
 * Prefix `0x`/`0X` is allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`e`, `E`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::scientific`.
 * </td>
 * </tr>
 * <tr>
 * <td>`f`, `F`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::fixed`.
 * </td>
 * </tr>
 * <tr>
 * <td>`g`, `G`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::general`.
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * `std::from_chars` with `std::chars_format::general | std::chars_format::hex`.
 * Prefix `0x`/`0X` is allowed.
 * </td>
 * </tr>
 * </table>
 *
 * \subsection type-bool Type specifier: booleans
 *
 * <table>
 * <caption id="type-bool-table">
 * `bool`
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`s`</td>
 * <td>
 * Allows for the textual representation (`true` or `false`).
 * </td>
 * </tr>
 * <tr>
 * <td>`b`, `B`, `d`, `i`, `o`, `O`, `u`, `x`, `X`</td>
 * <td>
 * Allows for the integral/numeric representation (`0` or `1`).
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * Allows for both the textual and the integral/numeric representation.
 * </td>
 * </tr>
 * </table>
 */

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
/**
 * A runtime format string
 *
 * \ingroup format-string
 */
template <typename CharT>
struct basic_runtime_format_string {
    basic_runtime_format_string(std::basic_string_view<CharT> s) : str(s) {}

    basic_runtime_format_string(const basic_runtime_format_string&) = delete;
    basic_runtime_format_string(basic_runtime_format_string&&) = delete;
    basic_runtime_format_string& operator=(const basic_runtime_format_string&) =
        delete;
    basic_runtime_format_string& operator=(basic_runtime_format_string&&) =
        delete;
    ~basic_runtime_format_string() = default;

    std::basic_string_view<CharT> str;
};
}  // namespace detail

/**
 * Create a runtime format string
 *
 * Can be used to avoid compile-time format string checking
 *
 * \ingroup format-string
 */
inline detail::basic_runtime_format_string<char> runtime_format(
    std::string_view s)
{
    return s;
}
inline detail::basic_runtime_format_string<wchar_t> runtime_format(
    std::wstring_view s)
{
    return s;
}

namespace detail {
struct compile_string {};

template <typename Str>
inline constexpr bool is_compile_string_v =
    std::is_base_of_v<compile_string, Str>;

template <typename Scanner, typename = void>
inline constexpr bool scanner_has_format_specs_member_v = false;
template <typename Scanner>
inline constexpr bool scanner_has_format_specs_member_v<
    Scanner,
    std::void_t<decltype(SCN_DECLVAL(Scanner&)._format_specs())>> = true;

template <typename T, typename Source, typename Ctx, typename ParseCtx>
constexpr typename ParseCtx::iterator parse_format_specs(ParseCtx& parse_ctx)
{
    using char_type = typename ParseCtx::char_type;
    using mapped_type = std::conditional_t<
        mapped_type_constant<T, char_type>::value != arg_type::custom_type,
        std::remove_reference_t<decltype(arg_mapper<char_type>().map(
            SCN_DECLVAL(T&)))>,
        T>;
    auto s = typename Ctx::template scanner_type<mapped_type>{};
    auto it = s.parse(parse_ctx)
                  .transform_error([&](scan_error err) constexpr {
                      parse_ctx.on_error(err.msg());
                      return err;
                  })
                  .value_or(parse_ctx.end());
    if constexpr (scanner_has_format_specs_member_v<decltype(s)>) {
        auto& specs = s._format_specs();
        if ((specs.type == presentation_type::regex ||
             specs.type == presentation_type::regex_escaped) &&
            !(ranges::range<Source> && ranges::contiguous_range<Source>)) {
            // clang-format off
            parse_ctx.on_error("Cannot read a regex from a non-contiguous source");
            // clang-format on
        }
    }
    return it;
}

template <typename CharT, typename Source, typename... Args>
class format_string_checker {
public:
    using parse_context_type = compile_parse_context<CharT>;
    static constexpr auto num_args = sizeof...(Args);

    explicit constexpr format_string_checker(
        std::basic_string_view<CharT> format_str)
        : m_parse_context(format_str,
                          num_args,
                          m_types,
                          type_identity<Source>{}),
          m_parse_funcs{&parse_format_specs<Args,
                                            Source,
                                            basic_scan_context<CharT>,
                                            parse_context_type>...},
          m_types{arg_type_constant<Args, CharT>::value...}
    {
    }

    constexpr void on_literal_text(const CharT* begin, const CharT* end) const
    {
        // TODO: Do we want to validate Unicode in format strings?
        // We're dealing with text, we probably do.
        // We could do codeunit-to-codeunit matching,
        // but that could get messy wrt. whitespace matching.
        // It's simpler to not allow nonsense.
#if 0
                SCN_UNUSED(begin);
                SCN_UNUSED(end);
#else
        while (begin != end) {
            const auto len =
                utf_code_point_length_by_starting_code_unit(*begin);
            if (SCN_UNLIKELY(len == 0 ||
                             static_cast<size_t>(end - begin) < len)) {
                return on_error("Invalid encoding in format string");
            }

            const auto cp = decode_utf_code_point_exhaustive(
                std::basic_string_view<CharT>{begin, len});
            if (SCN_UNLIKELY(cp >= invalid_code_point)) {
                return on_error("Invalid encoding in format string");
            }

            begin += len;
        }
#endif
    }

    constexpr auto on_arg_id()
    {
        return m_parse_context.next_arg_id();
    }
    constexpr auto on_arg_id(std::size_t id)
    {
        m_parse_context.check_arg_id(id);
        return id;
    }

    constexpr void on_replacement_field(size_t id, const CharT*)
    {
        m_parse_context.check_arg_can_be_read(id);
        set_arg_as_read(id);

        if (m_types[id] == arg_type::narrow_regex_matches_type ||
            m_types[id] == arg_type::wide_regex_matches_type) {
            // clang-format off
            return on_error("Regular expression needs to be specified when reading regex_matches");
            // clang-format on
        }
    }

    constexpr const CharT* on_format_specs(std::size_t id,
                                           const CharT* begin,
                                           const CharT*)
    {
        m_parse_context.check_arg_can_be_read(id);
        set_arg_as_read(id);
        m_parse_context.advance_to(begin);
        return id < num_args ? m_parse_funcs[id](m_parse_context) : begin;
    }

    constexpr void check_args_exhausted() const
    {
        if (num_args == 0) {
            return;
        }
        for (auto is_set : m_visited_args) {
            if (!is_set) {
                return on_error("Argument list not exhausted");
            }
        }
    }

    void on_error(const char* msg) const
    {
        SCN_UNLIKELY_ATTR
        m_parse_context.on_error(msg);
    }

    // Only to satisfy the concept and eliminate compiler errors,
    // because errors are reported by failing to compile on_error above
    // (it's not constexpr)
    constexpr explicit operator bool() const
    {
        return true;
    }
    constexpr scan_error get_error() const
    {
        return {};
    }

private:
    constexpr void set_arg_as_read(size_t id)
    {
        if (id >= num_args) {
            return on_error("Invalid out-of-range argument ID");
        }
        if (m_visited_args[id]) {
            return on_error("Argument with this ID already scanned");
        }
        m_visited_args[id] = true;
    }

    using parse_func = const CharT* (*)(parse_context_type&);

    parse_context_type m_parse_context;
    parse_func m_parse_funcs[num_args > 0 ? num_args : 1];
    arg_type m_types[num_args > 0 ? num_args : 1];
    bool m_visited_args[num_args > 0 ? num_args : 1] = {false};
};

template <typename Source, typename... Args, typename Str>
auto check_format_string(const Str&)
    -> std::enable_if_t<!is_compile_string_v<Str>>
{
    // TODO: SCN_ENFORE_COMPILE_STRING?
#if 0  // SCN_ENFORE_COMPILE_STRING
    static_assert(dependent_false<Str>::value,
              "SCN_ENFORCE_COMPILE_STRING requires all format "
              "strings to use SCN_STRING.");
#endif
}

template <typename Source, typename... Args, typename Str>
auto check_format_string(Str format_str)
    -> std::enable_if_t<is_compile_string_v<Str>>
{
    using char_type = typename Str::char_type;

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wconversion")
    constexpr auto s = std::basic_string_view<char_type>{format_str};
    SCN_GCC_POP

    using checker = format_string_checker<char_type, Source, Args...>;
    constexpr bool invalid_format =
        (parse_format_string<true>(s, checker(s)), true);
    SCN_UNUSED(invalid_format);
}

template <typename CharT, std::size_t N>
constexpr std::basic_string_view<CharT> compile_string_to_view(
    const CharT (&s)[N])
{
    return {s, N - 1};
}
template <typename CharT>
constexpr std::basic_string_view<CharT> compile_string_to_view(
    std::basic_string_view<CharT> s)
{
    return s;
}
}  // namespace detail

#define SCN_STRING_IMPL(s, base, expl)                                       \
    [] {                                                                     \
        struct SCN_COMPILE_STRING : base {                                   \
            using char_type = ::scn::detail::remove_cvref_t<decltype(s[0])>; \
            SCN_MAYBE_UNUSED constexpr expl                                  \
            operator ::std::basic_string_view<char_type>() const             \
            {                                                                \
                return ::scn::detail::compile_string_to_view<char_type>(s);  \
            }                                                                \
        };                                                                   \
        return SCN_COMPILE_STRING{};                                         \
    }()

#define SCN_STRING(s) SCN_STRING_IMPL(s, ::scn::detail::compile_string, )

/**
 * Compile-time format string
 *
 * \ingroup format-string
 */
template <typename CharT, typename Source, typename... Args>
class basic_scan_format_string {
public:
    SCN_CLANG_PUSH
#if SCN_CLANG >= SCN_COMPILER(10, 0, 0)
    SCN_CLANG_IGNORE("-Wc++20-compat")  // false positive about consteval
#endif
    template <
        typename S,
        std::enable_if_t<
            std::is_convertible_v<const S&, std::basic_string_view<CharT>> &&
            detail::is_not_self<S, basic_scan_format_string>>* = nullptr>
    SCN_CONSTEVAL basic_scan_format_string(const S& s) : m_str(s)
    {
#if SCN_HAS_CONSTEVAL
        using checker = detail::format_string_checker<CharT, Source, Args...>;
        const auto e = detail::parse_format_string<true>(m_str, checker(s));
        SCN_UNUSED(e);
#else
        detail::check_format_string<Source, Args...>(s);
#endif
    }
    SCN_CLANG_POP

    template <
        typename OtherSource,
        std::enable_if_t<std::is_same_v<detail::remove_cvref_t<Source>,
                                        detail::remove_cvref_t<OtherSource>> &&
                         ranges::borrowed_range<Source> ==
                             ranges::borrowed_range<OtherSource>>* = nullptr>
    constexpr basic_scan_format_string(
        const basic_scan_format_string<CharT, OtherSource, Args...>& other)
        : m_str(other.get())
    {
    }

    basic_scan_format_string(detail::basic_runtime_format_string<CharT> r)
        : m_str(r.str)
    {
    }

    constexpr operator std::basic_string_view<CharT>() const
    {
        return m_str;
    }
    constexpr std::basic_string_view<CharT> get() const
    {
        return m_str;
    }

private:
    std::basic_string_view<CharT> m_str;
};

SCN_END_NAMESPACE
}  // namespace scn
