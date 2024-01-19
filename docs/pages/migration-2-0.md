\page migration-2-0 Migration Guide v1.1 -> v2.0
\tableofcontents

For v2.0, the library was rewritten and redesigned in its entirety.
The new design is more focused and powerful, and closer to `std::format` than previously.

This guide isn't exhaustive, because the changes are very extensive, but should be enough to get you started.

\section m2-cpp17 C++17 required

v1 required C++11 in order to compile. v2, at least at this point, requires C++17.

\section headers Header files changed

The base header is renamed from `<scn/scn.h>` in v1 to `<scn/scan.h>` in v2.

To get support for `wchar_t` input, include `<scn/xchar.h>`. This is done to ease compile times.

\section m2-scan_prefix "scan_" prefix added to many names

To prepare for standardization, in v2, many names have the prefix `scan_`,
or otherwise indicate being related to scanning.

Changes include:

| v1                                                    | v2                                                                   |
|:------------------------------------------------------|:---------------------------------------------------------------------|
| `scn::error`                                          | `scn::scan_error`                                                    |
| `scn::basic_arg`, `scn::basic_args`, `scn::arg_store` | `scn::basic_scan_arg`, `scn::basic_scan_args`, `scn::scan_arg_store` |
| `scn::basic_context`                                  | `scn::basic_scan_context`, `scn::scan_context`                       |
| `scn::basic_parse_context`, `scn::parse_context`      | `scn::basic_scan_parse_context`, `scn::scan_parse_context`           |

\section m2-scan_arg_passing Argument passing and return value

The largest change is in how values are returned from `scn::scan` and other scanning functions.

In v1, values were passed to `scn::scan` by lvalue reference as out parameters.
The return value was used to get information about the leftover input data, and about possible errors.

\code{.cpp}
int i;
std::string str;
auto result = scn::scan("123 input", "{} {}", i, str);
\endcode

In v2, the values are returned from `scn::scan`, wrapped in an ``scn::expected``.
The types of the arguments are given in an explicit template parameter list,
instead of being deduced from the given arguments.

\code{.cpp}
auto result = scn::scan<int, std::string>("123 input", "{} {}");
if (result)
auto& [i, str] = result->values();
\endcode

The `result` value above is truthy when the operation was successful.
Use `result->range()` to get a `subrange` over the unparsed input,
`result->begin()` and `result->end()` to get the beginning and the end of that range, respectively, and
`result->values()` to access the parsed values through a `std::tuple`.
If only a single value is read, `result->value()` can be used to access it directly.
If `result` contains an error, use `result.error()` to access it.

\section m2-indirect No more "indirect" ranges: revamped source range error handling

The notion of "indirect" ranges from v1 is removed in v2.
Indirect ranges were source ranges, the value type of which was `scn::expected<CharT>`, instead of `CharT`.
This was to enable source ranges to report their own errors to the library,
and for it to pass them forward to the user.
In v2, the value type of the source range must either be `char` or `wchar_t`.

This approach was arguably against the principles of Ranges,
and made a lot of things more complicated than they needed to be.

In v2, the separation of I/O and input parsing is more clearly separated.
scnlib is not intended to be an I/O library, and that it shan't try to be.
In the optimal case, if I/O needs to be performed to fetch the data to be passed to scnlib,
that is done by the user, to ensure proper behavior and error recovery.
Also, when scnlib is given plain contiguous strings as input, instead of more complicated ranges,
a number of optimizations are enabled.

\code{.cpp}
std::string input;
std::getline(file, input);

auto result = scn::scan<int>(input, "{}");
\endcode

If doing your own I/O isn't possible, or is for some reason unfeasible, some other options are available:

 1) A `FILE*` can be given as the source to `scn::scan`, or in the case of `stdin`, `scn::input` can be used.
    

 1) A `scn::basic_istreambuf_view`/`scn::basic_istreambuf_subrange` can be given as a source range to `scn::scan`.
    These types wrap an arbitrary `std::basic_istream`/`std::basic_streambuf`.
    This can be useful, if you already have a `std::basic_istream`,
    and don't want to accidentally read anything extra from the stream, like with `std::cin`.
 
    \code{.cpp}
    auto result = scn::scan<int>(my_file, "{}");
    \endcode
 
 2) Signal errors like any other range signals them: by reaching the end prematurely, or with exceptions (discouraged).
    If using a custom user-provided range, this is likely the only option.
 
    \code{.cpp}
    auto result = scn::scan<int, double>(custom_source_range, "{} {}");
    // result can be true, if both the int and the double could be scanned,
    // but the given range reached an error condition.
    // We need to do the checking ourselves through custom_source_range, through whatever mechanism it provides
    if (result && custom_source_range.good()) {
        auto& [i, d] = result->values();
    }
 
    // Alternatively, if custom_source_range throws on error
    try {
        auto result = scn::scan<int, double>(custom_source_range, "{} {}");
        if (result) {
            auto& [i, d] = result->values();
        }
    } catch (const custom_source_range_error& e) {
        // ...
    }
    \endcode

\section m2-range-requirements Relaxed source range requirements

The set of allowed source ranges to be given to `scn::scan` is increased in v2, compared to v1.

In v1, a range was scannable, if it was bidirectional, and default and move constructible.

In v2, the range needs to just be a `forward_range`, and movable.

\section m2-ownership Returned ranges do not take ownership

In v1, the lifetime semantics of the range returned from `scn::scan` were complicated.
Usually, the returned range was a view over the given range, i.e. reference semantics were used.
But, sometimes, if the range was an rvalue container (or anything else that didn't model `borrowed_range`),
the return value contained that range, i.e. ownership was taken.

\code{.cpp}
// v1: reference semantics
int i{};
auto result = scn::scan("123 456", "{}", i);
// result contains a string_view over the given string literal

// v1: reference semantics
std::string source{"123 456"};
int i{};
auto result = scn::scan(source, "{}", i);
// result contains a string_view over source

// v1: ownership semantics
int i{};
auto result = scn::scan(std::string{"123 456"}, "{}", i);
// result contains a std::string
\endcode

In v2, the semantics are clearer: a view (`subrange`) over the given range is always returned.
If that view would dangle, `ranges::dangling` is returned instead.

\code{.cpp}
// v2: reference semantics (no change)
auto result = scn::scan<int>("123 456", "{}");
// result->begin() points to the given string literal

// v2: reference semantics (no change)
std::string source{};
auto result = scn::scan<int>(source, "{}");
// result->begin() points to source

// v2: dangling
auto result = scn::scan<int>(std::string{"123 456"}, "{}");
// result->begin() is of type scn::ranges::dangling, the given std::string has gone out of scope and been destroyed
\endcode

In other words, in v2, `scn::scan` always returns an iterator pointing to the given range.
If that's not possible without dangling, it returns `scn::ranges::dangling` instead.

\section m2-files Files removed

In v1, scnlib provided support for reading files with `scn::file`, `scn::owning_file`,
and `scn::mapped_file`. These caused the library to grow in size, blurred its focus, and were the source of many bugs.

In v2, these have been removed.
If you need to read from a file, either do your own I/O and give `scn::scan` a string,
or use `scn::scan` with a `FILE*`.
If you need to use memory mapped files, do the mapping yourself, and give `scn::scan` a view into the mapped memory.

In v2, `scn::cstdin()` and `scn::wcstdin()` have been removed.
For reading from stdin, use `scn::input` and `scn::prompt`,
or `scn::scan` with `stdin`.

\code{.cpp}
// v1:
int i;
auto result = scn::input("{}", i);
// or
auto result = scn::scan(scn::cstdin(), "{}", i);

// v2:
auto result = scn::input<int>("{}");
// or
auto result = scn::scan<int>(stdin, in, "{}");
\endcode

\section m2-scanner-specialize Specializing scn::scanner changed

In v1, `scn::scanner` took the type it was used for as a template parameter.
Inside it, `parse()` and `scan()` returned a `scn::error`.

\code{.cpp}
struct int_and_double {
    int i;
    double d;
};

template <>
struct scn::scanner<int_and_double> {
    template <typename ParseCtx>
    error parse(ParseCtx& pctx);

    template <typename Context>
    error scan(int_and_double& val, Context& ctx) const;

};
\endcode

In v2, `scn::scanner` also takes in the character type of the source range.
This is consistent with `std::formatter`.
The character type defaults to `char`.

`parse()` and `scan()` return a `scn::scan_expected<iterator>`.

`parse()` should be `constexpr`, to support compile-time format string checking.

\code{.cpp}
struct int_and_double {
   int i;
   double d;
};

template <typename CharT>
struct scn::scanner<int_and_double, CharT> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> scan_expected<typename ParseCtx::iterator>;
 
    template <typename Context>
    auto scan(int_and_double& val, Context& ctx) const -> scan_expected<typename Context::iterator>;

};
\endcode

\section m2-scan_usertype scn::scan_usertype removed

In v1, `scn::scan_usertype` could be used to make scanning values of custom types easier.
This helper function was necessary, because the scanning context had complex logic concerning the source range.
In v2, this has been removed, because of the new tuple-return API,
and because the context no longer deals with complicated ranges.

\code{.cpp}
// v1
template <typename Context>
error scan(int_and_double& val, Context& ctx) const {
    return scn::scan_usertype(ctx.range(), "[{}, {}]", val.i, val.d);
}

// v2
template <typename Context>
auto scan(int_and_double& val, Context& ctx) const
    -> expected<typename Context::iterator> {
    auto result = scn::scan<int, double>(ctx.range(), "[{}, {}]");
    if (!result) {
        return unexpected(result.error());
    }

    std::tie(val.i, val.d) = result->values();
    return result->begin();
}
\endcode

\section m2-parser scn::*_parser removed

In v1, there were helper base classes for creating `scanner::parse`,
including `scn::empty_parser` and `scn::common_parser`.

In v2, these are removed. Create your own `parse` member functions, or reuse already existing `scanner`s.

\section m2-istream-operator Including <scn/istream.h> no longer enables custom scanning for types with an operator>> by default

In v1, just by including `<scn/istream.h>`, any type with an `operator>>` would be automatically `scn::scan`able.

In v2, you'll need to explicitly opt in to this behavior for your own types,
by creating a `scn::scanner`,
and inheriting from the `scn::basic_istream_scanner<CharT>` class template.

This is done to avoid potentially surprising behavior.

\code{.cpp}
#include <scn/istream.h>

struct mytype {
    int i, j;

    friend std::istream& operator>>(std::istream& is, const mytype& val) {
        return is >> val.i >> val.j;
    }
};

// v1 would work out of the box:
mytype val{};
auto result = scn::scan("123 456", "{}", val);

// v2 requires a scanner definition
template <typename CharT>
struct scn::scanner<mytype, CharT> : public scn::basic_istream_scanner<CharT> {};

auto result = scn::scan<mytype>("123 456", "{}");
\endcode

\section m2-scan_localized scn::scan_localized renamed to scn::scan

In v1, to use a `std::locale` in scanning, the function `scn::scan_localized` had to be used.

In v2, this function is part of the `scn::scan` overload set.

\code{.cpp}
// v1
int i;
auto ret = scn::scan_localized(locale, "42", "{}", i);

// v2;
auto result = scn::scan<int>(locale, "42", "{}");
\endcode

\section m2-lists List operations removed

In v1, there were `scn::scan_list` and `scn::scan_list_ex`,
that could be used to scan multiple values of the same type into a container.

In v2, these have been removed.
Either scan each value manually, or use the new (experimental) range scanning functionality, in `<scn/ranges.h>`.

\code{.cpp}
// v1
std::vector<int> vec{};
auto result = scn::scan_list("123 456 abc", vec);
// vec == [123, 456]
// result.range() == " abc"
// NOTE: result.error() == invalid_scanner_value (because of "abc")

// v2
std::vector<int> vec{};
auto input = scn::ranges::subrange{std::string_view{"123 456 abc"}};

while (auto result = scn::scan<int>(input, "{}")) {
    vec.push_back(result->value());
    input = result->range();
}
// vec == [123, 456]
// input == " abc"

// or, if the source range is in the correct format
// (how std::format would output it)
auto result = scn::scan<std::vector<int>>("[123, 456]", "{}");
// result->value() == [123, 456]
\endcode

\section m2-ignore-getline scn::ignore and scn::getline removed

In v2, `scn::ignore` can be replaced with simple range operations, like `scn::ranges::views::drop_while`.

`scn::getline` can be replaced with `scn::scan<std::string>(..., "{:[^\n]}")`.

\section m2-encoding Encoding is always Unicode

In v1, when scanning in non-localized mode, the input was assumed to be Unicode
(UTF-8, UTF-16, or UTF-32, based on the character type),
and whatever the locale specified in localized mode.
Because of the limited character encoding handling support provided by the standard library, this was buggy.

In v2, all inputs are assumed to be Unicode, despite what has been set in a possibly supplied locale.
