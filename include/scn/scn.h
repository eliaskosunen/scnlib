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
 * \section advanced_tutorial Advanced tutorial
 *
 * This section of the tutorial is for more advanced usage.
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
 * First, there's a localization specifier:
 *  - `n`: Use thousands separator from the given locale
 *  - `l`: Accept characters specified as digits by the given locale. Implies
 * `n`
 *  - (default): Use `,` as thousands separator and `[0-9]` as digits
 *
 * \par
 * Then, there's a base specifier
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
 * \section rationale Rationale
 *
 * \subsection why_streams Why Streams? Why not just read from a string?
 * See \ref stream_concept and \ref sized_stream_concept.
 *
 * A frequently asked question is, that why does `scn::scan` take a `Stream`
 * as its first argument, and not something simpler, like a string.
 * Let's explore the alternative, passing a string.
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
 * to be instantiated for every given combination.
 */

/**
 * \defgroup concepts Concepts
 */

#include "detail/scan.h"
#include "detail/types.h"

#endif  // SCN_SCN_H

