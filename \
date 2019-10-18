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

#ifndef SCN_SCN_H
#define SCN_SCN_H

#include "detail/scan.h"

/**
 * \mainpage
 *
 * `scnlib` is a modern C++ library for scanning values.
 * Think of it as more C++-y `scanf`, or the inverse of
 * <a href="https://fmt.dev">fmtlib</a>.
 *
 * The repository lives on
 * <a href="https://github.com/eliaskosunen/scnlib">GitHub</a>.
 * More information is available on the project's home page,
 * at https://scnlib.dev
 *
 * The library is open source,
 * licensed under the Apache License, version 2.0.\n
 * Copyright (c) 2017-2019 Elias Kosunen\n
 * For further details, see the LICENSE file.
 *
 * \subpage tutorial\n
 * \subpage cmake\n
 * \subpage rationale\n
 *
 * \par API reference
 * \ref scanning\n
 * \ref scanning_operations\n
 * \ref convenience_scan_types\n
 * \ref scan_erase\n
 * \ref scan_low\n
 */

/**
 * \page tutorial Tutorial
 *
 * \section basics Basics
 *
 * The most basic operation is reading from `stdin`, which can be achieved with
 * `scn::input`. The function takes a format string as its first parameter,
 * which gives the library instructions on how to read the values. Followed by
 * that are references to the values to read.
 *
 * \code{.cpp}
 * int i;
 * // Reads an int from stdin
 * scn::input("{}", i);
 * // Equivalent to:
 * //   std::cin >> i;
 * //   scanf("%d", &i);
 * \endcode
 *
 * In this case the format string is `"{}"`.
 * The syntax is familiar from fmtlib, or from Python, where it originated.
 * This format string tells `scn::input` to read a single value with default
 * options.
 *
 * Notice, how you don't have to pass any type information with the format
 * string, like you have do with `scanf`. This information is preserved through
 * the usage of variadic templates, and gives the library stronger type safety.
 *
 * You can read multiple values with a single call to `scn::input`:
 *
 * \code{.cpp}
 * int i;
 * double d;
 * scn::input("{} {}", i, d);
 * // Equivalent to:
 * //   std::cin >> i >> d;
 * //   scanf("%d %lf", &i, &d);
 * \endcode
 *
 * The preceding snippet reads an integer, followed by whitespace (any
 * combination of spaces, newlines, tabs, what have you) that gets discarded,
 * and a floating-point value.
 *
 * To make common usage easier, `scnlib` also provides `scn::prompt`.
 * It is otherwise equivalent to `scn::input`, but it takes a string as its
 * first argument, that it prints to `stdout`. This can be used to give the user
 * instructions on what you're expecting of their input.
 *
 * \code{.cpp}
 * int i;
 * // Prints "Gimme integer pls " and reads an int
 * scn::prompt("Gimme integer pls ", "{}", i);
 * // Equivalent to:
 * //   std::cout << "Gimme integer pls ";
 * //   std::cin >> i;
 * // or
 * //   fputs("Gimme integer pls ", stdout);
 * //   scanf("%d", &i);
 * \endcode
 *
 * \section ranges Ranges
 *
 * We can, of course, read from other sources than `stdin`.
 * In fact, with scnlib, we can read from any `range`, as long as it fulfills
 * certain requirements. If you're not familiar with C++20 ranges, don't worry;
 * conceptually, they're quite simple. A range is simply something that one can
 * call `begin()` and `end()` on. For example, a `std::string` or a
 * `std::vector` are ranges.
 *
 * The library can't work with every range, though.
 * Most importantly, it needs to be a `view`, meaning that it doesn't own its
 * elements, and is fast to copy. Examples of `view`s are `std::string_view` and
 * `std::span`.
 *
 * This range can then be passed as the first parameter to `scn::scan`:
 *
 * \code{.cpp}
 * int i;
 * // A string literal is something one _can_ pass to scn::scan
 * scn::scan("42", "{}", i);
 * // i == 42
 * \endcode
 *
 * `scn::scan` takes the input by forwarding reference.
 * This means, that if it's given a modifiable lvalue (`T&`),
 * the same variable can easily be used in multiple calls to `scn::scan`.
 *
 * \code{.cpp}
 * auto input = scn::string_view("123 foo");
 *
 * int i;
 * scn::scan(input, "{}", i);
 * // i == 123
 * // input == " foo"
 *
 * std::string str;
 * scn::scan(input, "{}", str);
 * // str == "foo"
 * // input is empty
 * \endcode
 *
 * A convenience function, `scn::make_view`, is provided,
 * which makes converting a range to an appropriate `view` easier.
 *
 * \code{.cpp}
 * std::string str = ...;
 * auto view = scn::make_view(str);
 *
 * scn::scan(view, ...);
 * \endcode
 *
 * Note, that `const char*` is _not_ a range, but `const char(&)[N]` is.
 * This has the unfortunate consequence that this works:
 *
 * \code{.cpp}
 * // "foo" is a const char(&)[4]
 * scn::scan("foo", ...);
 * \endcode
 *
 * But this doesn't:
 *
 * \code{.cpp}
 * auto str = "foo";
 * // str is a const char*
 * scn::scan(str, ...);
 * // Error will be along the lines of
 * // "Cannot call begin on a const char*"
 * \endcode
 *
 * This is caused by the way string literals and array decay work in the
 * language.
 *
 * This can be worked around with `scn::make_view`:
 *
 * \code{.cpp}
 * auto str = scn::make_view("foo");
 * // str is a scn::string_view
 * scn::scan(str, ...);
 * \endcode
 *
 * Reading from files is also supported, with a range wrapping a `FILE*`.
 *
 * \code{.cpp}
 * auto f = std::fopen(...);
 * // Non-owning wrapper around a FILE*
 * auto file = scn::file(f);
 * scn::scan(file, ...);
 * // scn::file does _not_ sync with the underlying FILE* by default
 * // call .sync() if you wish to use scnlib in conjunction with <cstdio>
 * file.sync();
 * // scn::file doesn't take ownership, and doesn't close
 * std::fclose(f);
 * \endcode
 *
 * `scn::cstdin()` returns a `scn::file` pointing to `stdin`.
 *
 * \section tuple Alternative tuple-based API
 *
 * By including `<scn/tuple_return.h>` an alternative API becomes available,
 * returning a `std::tuple` instead of taking references.
 *
 * \code{.cpp}
 * // Use structured bindings with C++17
 * auto [result, i] = scn::scan_tuple<int>(range, "{}");
 * // result is a `scan_result`, similar to the return value of `scn::scan`
 * // Error handling is further touched upon later
 * // i is an `int`, scanned from the range
 * \endcode
 *
 * \section strings Strings and getline
 *
 * Reading a `std::string` with `scnlib` works the same way it does with
 * `operator>>` and `<iostream>`: the input range is read until a whitespace
 * character or EOF is found. This effectively means, that scanning a
 * `std::string` reads a word at a time.
 *
 * \code{.cpp}
 * auto source = scn::make_view("Hello world!");
 *
 * std::string word;
 * scn::scan(source, "{}", word);
 * // word == "Hello"
 *
 * scn::scan(source, "{}", word);
 * // word == "world!"
 * \endcode
 *
 * If reading word-by-word isn't what you're looking for, you can use
 * `scn::getline`. It works pretty much the same way as `std::getline` does for
 * `std::string`s.
 *
 * \code{.cpp}
 * // Using the source range from the earlier example
 * std::string word;
 * // A third parameter could be given, denoting the delimeter
 * // Defaults to '\n'
 * scn::getline(source, word);
 * // word == "Hello world!"
 * // The delimeter is not included in the output
 * \endcode
 *
 * \section error Error handling
 *
 * `scnlib` does not use exceptions for error handling.
 * Instead, `scn::scan` and others return a
 * `scn::scan_result`, which is an object that contains:
 *  - an integer, telling the number of arguments successfully read
 *  - a range, denoting the unused part of the input range
 *  - an `scn::error` object
 *
 * \code{.cpp}
 * // successful read:
 * int i{}
 * auto ret = scn::scan("42 leftovers", "{}", i);
 * // ret == true
 * // ret.value() == 1
 * // ret.range() == " leftovers"
 * // ret.error() == true
 *
 * // failing read:
 * int i{};
 * auto ret = scn::scan("foo", "{}", i);
 * // ret == false
 * // ret.value() == 0
 * // ret.range() == "foo"
 * // ret.error() == false
 * \endcode
 *
 * The `scn::error` object can be examined further. It contains an error code
 * `scn::error::code`, accessible with member function `code()` and a message,
 * that can be get with `msg()`.
 *
 * \code{.cpp}
 * auto ret = scn::scan(range, "{}", value);
 * if (!ret) {
 *     std::cout << "Read failed with message: '" << ret.error().msg() << "'\n";
 * }
 * \endcode
 *
 * Please note, that EOF is also an error, with error code
 * `scn::error::end_of_range`.
 *
 * If the error is of such quality that it cannot be recovered from, the range
 * becomes <i>bad</i>, and the member function `is_recoverable()` of
 * `scn::error` will return `false`. This means, that the range is unusable and
 * in an indeterminate state.
 *
 * See `scn::error` for more details about the error codes.
 *
 * \subsection error_guarantees Error guarantees
 * Should the reading of any of the arguments fail, and the range is not bad,
 * the state of the range will be reset to what it was before the reading of
 * said argument. Also, the argument will not be written to.
 *
 * \code{.cpp}
 * int i{}, j{};
 * // "foo" cannot be read to an integer, so this will fail
 * auto ret = scn::scan("123 foo", "{} {}", i, j);
 * assert(!ret);
 * // First read succeeded
 * assert(ret.value() == 1);
 * assert(i == 123);
 * // Second read failed, value was not touched
 * assert(j == 0);
 * assert(ret.error().code() == scn::error::invalid_scanned_value);
 * // std::string so operator== works
 * assert(ret.range() == std::string{" foo"});
 *
 * // The range now contains "foo",
 * // as it was reset to the state preceding the read of j
 * std::string s{};
 * ret = scn::scan(ret.range(), "{}", s);
 * // This succeeds
 * assert(ret);
 * assert(ret.value() == 1);
 * assert(s == "foo");
 * assert(ret.range().empty() == true);
 * \endcode
 *
 * \subsection error_exceptions Exceptions
 * No exceptions will ever be thrown by `scnlib` functions (save for a
 * `std::bad_alloc`, but that's probably your fault).
 * Should any user-defined operations, like `operator*` on an iterator, or
 * `operator>>`, throw, the behavior is undefined.
 *
 * The library can be compiled with `-fno-exceptions` and `-fno-rtti`.
 *
 * \section scan_value
 *
 * If you only wish to scan a single value with all default options, you can
 * save some cycles and use `scn::scan_value`. Instead of taking its argument by
 * reference, it returns the read value. It is functionally equivalent to
 * `scn::scan(range, scn::default_tag, value)`.
 *
 * \code{.cpp}
 * auto ret = scn::scan_value<int>("42 leftovers");
 * // ret == true
 * // ret.value() == 42
 * // ret.range() == " leftovers"
 * \endcode
 *
 * \section wide Wide ranges
 *
 * Ranges can also be wide (terminology borrowed from iostreams), meaning that
 * their character type is `wchar_t` instead of `char`. This has some usage
 * implications.
 *
 * The format string must be wide:
 *
 * \code{.cpp}
 * scn::scan(range, L"{}", value);
 * \endcode
 *
 * `char`s and `std::string`s cannot be read from a wide range, but `wchar_t`s
 * and `std::wstring`s can.
 *
 * \code{.cpp}
 * std::wstring word;
 * scn::scan(range, L"{}", word);
 * \endcode
 *
 * Ranges with character types other that `char` and `wchar_t` are not
 * supported, due to lacking support for them in the standard library.
 * Converting between character types is out-of-score for this library.
 *
 * \subsection unicode Encoding and Unicode
 * Because of the rather lackluster Unicode support of the standard library,
 * this library doesn't have any significant Unicode support either.
 *
 * Narrow ranges are expected to be ASCII encoded, and using multibyte
 * encodings (like UTF-8) with them is probably going to cause problems (blame
 * `std::locale`). If you need some sort of Unicode support, your best bet is
 * going to be wide ranges, encoded in the way your platform expects (UTF-32 in
 * POSIX, the thing resembling UCS-2 in Windows)
 *
 * \section format_string Format string
 *
 * Every value to be scanned from the input range is marked with a pair of
 * curly braces `"{}"` in the format string. Inside these braces, additional
 * options can be specified. The syntax is not dissimilar from the one found in
 * fmtlib.
 *
 * The information inside the braces consist of two parts: the index and the
 * scanning options, separated by a colon <tt>':'</tt>.
 *
 * The index part can either be empty, or be an integer.
 * If the index is specified for one of the arguments, it must be set for all of
 * them. The index tells the library which argument the braces correspond to.
 *
 * \code{.cpp}
 * int i;
 * std::string str;
 * scn::scan(range, "{1} {0}", i, str);
 * // Reads from the range in the order of:
 * //   string, whitespace, integer
 * // That's because the first format string braces have index '1', pointing to
 * // the second passed argument (indices start from 0), which is a string
 * \endcode
 *
 * After the index comes a colon and the scanning options.
 * The colon only has to be there if any scanning options are specified.
 *
 * For `span`s, there are no supported scanning options.
 *
 * \par Integral types
 * There are localization specifiers:
 *  - `n`: Use thousands separator from the given locale
 *  - `l`: Accept characters specified as digits by the given locale. Implies
 * `n`
 *  - (default): Use `,` as thousands separator and `[0-9]` as digits
 *
 * \par
 * And base specifiers:
 *  - `d`: Decimal (base-10)
 *  - `x`: Hexadecimal (base-16)
 *  - `o`: Octal (base-8)
 *  - `b..` Custom base; `b` followed by one or two digits
 *     (e.g. `b2` for binary). Base must be between 2 and 36, inclusive
 *  - (default): Detect base. `0x`/`0X` prefix for hexadecimal,
 *    `0` prefix for octal, decimal by default
 *  - `i`: Detect base. Argument must be signed
 *  - `u`: Detect base. Argument must be unsigned
 *
 * \par
 * And other options:
 *  - `'`: Accept thousands separator characters,
 *         as specified by the given locale (only with `custom`-scanning method)
 *  - (default): Thousands separator characters aren't accepter
 *
 * \par
 * These specifiers can be given in any order, with up to one from each
 * category.
 *
 * \par Floating-point types
 * First, there's a localization specifier:
 *  - `n`: Use decimal and thousands separator from the given locale
 *  - (default): Use `.` as decimal point and `,` as thousands separator
 *
 * \par
 * After that, an optional `a`, `A`, `e`, `E`, `f`, `F`, `g` or `G` can be
 * given, which has no effect.
 *
 * \par `bool`
 * First, there are a number of specifiers that can be given, in any order:
 *  - `a`: Accept only `true` or `false`
 *  - `n`: Accept only `0` or `1`
 *  - `l`: Implies `a`. Expect boolean text values as specified as such by the
 *     given locale
 *  - (default): Accept `0`, `1`, `true`, and `false`, equivalent to `an`
 *
 * \par
 * After that, an optional `b` can be given, which has no effect.
 *
 * \par Strings
 * Only supported option is `s`, which has no effect
 *
 * \par Characters
 * Only supported option is `c`, which has no effect
 *
 * \par Whitespace
 * Any amount of whitespace in the format string tells the library to skip until
 * the next non-whitespace character is found from the range. Not finding any
 * whitespace from the range is not an error.
 *
 * \par Literal characters
 * To scan literal characters and immediately discard them, just write the
 * characters in the format string. `scanf`-like `[]`-wildcard is not supported.
 * To read literal `{` or `}`, write `{{` or `}}`, respectively.
 *
 * \par
 * \code{.cpp}
 * std::string bar;
 * scn::scan("foobar", "foo{}", bar);
 * // bar == "bar"
 * \endcode
 *
 * \par Default format string
 * If you wish to not pass any custom parsing options, you should probably pass
 * a `scn::default_tag` instead. This will increase performance, as an useless
 * format string doesn't need to be parsed.
 *
 * \par
 * \code{.cpp}
 * scn::scan(range, scn::default_tag, value);
 * // Equivalent to:
 * // scn::scan(range, "{}", value);
 * \endcode
 *
 * \section locale Localization
 *
 * To scan localized input, a `std::locale` can be passed as the first argument
 * to `scn::scan_localized`.
 *
 * \code{.cpp}
 * auto loc = std::locale("fi_FI");
 *
 * int a, b;
 * scn::scan_localized(
 *     loc,
 *     range, "{} {:n}", a, b);
 * \endcode
 *
 * Only reading of `b` will be localized, as it has `{:n}` as its format string.
 *
 * \section reading Semantics of scanning a value
 *
 * In the beginning, with every `scn::scan` (or similar) call, the
 * library calls `begin()` on the range, getting an iterator. This iterator is
 * advanced until a non-whitespace character is found.
 *
 * After that, the format string is scanned character-by-character, until an
 * unescaped \c '{' is found, after which the part after the \c '{' is parsed,
 * until a <tt>':'</tt> or \c '}' is found. If the parser finds an argument id,
 * the argument with that id is fetched from the argument list, otherwise the
 * next argument is used.
 *
 * The `parse()` member function of the appropriate `scn::scanner`
 * specialization is called, which parses the parsing options-part of the format
 * string argument, setting the member variables of the `scn::scanner`
 * specialization to their appropriate values.
 *
 * After that, the `scan()` member function is called. It reads the range,
 * starting from the aforementioned iterator, into a buffer until the next
 * whitespace character is found (except for `char`/`wchar_t`: just a single
 * character is read; and for `span`: `span.size()` characters are read). That
 * buffer is then parsed with the appropriate algorithm (plain copy for
 * `string`s, the method determined by the `options` object for ints and
 * floats).
 *
 * If some of the characters in the buffer were not used, these characters are
 * put back to the range, meaning that `operator--` is called on the iterator.
 *
 * Because how the range is read until a whitespace character, and how the
 * unused part of the buffer is simply put back to the range, some interesting
 * situations may arise. Please note, that the following behavior is consistent
 * with both `scanf` and `<iostream>`.
 *
 * \code{.cpp}
 * char c;
 * std::string str;
 *
 * // No whitespace character after first {}, no range whitespace is skipped
 * scn::scan("abc", "{}{}", c, str);
 * // c == 'a'
 * // str == "bc"
 *
 * // Not finding whitespace to skip from the range when whitespace is found in
 * // the format string isn't an error
 * scn::scan("abc", "{} {}", c, str);
 * // c == 'a'
 * // str == "bc"
 *
 * // Because there are no non-whitespace characters between 'a' and the next
 * // whitespace character ' ', `str` is empty
 * scn::scan("a bc", "{}{}", c, str);
 * // c == 'a'
 * // str == ""
 *
 * // Nothing surprising
 * scn::scan("a bc", "{} {}", c, str);
 * // c == 'a'
 * // str == "bc"
 * \endcode
 *
 * Using `scn::default_tag` is equivalent to using `"{}"` in the format string
 * as many times as there are arguments, separated by whitespace.
 *
 * \code{.cpp}
 * scn::scan(range, scn::default_tag, a, b);
 * // Equivalent to:
 * // scn::scan(range, "{} {}", a, b);
 * \endcode
 *
 * \section ignore ignore
 *
 * `scnlib` has various functions for skipping characters from a range.
 *
 * `scn::ignore_until(range, ch)` will skip until `ch` is read.
 *
 * `scn::ignore_n_until(range, n, ch)` will skip until either `n` characters
 * have been skipped or `ch` is read.
 *
 * \section user_types User types
 *
 * To make your own types scannable with `scnlib`, you can specialize the struct
 * template `scn::scanner`.
 *
 * \code{.cpp}
 * struct my_type {
 *    int i{};
 *    double d{};
 * };
 *
 * template <typename Char>
 * struct scn::scanner<Char, my_type>
 *    : public scn::empty_parser<Char> {
 *    template <typename Context>
 *    error scan(my_type& val, Context& c) {
 *        return scn::scan(c.range(), "[{}, {}]", val.i, val.d);
 *    }
 * };
 *
 * // Input: "[123, 4.56]"
 * // ->
 * //   my_type.i == 123
 * //   my_type.d == 4.56
 * \endcode
 *
 * Inheriting from `scn::empty_parser` means only an empty format string `"{}"`
 * is accepted. You can also implement a `parse()` method, or inherit from a
 * `scn::scanner` for another type (like `scn::scanner<Char, int>`) to get
 * access to additional options.
 *
 * \section temp Scanning temporaries
 *
 * `scnlib` provides a helper type for scanning into a temporary value:
 * `scn::temporary`. which can be created with the helper function `scn::temp`.
 * This is useful, for example, for scanning a `scn::span`.
 *
 * \code{.cpp}
 * // Doesn't work, because arguments must be lvalue references
 * scn::scan(range, "{}", scn::make_span(...));
 *
 * // Workaround
 * auto span = scn::make_span(...);
 * scn::scan(range, "{}", span);
 *
 * // Using scn::temporary
 * // Note the () at the end
 * scn::scan(range, "{}", scn::temp(scn::make_span(...))());
 * \endcode
 *
 * \section scanf scanf-like format strings
 *
 * With `scn::scanf`, a `scanf`-like format string syntax can be used, instead.
 * `scn::ranges::scanf` is also available. The syntax is not 100% compatible
 * with C `scanf`, as it uses the exact same options as the regular format
 * string syntax. The following snippet demonstrates the syntax.
 *
 * \code{.cpp}
 * int i;
 * double d;
 * std::string s;
 * scn::scanf(range, "%i %f %s", i, d, s);
 * // How C scanf would do it:
 * //   scanf(range, "%i %lf", &i, &d);
 * //   reading a dynamic-length string is not possible with scanf
 * // How scn::scan would do it:
 * //   scn::scan(range, "{} {} {}", i, d, s);
 * //   or to be more explicit:
 * //   scn::scan(range, "{:i} {:f} {:s}", i, d, s);
 * \endcode
 *
 * Notice, how the options map exactly to the ones used with `scn::scan`: `%d ->
 * {:d}`, `%f -> {:f}` and `%s -> {:s}`; and how the syntax is not fully
 * compatible with C `scanf`: "%f != %lf", `scanf` doesn't support
 * dynamic-length strings.
 *
 * To read literal a `%`-character and immediately discard it, write `%%` (`{{`
 * and `}}` with default format string syntax).
 */

/**
 * \page cmake CMake usage
 *
 * Using `scnlib` with CMake is pretty easy. Just import it in the way of your
 * liking (`find_package`, `add_subdirectory` etc) and add `scn::scn` (or
 * `scn::scn-header-only`) to your `target_link_libraries`.
 *
 * \section cmake-config CMake configuration options
 *
 * These default to `OFF`, unless `scnlib` is built as a standalone project.
 *
 *  * `SCN_TESTS`: Build tests
 *  * `SCN_EXAMPLES`: Build examples
 *  * `SCN_BENCHMARKS`: Build benchmarks
 *  * `SCN_DOCS`: Build docs
 *  * `SCN_INSTALL` Generate `install` target
 *  * `SCN_PEDANTIC`: Enable stricter warning levels
 *
 * These default to `OFF`, but can be turned on if you want to:
 *
 *  * `SCN_USE_NATIVE_ARCH`: Add `-march=native` to build flags
 *     (gcc or clang only). Useful for increasing performance,
 *     but makes your binary non-portable.
 *  * `SCN_USE_ASAN`, `SCN_USE_UBSAN`, `SCN_USE_MSAN`:
 *     Enable sanitizers, clang only
 *
 * These default to `ON`:
 *
 *  * `SCN_USE_EXCEPTIONS`, `SCN_USE_RTTI`: self-explanatory
 *
 * These default to `OFF`, and should only be turned on if necessary:
 *
 *  * `SCN_WERROR`: Stops compilation on compiler warnings
 *  * `SCN_USE_32BIT`: Compile as 32-bit (gcc or clang only)
 *  * `SCN_COVERAGE`: Generate code coverage report
 *  * `SCN_BLOAT`: Generate bloat test target
 *  * `SCN_BUILD_FUZZING`: Build fuzzer
 */

/**
 * \page rationale Rationale
 *
 * \section view Why take just views? Why not every possible range?
 *
 * First off, it's not possible to take every `range`; `operator--` is
 * required for error recovery, so at least `bidirectional_range` is needed.
 *
 * `view`s have clearer lifetime semantics, and make it more difficult to write
 * less performant code.
 *
 * \code{.cpp}
 * std::string str = "verylongstring";
 * auto ret = scn::scan(str, ...);
 * // str would have to be reallocated and its contents moved
 * \endcode
 *
 * \section return Why take arguments by reference?
 *
 * <a href="https://github.com/eliaskosunen/scnlib/issues/2">
 * Relevant GitHub issue</a>
 *
 * Another frequent complaint is how the library requires default-constructing
 * your arguments, and then passing them by reference.
 * A proposed alternative is returning the arguments as a tuple, and then
 * unpacking them at call site.
 *
 * This is covered pretty well by the above GitHub issue, but to summarize:
 *   - `std::tuple` has measurable overhead (~5% slowdown)
 *   - it still would require your arguments to be default-constructible
 *
 * To elaborate on the second bullet point, consider this example:
 *
 * \code{.cpp}
 * auto [result, i, str] =
 *     scn::scan_tuple<int, non_default_constructible_string>(
 *         range, scn::default_tag);
 * \endcode
 *
 * Now, consider what would happen if an error occurs during scanning the
 * integer. The function would need to return, but what to do with the string?
 * It <i>must</i> be default-constructed (`std::tuple` doesn't allow
 * unconstructed members).
 *
 * Would it be more convenient, especially with C++17 structured bindings?
 * One could argue that, and that's why an alternative API, returning a `tuple`,
 * is available, in the header `<scn/tuple_return.h>`.
 * The rationale of putting it in a separate header is to avoid pulling in the
 * entirety of very heavy standard headers `<tuple>` and `<functional>`.
 *
 * \section vscan What's with all the vscan, basic_args and arg_store stuff?
 *
 * This approach is borrowed (*cough* stolen *cough*) from fmtlib, for the same
 * reason it's in there as well. Consider this peace of code:
 *
 * \code{.cpp}
 * int i;
 * std::string str;
 *
 * scn::scan(range, scn::default_tag, i, str);
 * scn::scan(range, scn::default_tag, str, i);
 * \endcode
 *
 * If the arguments were not type-erased, almost all of the internals would have
 * to be instantiated for every given combination of argument types.
 */

/**
 * \page range Input range
 *
 * This page is about the types of ranges that can be passed as inputs to
 * various functions within scnlib.
 *
 * Fundamentally, a range is something that has a beginning and an end. Examples
 * of ranges are string literals, arrays, and `std::vector`s. All of these can
 * be passed to `std::begin` and `std::end`, which then return an iterator to
 * the range. This notion of ranges was standardized in C++20 with the Ranges
 * TS. This library provides barebone support of this functionality.
 *
 * \section range_req Range requirements
 *
 * The range must be:
 *   - bidirectional
 *   - a view
 *   - reconstructible
 *
 * Using C++20 concepts:
 *
 * \code{.cpp}
 * template <typename Range>
 * concept scannable_range =
 *     std::ranges::bidirectional_range<Range> &&
 *     std::ranges::view<Range> &&
 *     std::ranges::pair-reconstructible-range<Range>;
 * \endcode
 *
 * \par Bidirectional?
 * A bidirectional range is a range, the iterator type of which is
 * bidirectional: <a
 * href="http://eel.is/c++draft/iterator.concepts#iterator.concept.bidir">C++
 * standard</a>. Bidirectionality means, that the iterator can be moved both
 * forwards: `++it` and backwards `--it`.
 *
 * \par
 * Note, that both random-access and contiguous ranges are refinements of
 * bidirectional ranges, and can be passed to the library. In fact, the library
 * implements various optimizations for contiguous ranges.
 *
 * \par View?
 * A view is a range that is cheap to copy and doesn't own its elements: <a
 * href="http://eel.is/c++draft/range.view">C++ standard</a>.
 *
 * \par
 * Basically no container within the standard library is a view. This means,
 * that for example a `std::string` can't be passed to scnlib. This can be
 * worked around with `scn::make_view`, which returns a `string_view` for a
 * `std::string`, which is a view.
 *
 * \par
 * \code{.cpp}
 * std::string str = ...;
 * scn::scan(scn::make_view(str), ...);
 * \endcode
 *
 * \par Reconstructible?
 * A reconstructible range is a range that can be constructed from a begin
 * iterator and an end iterator (sentinel): <a
 * href="http://wg21.link/p1664">P1664</a>.
 *
 * \section range_char Character type
 *
 * The range has an associated character type.
 * This character type can be either `char` or `wchar_t`.
 * The character type is determined by the result of `operator*` of the range
 * iterator. If dereferencing the iterator returns
 *   - `char` or `wchar_t`: the character type is `char` or `wchar_t`
 *   - `expected<char>` or `expected<wchar_t>`: the character type is `char` or
 *     `wchar_t`
 */

#endif  // SCN_SCN_H

