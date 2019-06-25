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
#include "detail/types.h"

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
 * at https://scnlib.eliaskosunen.com
 *
 * The library is open source,
 * licensed under the Apache License, version 2.0.\n
 * Copyright (c) 2017-2019 Elias Kosunen\n
 * For further details, see the LICENSE file.
 *
 * \section tutorial Tutorial
 *
 * \subsection basics Basics
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
 * \subsection streams Streams
 *
 * To read from anywhere else but `stdin`, we need streams.
 *
 * Streams are objects that and hold a source
 * that characters can be read from.
 *
 * A stream can be created with `scn::make_stream`, which is passed a source.
 * In the following example, we are creating a stream holding a string literal.
 *
 * \code{.cpp}
 * auto source = "42 foo";
 * auto stream = scn::make_stream(source);
 * \endcode
 *
 * Streams generally do _not_ take ownership of the sources passed to them, but
 * take them by reference. You'll need to ensure that the sources outlive your
 * streams.
 *
 * Streams can then be passed to `scn::scan`, which then reads from them.
 *
 * \code{.cpp}
 * int i;
 * scn::scan(stream, "{}", i);
 * // i == 42
 *
 * std::string str;
 * scn::scan(stream, "{}", str);
 * // str == "foo"
 * \endcode
 *
 * Streams can be created from multiple different sources:
 *
 * \code{.cpp}
 * // String literals
 * auto string_stream = scn::make_stream("42");
 *
 * // Containers
 * std::vector<char> container_source{'1', '2', '3'};
 * auto container_stream = scn::make_stream(container_source);
 *
 * // Static buffers
 * scn::span<char> span_source(container_source.data(),
 *                             container_source.size());
 * auto span_stream = scn::make_stream(span_source);
 *
 * // Pairs of iterators
 * auto iter_stream = scn::make_stream(container_source.begin(),
 *                                     container_source.begin() + 2);
 *
 * // C FILEs
 * auto cfile_source = fopen("myfile.txt", "r");
 * // Remember to call fclose yourself
 * auto cfile_stream = scn::make_stream(cfile_source);
 *
 * // `std::istream`s
 * // You need to #include <scn/istream.h> as well for this to work
 * auto cppfile_source = std::ifstream("myfile.txt");
 * auto cppfile_stream = scn::make_stream(cppfile_source);
 *
 * // null stream
 * // always at EOF
 * auto null_stream = scn::make_null_stream<char>();
 * \endcode
 *
 * A notable exceptions to this `make_stream` pattern is the standard input
 * `stdin`. Even though you could, theoretically, do this:
 *
 * \code{.cpp}
 * // DON'T DO THIS
 * auto stdin_stream = scn::make_stream(stdin);
 * auto cin_stream = scn::make_stream(std::cin);
 * \endcode
 *
 * That will lead to synchronization issues between the streams and their
 * underlying sources.
 *
 * You should instead use `scn::input`, `scn::prompt` or `scn::scan` with
 * `scn::cstdin()` as its first parameter:
 *
 * \code{.cpp}
 * scn::scan(scn::cstdin(), ...);
 * \endcode
 *
 * Because all of the streams are defined in the `scn` namespace, you could,
 * theoretically, call virtually every function of the `scnlib` public interface
 * without specifying the namespace `scn::`, instead relying on ADL.
 * I would not recommend that, though.
 *
 * \code{.cpp}
 * auto stream = make_null_stream<char>();
 * // this would work
 * // _please_ don't do this
 * scan(stream, ...);
 * \endcode
 *
 * \subsection tuple Alternative tuple-based API
 *
 * By including `<scn/tuple_return.h>` an alternative API becomes available,
 * returning a `std::tuple` instead of taking references.
 *
 * \code{.cpp}
 * // Use structured bindings with C++17
 * auto [result, i] = scn::scan<int>(stream, "{}");
 * // result is a `scan_result`, similar to the return value of `scn::scan`
 * // Error handling is further touched upon later
 * // i is an `int`, scanned from the stream
 *
 * // `std::tie` for pre-C++17
 * // scn::scan_result is not default-constructible, init to a dummy value of 0
 * scn::scan_result result{0};
 * int i;
 * std::tie(result, i) = scn::scan<int>(stream, "{}");
 * \endcode
 *
 * \subsection strings Strings and getline
 *
 * Reading a `std::string` with `scnlib` works the same way it does with
 * `operator>>` and `<iostream>`: the stream is read until a whitespace
 * character or EOF is found. This effectively means, that scanning a
 * `std::string` reads a word at a time.
 *
 * \code{.cpp}
 * auto stream = scn::make_stream("Hello world!");
 *
 * std::string word;
 * scn::scan(stream, "{}", word);
 * // word == "Hello"
 *
 * scn::scan(stream, "{}", word);
 * // word == "world!"
 * \endcode
 *
 * If reading word-by-word isn't what you're looking for, you can use
 * `scn::getline`. It works pretty much the same way as `std::getline` does for
 * `std::string`s.
 *
 * \code{.cpp}
 * // Using the stream from the earlier example
 * std::string word
 * // A third parameter could be given, denoting the delimeter
 * // Defaults to '\n'
 * scn::getline(stream, word);
 * // word == "Hello world!"
 * // The delimeter is not included in the output
 * \endcode
 *
 * \subsection error Error handling
 *
 * `scnlib` does not use exceptions for error handling.
 * Instead, `scn::scan`, `scn::input`, `scn::prompt` and `scn::vscan` return a
 * `scn::scan_result`, which is an object containing an integer, telling the
 * number of arguments successfully read, and an `scn::error` object.
 *
 * \code{.cpp}
 * // successful read:
 * auto ret = scn::scan(stream, "{}", value);
 * // ret == true
 * // ret.value() == 1
 * // ret.error() == true
 *
 * // failing read:
 * ret = scn::scan(stream, "{}", value);
 * // ret == false
 * // ret.value() == 0
 * // ret.error() == false
 * \endcode
 *
 * Other items of the `scnlib` public interface, like `scn::ignore` or
 * `scn::getline` will only return a `scn::error`.
 *
 * The `scn::error` object can be examined further. It contains an error code
 * `scn::error::code`, accessible with member function `code()` and a message,
 * that can be get with `msg()`.
 *
 * \code{.cpp}
 * auto ret = scn::scan(stream, "{}", value);
 * if (!ret) {
 *     std::cout << "Read failed with message: '" << ret.error().msg() << "'\n";
 * }
 * \endcode
 *
 * Please note, that EOF is also an error, with error code
 * `scn::error::end_of_stream`.
 *
 * If the error is of such quality that it cannot be recovered from, the stream
 * is deemed <i>bad</i>. In this case, the member function `is_recoverable()` of
 * `scn::error` will return `false`, and the member function `bad()` of your
 * stream will return `true`.
 *
 * See `scn::error` for more details about the error codes.
 *
 * \par Error guarantees
 * Should the reading of any of the arguments fail, and the stream is not bad,
 * the state of the stream will be reset to what it was before the reading of
 * said argument. Also, the argument will not be written to.
 *
 * \par
 * \code{.cpp}
 * auto stream = scn::make_stream("123 foo");
 *
 * int i{}, j{};
 * // "foo" cannot be read to an integer, so this will fail
 * auto ret = scn::scan(stream, "{} {}", i, j);
 * assert(!ret);
 * // First read succeeded
 * assert(ret.value() == 1);
 * assert(i == 123);
 * // Second read failed, value was not touched
 * assert(j == 0);
 * assert(ret.error().code() == scn::error::invalid_scanned_value);
 *
 * // The stream now contains "foo",
 * // as it was reset to the state preceding the read of j
 * std::string s{};
 * ret = scn::scan(stream, "{}", s);
 * // This succeeds
 * assert(ret);
 * assert(s == "foo");
 * \endcode
 *
 * \par Exceptions
 * No exceptions will ever be thrown by `scnlib` functions (save for a
 * `std::bad_alloc`, but that's probably your fault).
 * Should any user-defined operations, like `operator>>` throw, the behavior is
 * undefined.
 *
 * \par
 * The library can be compiled with `-fno-exceptions` and `-fno-rtti`, but some
 * of its functionality will be disabled, namely `sto` method for integer and
 * float scanning (we'll talk more about scanning methods later).
 *
 * \subsection get_value
 *
 * If you only wish to scan a single value with all default options, you can
 * save some cycles and use `scn::get_value`. Instead of taking its argument by
 * reference, it returns an `expected<T>`. It is functionally equivalent to
 * `scn::scan(stream, scn::default_tag, value)`.
 *
 * \code{.cpp}
 * auto stream = scn::make_stream("42");
 * auto ret = scn::get_value<int>(stream);
 * // ret == true
 * // ret.value() == 42
 * \endcode
 *
 * \subsection wide Wide streams
 *
 * Streams can also be wide, meaning that their character type is `wchar_t`
 * instead of `char`. This has some usage implications.
 *
 * The format string must be wide:
 *
 * \code{.cpp}
 * scn::scan(stream, L"{}", value);
 * \endcode
 *
 * `char`s and `std::string`s cannot be read from a wide stream, but `wchar_t`s
 * and `std::wstring`s can.
 *
 * \code{.cpp}
 * std::wstring word;
 * scn::scan(stream, L"{}", word);
 * \endcode
 *
 * Wide streams using a `FILE*` as its source must use `make_stream<wchar_t>` or
 * `make_wide_stream for construction:
 *
 * \code{.cpp}
 * auto f = fopen("wide_file.txt", "r");
 * auto stream = scn::make_wide_stream(f);
 * \endcode
 *
 * Streams with character types other that `char` and `wchar_t` are not
 * supported, due to lacking support for them in the standard library.
 * Converting between character types is out-of-score for this library at this
 * time.
 *
 * \par Encoding and Unicode
 * Because of the rather lackluster Unicode support of the standard library,
 * this library doesn't have any significant Unicode support either.
 *
 * \par
 * Narrow streams are expected to be ASCII encoded, and using multibyte
 * encodings (like UTF-8) with them is probably going to cause problems (blame
 * `std::locale`). If you need some sort of Unicode support, your best bet is
 * going to be wide streams, encoded in the way your platform expects (UTF-32 in
 * POSIX, the thing resembling UCS-2 in Windows)
 *
 * \subsection format_string Format string
 *
 * Every value to be scanned from the input stream is marked with a pair of
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
 * scn::scan(stream, "{1} {0}", i, str);
 * // Reads from the stream in the order of:
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
 * the next non-whitespace character is found from the stream. Not finding any
 * whitespace from the stream is not an error.
 *
 * \par Literal characters
 * To scan literal characters and immediately discard them, just write the
 * characters in the format string. `scanf`-like `[]`-wildcard is not supported.
 * To read literal `{` or `}`, write `{{` or `}}`, respectively.
 *
 * \par
 * \code{.cpp}
 * auto stream = scn::make_stream("foobar");
 * std::string bar;
 * scn::scan(stream, "foo{}", bar);
 * // bar == "bar"
 * \endcode{.cpp}
 *
 * \par Default format string
 * If you wish to not pass any custom parsing options, you should probably pass
 * a `scn::default_tag` instead. This will increase performance, as an useless
 * format string doesn't need to be parsed.
 *
 * \par
 * \code{.cpp}
 * scn::scan(stream, scn::default_tag, value);
 * // Equivalent to:
 * // scn::scan(stream, "{}", value);
 * \endcode
 *
 * \subsection options Additional options
 *
 * An `scn::options` object can be passed as the first argument to `scn::scan`
 * or `scn::input`, determining additional options for the scanning operation.
 * A `scn:::options` object cannot be constructed directly, but one can be
 * created with `scn::options::builder`, like so:
 *
 * \code{.cpp}
 * scn::scan(
 *     scn::options::builder{}.make(),
 *     stream, ...);
 * \endcode
 *
 * Redundant specification of additional options should be avoided, as it can
 * have some performance implications.
 *
 * \par Localization
 * A constant reference to a `std::locale` can be passed to
 * `scn::options::builder::locale()` for scanning localized input.
 *
 * \par
 * \code{.cpp}
 * auto loc = std::locale("fi_FI");
 *
 * int a, b;
 * scn::scan(
 *     scn::options::builder{}
 *         .locale(loc)
 *         .make(),
 *     "{} {:n}", a, b);
 * \endcode
 *
 * \par
 * Only reading of `b` will be localized, as it has `{:n}` as its format string.
 *
 * \par Scanning method
 * An enumeration value of `scn::method` can be passed to
 * `scn::options::builder::int_method()` or
 * `scn::options::builder::float_method()`, specifying how a value of each
 * respective type will be scanned.
 *
 * \par
 *   - `strto`: Use `std::strtol`, `std::strtod` etc.
 *   - `sto`: Use `std::stol`, `std::stod` etc.
 *   - `from_chars`: Use `std::from_chars`
 *   - `custom`: Use custom hand-rolled algorithm
 *
 * \par
 * \code{.cpp}
 * scn::scan(
 *     scn::options::builder{}
 *         .int_method(scn::method::strto)
 *         .float_method(scn::method::sto)
 *         .make(), ...);
 * \endcode
 *
 * \par
 * Please note, that:
 *  - `custom` is the default method for integers, and `strto` for floats
 *  - `from_chars` requires a very recent standard library version. Your
 *     implementation may not yet have `std::from_chars` implemented.
 *  - `scn::int_from_chars_if_available()` and
 *    `scn::float_from_chars_if_available()` return `from_chars` if that method
 *     is available for ints and floats, respectively, and the default method
 *     otherwise
 *  - `custom` is a little faster than the other alternatives with the added
 *     caveat of being highly likely to contain bugs
 *  - `custom` at this time only supports integers
 *
 * \subsection reading Semantics of scanning a value
 *
 * In the beginning, with every `scn::scan` and `scn::get_value` call, the
 * stream is skipped until a non-whitespace character is found.
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
 * After that, the `scan()` member function is called. It reads the stream into
 * a buffer until the next whitespace character is found (except for
 * `char`/`wchar_t`: just a single character is read; and for `span`:
 * `span.size()` characters are read). That buffer is then parsed with the
 * appropriate algorithm (plain copy for `string`s, the method determined by the
 * `options` object for ints and floats).
 *
 * If some of the characters in the buffer were not used, these characters are
 * put back to the stream.
 *
 * Because how the stream is read until a whitespace character, and how the
 * unused part of the buffer is simply put back to the stream, some interesting
 * situations may arise. Please note, that the following behavior is consistent
 * with both `scanf` and `<iostream>`.
 *
 * \code{.cpp}
 * auto stream = scn::make_stream("abc");
 * char c;
 * std::string str;
 *
 * // No whitespace character after first {}, no stream whitespace is skipped
 * scn::scan("{}{}", c, str);
 * // c == 'a'
 * // str == "bc"
 *
 * stream = scn::make_stream("abc");
 * // Not finding whitespace to skip from the stream when whitespace is found in
 * // the format string isn't an error
 * scn::scan("{} {}", c, str);
 * // c == 'a'
 * // str == "bc"
 *
 * stream = scn::make_stream("a bc");
 * // Because there are no non-whitespace characters between 'a' and the next
 * // whitespace character ' ', `str` is empty
 * scn::scan("{}{}", c, str);
 * // c == 'a'
 * // str == ""
 *
 * stream = scn::make_stream("a bc");
 * // Nothing surprising
 * scn::scan("{} {}", c, str);
 * // c == 'a'
 * // str == "bc"
 * \endcode
 *
 * Using `scn::default_tag` is equivalent to using `"{}"` in the format string
 * as many times as there are arguments, separated by whitespace.
 *
 * \code{.cpp}
 * scn::scan(stream, scn::default_tag, a, b);
 * // Equivalent to:
 * // scn::scan(stream, "{} {}", a, b);
 * \endcode
 *
 * \subsection ignore ignore
 *
 * `scnlib` has various functions for skipping characters from a stream.
 *
 * `scn::ignore_n(stream, n)` will skip `n` characters.
 *
 * `scn::ignore_until(stream, ch)` will skip until `ch` is read.
 *
 * `scn::ignore_n_until(stream, n, ch)` will skip until either `n` characters
 * have been skipped or `ch` is read.
 *
 * `scn::ignore_all(stream)` will skip to the end of the stream.
 *
 * All of these functions return a `scn::error`.
 *
 * \subsection getchar getchar
 *
 * `scn::getchar(stream)` will read a single character from a stream.
 * The character type will be `char` for narrow streams and `wchar_t` for wide
 * streams. The function will return a `scn::expected<char_type>`.
 *
 * Please note, that the semantics of `scn::getchar` are different from
 * `scn::scan` or `scn::get_value`. `scn::getchar` will return the next
 * character from a stream, whereas `scn::scan` skips leading whitespace and
 * returns the next non-whitespace character.
 *
 * \subsection user_types User types
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
 *       // This interface is likely to change soon(tm)
 *       auto args = make_args<Context>(val.i, val.d);
 *       auto ctx = Context(c.stream(), "[{}, {}]", args);
 *       return vscan(ctx);
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
 * \subsection range Range-based interface
 *
 * `scnlib` also supports a Ranges-based interface, using
 * <a href="https://github.com/ericniebler/range-v3>range-v3</a>.
 * To use the interface, range-v3 must be installed on your system, you must
 * include the header file `<scn/ranges.h>`, and your program must link against
 * the `scn::scn-ranges` CMake target.
 *
 * \code{.cmake}
 * target_link_libraries(your-target scn::scn-ranges)
 * # or scn::scn-ranges-header-only if you'd prefer that
 * \endcode
 *
 * The inferface resides in the namespace `scn::ranges`. Instead of a stream,
 * `scn::ranges::scan` takes a `ranges::ForwardRange` as its first argument.
 *
 * \code{.cpp}
 * #include <scn/ranges.h>
 * // ...
 * auto range = std::string{"Hello"};
 * std::string str{};
 * auto ret = scn::ranges::scan(range, "{}", str);
 * // str == "Hello"
 * // ret == true
 * // ret.value() == 1
 * \endcode
 *
 * The `scn::ranges::scan` return value `scn::ranges::ranges_result` has a
 * member function `iterator()`, which returns an iterator past the last read
 * charaacter of the stream.
 *
 * \code{.cpp}
 * // ret from snippet above
 * assert(ret.iterator() == range.end());
 * \endcode
 *
 * Also, a member function `view()` is available, returning a range that can be
 * used for subsequent `scan` calls.
 *
 * \code{.cpp}
 * auto range = std::string{"Hello world"};
 * std::string str{};
 *
 * auto ret = scn::ranges::scan(range, "{}", str);
 * // str == "Hello"
 * // ret.iterator points to 'w' in "world"
 * // ret.iterator() == range.begin() + str.length() + 1
 *
 * // ret.view() starts from ret.iterator() and ends at range.end()
 * ret = scn::ranges::scan(ret.view(), "{}", str);
 * // str == "world"
 * // ret.iterator() == range.end()
 * \endcode
 *
 * \subsection temp Scanning temporaries
 *
 * `scnlib` provides a helper type for scanning into a temporary value:
 * `scn::temporary`. which can be created with the helper function `scn::temp`.
 * This is useful, for example, for scanning a `scn::span`.
 *
 * \code{.cpp}
 * // Doesn't work, because arguments must be lvalue references
 * scn::scan(stream, "{}", scn::make_span(...));
 *
 * // Workaround
 * auto span = scn::make_span(...);
 * scn::scan(stream, "{}", span);
 *
 * // Using scn::temporary
 * scn::scan(stream, "{}", scn::temp(scn::make_span(...)));
 * \endcode
 *
 * \subsection scanf scanf-like format strings
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
 * scn::scanf(stream, "%i %f %s", i, d, s);
 * // How C scanf would do it:
 * //   scanf(stream, "%i %lf", &i, &d);
 * //   reading a dynamic-length string is not possible with scanf
 * // How scn::scan would do it:
 * //   scn::scan(stream, "{} {} {}", i, d, s);
 * //   or to be more explicit:
 * //   scn::scan(stream, "{:i} {:f} {:s}", i, d, s);
 * \endcode
 *
 * Notice, how the options map exactly to the ones used with `scn::scan`: `%d ->
 * {:d}`, `%f -> {:f}` and `%s -> {:s}`; and how the syntax is not fully
 * compatible with C `scanf`: "%f != %lf", `scanf` doesn't support
 * dynamic-length strings.
 *
 * To read literal a `%`-character and immediately discard it, write `%%` (`{{`
 * and `}}` with default format string syntax).
 *
 * \section cmake CMake usage
 *
 * Using `scnlib` with CMake is pretty easy. Just import it in the way of your
 * liking (`find_package`, `add_subdirectory` etc) and add `scn::scn` (or
 * `scn::scn-header-only`) to your `target_link_libraries`.
 *
 * \subsection cmake-config CMake configuration options
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
 *  * `SCN_PREDEFINE_VSCAN_OVERLOADS`: Increases compile time and generated
 *     library size, but decreases user binary size.
 *     Don't use in header-only mode.
 *  * `SCN_USE_NATIVE_ARCH`: Add `-march=native` to build flags
 *     (gcc or clang only). Useful for increasing performance,
 *     but makes your binary non-portable.
 *  * `SCN_USE_ASAN`, `SCN_USE_UBSAN`, `SCN_USE_MSAN`:
 *     Enable sanitizers, clang only
 *
 * These default to `ON`:
 *
 *  * `SCN_RANGES`: Search for `range-v3`
 *     (or `cmcstl2` if `SCN_USE_CMCSTL2` is `ON`).
 *     Doesn't error if not found, but just doesn't generate the targets.
 *  * `SCN_USE_EXCEPTIONS`, `SCN_USE_RTTI`: self-explanatory
 *
 * These default to `OFF`, and should only be turned on if necessary:
 *
 *  * `SCN_WERROR`: Stops compilation on compiler warnings
 *  * `SCN_USE_CMCSTL2`: Use `cmcstl2` instead of `range-v3`, not supported yet
 *  * `SCN_USE_32BIT`: Compile as 32-bit (gcc or clang only)
 *  * `SCN_COVERAGE`: Generate code coverage report
 *  * `SCN_BLOAT`: Generate bloat test target
 *  * `SCN_BUILD_FUZZING`: Build fuzzer
 *  * `SCN_BUILD_LOCALE_TESTS`: Build localized tests,
 *     needs `en_US.utf8` and `fi_FI.utf8` locales
 *
 *
 * \section rationale Rationale
 *
 * \subsection why_streams Why Streams? Why not just read from a string?
 * See \ref stream_concept and \ref sized_stream_concept.
 *
 * A frequently asked question is, that why does `scn::scan` take a `Stream`
 * as its first argument, and not something simpler, like a string.
 * Let's explore the alternative of passing a string.
 *
 * Let's say we have a file, and we want to read an integer.
 * We'd need to do the reading part ourselves, and then pass the result to
 * `scn`.
 *
 * \code{.cpp}
 * auto f = fopen("file.txt", "r");
 * char ch;
 * std::string str;
 * while((ch = fgetc(f)) != EOF) {
 *     if (isspace(static_cast<unsigned char>(ch)) != 0) {
 *         break;
 *     }
 *     str.push_back(ch);
 * }
 *
 * int i;
 * // hypothetical example
 * scn::scan(str, scn::default_tag, i);
 *
 * fclose(f);
 * \endcode
 *
 * It's really clunky when we have to determine the amount of characters
 * we need to pass to `scn` ourselves. With streams, all that is handled for us:
 *
 * \code{.cpp}
 * auto f = fopen("file.txt", "r");
 * auto stream = scn::make_stream(f);
 *
 * int i;
 * scn::scan(stream, scn::default_tag, i);
 *
 * fclose(f);
 * \endcode
 *
 * Now, what if the string we pass is longer than the scanner requires?
 * Like, for example, our source string could be of arbitrary length, but we
 * only want to read a single word. We could either parse the first word
 * ourselves:
 *
 * \code{.cpp}
 * std::string source = populate();
 * auto it = source.begin();
 * for (; it != source.end(); ++it) {
 *     if (isspace(static_cast<unsigned char>(*it)) != 0) {
 *         break;
 *     }
 * }
 * // contains the first word
 * std::string_view word(source.begin(), it);
 * // contains the rest of the source, used for subsequent parsing
 * std::string_view source_view(it, source.end());
 * \endcode
 *
 * Or we could mess with iterators and string views, passing them to `scn`:
 *
 * \code{.cpp}
 * std::string source = populate();
 * std::string word;
 * auto result = scn::scan(source, scn::default_tag, word);
 * // `word` contains the first word
 * // `source_view` can be used for subsequent parsing
 * std::string_view source_view(result.iterator(), source.end());
 * \endcode
 *
 * Instead of dealing with all of this by hand, it is abstracted away by
 * streams, making your code less error-prone and easier to write:
 *
 * \code{.cpp}
 * // Real example, working code
 * auto stream = scn::make_stream(source);
 * std::string word;
 * scn::scan(stream, scn::default_tag, word);
 * // No need to mess around further,
 * // `stream` can be used for future `scan` calls
 * \endcode
 *
 * Now, if you <i>really</i> want to skip all that, you can use the Ranges-based
 * API, provided inside the `<scn/ranges.h>` header. Please note, that it
 * depends on range-v3, and using it involves a ~10% slowdown and a noticeable
 * increase in generated code size.
 *
 * \code{.cpp}
 * std::string source = populate();
 * std::string word;
 * auto result = scn::ranges::scan(source, scn::default_tag, word);
 * // Use result.view() as the source for subsequent calls
 * \endcode
 *
 * \subsection return Why take arguments by reference?
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
 * auto [result, i, str] = scn::scan<int, non_default_constructible_string>(
 *     stream, scn::default_tag);
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
 * \subsection vscan What's with all the vscan, basic_args and arg_store stuff?
 *
 * This approach is borrowed (*cough* stolen *cough*) from fmtlib, for the same
 * reason it's in there as well. Consider this peace of code:
 *
 * \code{.cpp}
 * int i;
 * std::string str;
 *
 * scn::scan(stream, scn::default_tag, i, str);
 * scn::scan(stream, scn::default_tag, str, i);
 * \endcode
 *
 * If the arguments were not type-erased, almost all of the internals would have
 * to be instantiated for every given combination of argument types.
 */

/**
 * \defgroup concepts Concepts
 */

#endif  // SCN_SCN_H

