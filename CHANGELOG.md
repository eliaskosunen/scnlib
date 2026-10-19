# Changelog

## 5.0.0

_Release 2026-xx-xx_

### Breaking changes

 * `scn::input` and `scn::prompt` now internally use `std::cin` (if iostreams haven't been disabled)
   instead of `stdin`.
 * The return type of `scn::input` and `scn::prompt` now use `scn::stdin_tag_t` as the source type,
   instead of giving access to a `FILE*` directly.
 * The return type of `scn::vinput` is now `scn::vscan_result<scn::stdin_tag_t>`, instead of `scn::scan_expected<void>`.

 * C `FILE`s as sources deprecated. Use `scn::scan_file`s or `std::istream`s instead.
   This is to prevent problems with failures to sync with the underlying `FILE`,
   since `FILE`s are only guaranteed to support a single character's worth of putback.

```cpp
FILE* f = ...;
// Before (deprecated)
scn::scan<...>(f, ...);
// After (scan_file)
scn::scan_file file{f};
scn::scan<...>(file, ...);
// After (std::istream)
std::ifstream strm{...};
scn::scan<...>(strm, ...);
```

 * Previously deprecated `scn::visit_scan_arg` is now removed. Use `scn::basic_scan_arg::visit` instead.
 * The `basic_scan_context` constructor only taking an iterator is now deprecated.
   Instead, use the new constructors that take a range, or both an interator and a sentinel.
 * `scn::invalid_input_range` is renamed to `scn::invalid_source`.
 * `file_marker` is removed as a return type of `scan`
 * The `basic_scan_parse_context` constructor only taking a format string is now deprecated.
   Instead, use the new constructor taking a `scn::source_tag<Source>`, which enables more compile-time checking.
 * `scn::basic_regex_matches` is no longer a `std::vector` of `std::optional<scn::basic_regex_match>`:
   it can't be modified, and doesn't have `data()` or reverse iterators.
   `operator[]`, `at()`, and its iterators return the `std::optional<scn::basic_regex_match>` by value.
   Its iterators are random-access only by `iterator_concept`: their `iterator_category` is `std::input_iterator_tag`.
 * fast_float 6.0.0 or newer is now required (previously 5.0.0), unless it's disabled with `SCN_DISABLE_FAST_FLOAT`
 * A width or precision of zero (`{:0}`, `{:.0}`) or with a leading zero (`{:05}`, `{:.05}`),
   and a `.` without a precision (`{:5.}`) are now format string errors.
   Previously, `{:05}` and `{:.05}` were read as `{:5}` and `{:.5}`, and the others were ignored.
 * A width or precision on a `char`, `wchar_t`, `char32_t`, or `bool` is now a format string error,
   unless the presentation type is an integer one, like `d` or `x` (which `char32_t` doesn't have).
   Previously, it was accepted.
 * Fill and alignment (`{:*<5}`, `{:>5}`) are removed from the format string syntax, and are now format string errors.
   `scn::scan_error::invalid_fill` is removed, so the enumerators after it in `scn::scan_error::code` have new values.
 * In a character set (`{:[...]}`), a backslash followed by a letter (like `\d`),
   or something that looks like a character class (like `[:alpha:]`), is now a format string error.
   Previously, they were matched as literal characters.
 * In an inverted character set, `]` after `^` now starts a range if followed by `-` and another character:
   `{:[^]-a]}` is everything except the characters from `]` to `a`. Previously, it was everything except `]`, `-`, and `a`.
 * `{:[]}` is now a format string error (an unterminated character set, like `{:[^]}` already was).
   Previously, it was an empty set.
 * `std::chrono::time_point`s are now scanned as UTC, applying an offset scanned with `%z`,
   instead of as local time with `std::mktime`, which depended on the `TZ` environment variable.
   Unset fields default to 1970-01-01 00:00:00.
   To get the previous result, convert the scanned UTC time to local time with `std::chrono::current_zone()`,
   or scan into a `std::tm` and call `std::mktime`.
 * `%u` in chrono format strings: `1` is now Monday and `7` Sunday, as in `strftime`,
   and, without `L`, `0` is rejected (previously, `1` was Sunday).
   For a Sunday-based weekday number from `0` to `6`, use `%w`.
 * `%x` in chrono format strings, without `L`, is now `%m/%d/%y`, as in the C locale
   (previously `%m/%d/%Y`). For a four-digit year, use `%m/%d/%Y`.
 * `%S` in chrono format strings reads an optional fractional part when scanning a `time_point`
   more precise than seconds, like `std::chrono::parse`, and the fractional part of `%.S` is now optional.
   To leave the fractional part unread, scan a `time_point` with a precision of seconds.
 * `scn::expected::emplace()` now requires `T` (or `E`, for `scn::unexpected`) to be nothrow constructible
   from the arguments, like `std::expected`.
   The assignment operators of `scn::expected` are now deleted or constrained like those of `std::expected`.
 * The CMake option `SCN_USE_32BIT` was removed.
   To build for 32-bit x86, use a toolchain file, or add `-m32` to `CMAKE_CXX_FLAGS`,
   `CMAKE_EXE_LINKER_FLAGS`, and `CMAKE_SHARED_LINKER_FLAGS`.
 * The CMake options `SCN_USE_RTTI`, `SCN_USE_NATIVE_ARCH`, and `SCN_USE_HASWELL_ARCH` were removed.
   Use `CMAKE_CXX_FLAGS` instead, for example `-fno-rtti` (`/GR-` with MSVC), `-march=native`, or `-march=haswell`.
   Unlike the options, the flags also apply to the code compiled from the headers in your own targets.
 * The CMake option `SCN_BENCHMARKS_BUILDTIME` was removed. The build time benchmark is run with `scripts/benchmark.py`.

### Changes

 * `char` and `wchar_t` scanned with an integer presentation type (`{:d}`) now skip leading whitespace, like integers.
   Previously, `" 42"` was an error.
 * `scn::scan_error::code_t` is now public, to name the error code enum without `enum scn::scan_error::code`
 * `scn::expected::value()` is now checked: if it contains an error, it throws the new `scn::bad_expected_access`
   (or aborts, without exceptions). `operator*` and `operator->` remain unchecked.
   This is consistent with `std::expected`.
 * `scn::expected<T, E>` can now be assigned an `scn::unexpected<G>` with `G` other than `E`, like `std::expected`
 * In release builds (with `NDEBUG`), internal assertions are no longer turned into optimizer assumptions
 * Floating-point error messages no longer name the backend (fast_float, `std::from_chars`, or `std::strtod`)
 * `%j` and `%Z` in the format string of a `time_point` are now format string errors, instead of always failing when scanning
 * A read error from a C `FILE` now fails the scan, instead of being treated as the end of the input
 * Regex support is disabled with `SCN_USE_EXCEPTIONS=OFF`, unless `SCN_REGEX_BACKEND` is `re2`,
   since `std::regex` and Boost.Regex report errors only by throwing
 * Textual boolean representations are now case-insensitive, so both `true` and `True` (and so on) are accepted.
 * `std::from_chars` is now used for `float` and `double` with libc++ 20 and newer
 * Floating-point values that can't be parsed with fast_float or `std::from_chars` (like `long double`, or hexfloats without `std::from_chars`)
   are now parsed with a custom built-in implementation, instead of `std::strtod`.
   This covers every type with a known binary format: the regular IEEE 754 types (binary16/32/64/128),
   bfloat16, and x87, m68k, and double-double `long double`.
   `std::strtod` (or `strtof`, `strtold`) is now only used for a `float`, `double`, or `long double` of an unknown format (like IBM hexadecimal floating-point).
   Instead of using `strtod_l`, or setting the C locale temporarily with `setlocale` (which isn't thread-safe), scnlib replaces the decimal point in the input with the one of the C locale.
   With `SCN_DISABLE_STRTOD`, scanning such a type is a compile-time error.
 * fast_float is only used if `float` is IEEE 754 binary32 and `double` is binary64. Otherwise, it isn't included, and doesn't break the build (like with a 32-bit `double`)
 * The configuration macros are now in a separate header, `<scn/macros.h>`
 * When scnlib is the top-level CMake project (and `SCN_DISABLE_TOP_PROJECT` is `OFF`), symbols are hidden by default
   (`CMAKE_CXX_VISIBILITY_PRESET` and `CMAKE_VISIBILITY_INLINES_HIDDEN`)
 * A whitespace character in a format string now also matches the end of the input, so `"{} "` matches `"123"`.
   A non-whitespace literal character not matched because the input ended now gives `end_of_input`,
   instead of `invalid_literal`.
 * Scanning takes 33-55% less time than in v4.0.1 with GCC, and 10-25% less with Clang
   (37% less with format specifiers, like `"{:d}"`), in the integer and floating-point benchmarks.
   Values like `scn::expected` and the scan arguments are no longer passed through memory in ways that stall the CPU,
   and format specifiers are handled with less overhead.
   Scanning strings takes 20-30% less time with GCC, because the UTF-8 validation no longer copies a lookup table to the stack on every call.
 * The compiler error for an invalid format string checked at compile time contains the error message with GCC and Clang,
   regardless of how the code is formatted, or whether the compiler shows the source code at all.
   This includes the messages of custom scanners passed to `ParseContext::on_error`,
   which is now `constexpr`, and keeps the first error if called more than once.
 * A character set (`{:[...]}`) on empty input now fails with `scn::scan_error::end_of_input`,
   instead of `invalid_scanned_value`
 * When scanning a `year_month_day` or a `time_point`, a weekday given together with a full date must now match the date.
   A weekday can now be used together with a day of the month: previously, that was a format string error.
 * With clang-cl, `SCN_MODULES` requires CMake 4.4 or newer, and the Ninja generator

### Features

 * A character set (`{:[...]}`) can now contain `]`, by putting it first, like in `scanf`:
   `{:[]abc]}`. This already worked in inverted sets (`{:[^]abc]}`).
 * `ranges::input_range`s can now be used as sources for `scn::scan`
   (previous requirement was `ranges::forward_range`)
   * When the source range is an `input_range`, the range type returned from `scn::scan` isn't a `subrange` anymore,
     but another `input_range`.
 * `scn::basic_regex_matches` now has `front()`, `back()`, `cbegin()`, `cend()`, `str()`, and `operator==`
 * `scn::scan_file` has been added, to scan from a `FILE` more resiliently than from a `FILE*` directly
 * `std::istream`s can now be used as sources for `scn::scan`, if `"scn/istream.h"` is included.
   * Instead of `.range()`, the member function to access the stream from a `scn::scan` result object is `.stream()`.
 * Locale-enabled overloads for `scn::input` have been added.
 * The `/m` regex flag is now supported with the `std` regex backend on MSVC Build Tools 14.50 (Visual Studio 2026) and later
 * Extended standard floating point types can now be scanned, if they're supported by the implementation
   (`std::float16_t`, `std::float32_t`, `std::float64_t`, `std::float128_t`, `std::bfloat16_t`)
 * `__int128` and `unsigned __int128` can now be scanned, if they're supported by the implementation.
   Aliases for these types are provided and used, with `scn::int128` and `scn::uint128`.
 * The payload of a NaN in floating-point values (like `nan(0x1234)`) is now parsed and stored in the scanned floating-point value.
   * This uses a new optional dependency, [eknan](https://github.com/eliaskosunen/eknan),
     fetched with `FetchContent` by default, like fast_float.
     It can be disabled with `SCN_DISABLE_EKNAN`, which makes payloads ignored,
     or found with `find_package`, with `SCN_USE_EXTERNAL_EKNAN`.
   * The payload is an integer, with the same base prefixes as with the `i` type specifier
     (`0b`, `0o`, `0x`, or `0` for octal), and `_` allowed anywhere after the prefix.
     A payload that is invalid, or doesn't fit in the NaN, gives the default quiet NaN.
 * C++20 modules support
 * Building as a shared library (with `BUILD_SHARED_LIBS`) is now supported, with the exported symbols marked explicitly
 * More optimized reading for source ranges that are both `bidirectional_range`s and `sized_range`s.
 * Regular expressions can now be scanned from non-contiguous sources, with the `std` and `Boost` regex backends
 * `scn::basic_scan_parse_context` now tells whether the source is contiguous or borrowed,
   with `is_source_contiguous()` and `is_source_borrowed()`, so that `scanner::parse` can check them
 * C++20 views whose `iterator_concept` is stronger than their `iterator_category` (like `std::views::transform`)
   can now be scanned from, with the range category given by their `iterator_concept`,
   as are ranges whose iterators only describe themselves with a `std::iterator_traits` specialization
   (like `std::common_iterator`, from `std::views::common`)
 * Ranges that can only be iterated over when non-const (like `std::views::filter`) can now be scanned from
 * The `rNN` and `RNN` integer type specifiers are now supported, for an arbitrary base `NN` from 2 to 36 (without a base prefix)

### Fixes

 * Tons of compiler warning fixes
 * Fix compiler errors occurring from accidental use of ADL when calling `decay_copy` unqualified
 * Fix scanning from stdin not being thread-safe on MinGW, and with other C standard libraries not recognized by scnlib,
   since the `FILE` wasn't locked
 * Add compatibility with fast_float v7 and v8
 * Fix a double-width character (like an emoji) being read when only one column of field width or precision was left.
   A precision smaller than the width of the next character now fails the scan with `invalid_scanned_value`.
 * Character arrays (including string literals) as sources now end at the first null character,
   or at the end of the array if there's none. Previously, the last element was always dropped,
   even if it wasn't a null character, and the characters after an embedded null character were scanned.
 * Fix compilation with GCC 8 in debug mode
 * Fix CMake deprecation warnings caused by `FetchContent_Populate` and `CMAKE_FIND_PACKAGE_SORT_ORDER`
 * Fix localized month names (`L%b`) being ignored when `SCN_DISABLE_FAST_FLOAT` is set
 * Fix the check for `SCN_REGEX_BOOST_USE_ICU` in the installed CMake package config,
   which also required fast_float when built with `SCN_DISABLE_FAST_FLOAT`, and Boost when built with `SCN_DISABLE_REGEX`
 * Fix `cmake --install --component scnlib_Development` not installing the library itself
 * Fix the CMake option `SCN_DISABLE_CHRONO` having no effect, and the build errors with it
 * `SCN_INSTALL` now defaults to `ON` whenever scnlib is the top-level CMake project,
   even with `SCN_DISABLE_TOP_PROJECT`, so that package managers don't need to set it
 * Fix `scn::expected`: `emplace()` over an error leaked the error,
   the converting constructors default-constructed a value and never destroyed it,
   and assigning a value over an error swallowed an exception thrown while constructing it.
   When constructing a value or an error threw, the copy and move constructors destroyed an object that was never constructed,
   and `emplace()` and assigning an `scn::unexpected` over a value destroyed the old one twice.
 * Fix `scn::expected<T, E>::operator=(U&&)` recursing until a stack overflow
   for a `U` that isn't assignable to `T` (like an `scn::expected<T, E>` lvalue)
 * `scn::expected<T, E>` is no longer constructible from `scn::unexpect` as a value of `T` (like with `T = std::any`)
 * `scn::expected<T, E>` is now nothrow move assignable when `T` and `E` are (like `scn::expected<std::string, int>`)
 * `scn::expected` is no longer marked `[[clang::trivial_abi]]` when `T` or `E` isn't trivially copyable,
   which made Clang relocate them with a bitwise copy
 * Fix multi-byte whitespace (like U+2028) not being recognized when it's split between two chunks of a non-contiguous source,
   like a buffered file
 * An invalid code point is no longer skipped as whitespace when scanning from a non-contiguous source,
   which made the result differ from a contiguous source with the same contents
 * Fix the lock of a C `FILE` being released between the characters put back into it after a scan
 * Fix scanning an integer right after another integer from a non-contiguous source (like `std::deque<char>`) failing
   with `end_of_input` when the format string has no whitespace between the fields (`"{}{}"`)
 * Fix `putback()` and repeated `unget()` in an `operator>>` used through `scn::basic_istream_scanner`
   failing an assertion, and `unget()` at the beginning of the input not failing
 * Fix argument tracking in format strings with 64 or more arguments,
   and report the first error in a format string, instead of the last
 * Overlong UTF-8 sequences, encoded surrogate code points, and unpaired UTF-16 surrogates are now treated as invalid
 * Fix scanning a `char32_t`, or a `wchar_t` from a narrow source: empty input
   was read out of bounds, and invalid or truncated UTF-8, surrogates, and values above U+10FFFF weren't rejected
 * Fix character set ranges crossing the ASCII boundary
 * Fix characters outside the BMP being transcoded with an invalid low surrogate when scanning a narrow source
   into a wide string with a 16-bit `wchar_t` (Windows)
 * `scn::scan_int` returns an error for an invalid base, and rejects unsupported types at compile time instead of failing to link.
   `scn::scan_int_exhaustive_valid` is now supported on big-endian platforms, and accepts leading zeroes.
 * Fix integer overflow not being detected for some 64-bit values with the maximum number of digits
   (e.g. `30000000000000000000` scanned as a `uint64_t` gave 11553255926290448384)
 * Integer presentations (like `{:d}`) for `char` and `wchar_t` now use the signedness of the type:
   where it's unsigned, values above the signed maximum are accepted, and negative values rejected
 * `{:c}` is now accepted for `char` and `wchar_t`, as documented,
   and the format string of a `char32_t` is checked the same way at compile time as at runtime
 * Fix localized integer scanning (`{:L}`): a thousands separator before the first digit (`,,,5`) was accepted,
   and with `o` or `i`, a leading zero followed by a thousands separator (`0,10`) ended the value at the zero.
   A thousands separator after the last digit is no longer consumed, so `"{:L}, {}"` matches `"1, 2"`.
 * Defining `SCN_USE_EXCEPTIONS` as `0` now makes scnlib behave as if exceptions were disabled; previously, the macro had no effect.
   Building with the CMake option `SCN_USE_EXCEPTIONS=OFF` now defines it for the users of the library too,
   and no longer fails to compile.
 * `scn::scan_error::to_errc()` returns `std::errc::invalid_argument` for `max_error` instead of being undefined behavior,
   and `scn::basic_scan_parse_context::advance_to()` reports an error for an iterator outside of the format string,
   which fails the scan when a custom scanner's `parse` returns one
 * Runtime format strings mixing automatic and manual argument indexing (like `"{} {0}"`) are now rejected,
   like format strings checked at compile time
 * Fix scanning ranges with wide characters failing to compile,
   and reading out of bounds with multi-character brackets or separators
 * Fix `scn::regex_matches` being unscannable when `<scn/ranges.h>` is included
 * Scanning a `scn::regex_matches` with a runtime format string without a regex (like `{}`) now fails,
   instead of failing an assertion (undefined behavior in release builds)
 * A custom scanner returning an iterator past the end of the source, or before the start of its field,
   now fails the scan with `scn::scan_error::invalid_source_state`,
   instead of reading out of bounds, or corrupting the position of a stream or a `FILE`
 * `scn::basic_istream_scanner` now imbues the locale passed to `scn::scan` into the stream given to `operator>>`
 * Fix compilation with libc++ 23, which removed `__wrap_iter::base()`
 * Fix the locale not being passed on when scanning a single argument with the default format string (`"{}"`)
 * Regular expressions: with the Boost backend, `.` no longer matches a newline without the `s` flag,
   and with the re2 backend, `^` and `$` no longer match at line breaks without the `m` flag
 * A `/` in a regex in a format string is escaped only by an odd number of backslashes,
   so that a regex can end in a literal backslash (`{:/\\/}`)
 * Scanning from a character pointer (like `const char*`) is now a call to a deleted function,
   with an explanation in C++26, instead of an unexplained "no matching function"
 * Fix fractional seconds being off by one unit of the `time_point` precision
   (e.g. `00.000511` scanned as 510 microseconds)
 * `%Z` in chrono format strings no longer hits an assertion failure on an empty timezone name, and fails instead
 * Fix chrono format strings: `%C` wasn't range-checked, a non-ASCII literal character caused the character after it to be treated as a literal
   (e.g. `ä%Y`), localized fields read from the beginning of the input instead of the current position,
   and building with `SCN_DISABLE_TYPE_STRING` failed
 * Fix `%y` and `%C` being combined incorrectly when scanning a `scn::datetime_components`,
   and `%j` only accepting values up to 6
 * Scanning a `month_day`, `year_month_day`, or `time_point` fails if the day doesn't exist in its month
   (e.g. February 30th), like `std::chrono::parse`. Previously, it was accepted, and a `time_point` was normalized
   to the next month. Scanning a `time_point` also fails on a leap second (second `60`).
 * Fix localized `%O` modifiers in chrono format strings (like `{:L%Om}`) always failing with libc++,
   and `L%OI` failing on `12` with libstdc++
 * Fix `long double` hexfloats close to the smallest subnormal value: with musl, a value rounding to zero
   wasn't reported as an underflow, and with a double-double `long double` (PowerPC), an exact half of the smallest
   subnormal value gave 2^-970
 * `%Ou` is now accepted in chrono format strings, `%OW` is no longer scanned as a weekday,
   and `%z` rejects minutes above 59
 * Documentation fixes, and documentation for scanning dates and times

## 4.0.1

_Released 2024-11-04_

 * Fix documentation generation

## 4.0.0

_Released 2024-11-03_

### Breaking changes

 * `scn::scanner::parse` now returns `Iterator`, not `scn::scan_expected<Iterator>`
   * Errors are reported by throwing a `scn::scan_format_string_error`, or by calling `ParseContext::on_error`.
 * Optimize the way `scn::scan` calls `scn::vscan` to remove extra copies/moves
```cpp
// Dummy-implementation of `scan`
// Before (v3):
auto args = make_scan_args<scan_context, Args...>();
auto result = vscan(std::forward<Source>(source), format, args);
return make_scan_result(std::move(result), std::move(args.args()));

// Now (v4):
auto result = make_scan_result<Source, Args...>();
fill_scan_result(result, vscan(std::forward<Source>(source), format,
                               make_scan_args(result->values())));
return result;
```
 * Changes to `scn::scan_error`
   * Success state removed: use `scn::expected<void, scan_error>` instead.
   * `scn::scan_error::value_out_of_range` split into `value_positive_overflow`, `value_negative_overflow`,
     `value_positive_underflow`, and `value_negative_overflow`.
   * `end_of_range` renamed to `end_of_input`.
   * `invalid_literal`, `invalid_fill`, `length_too_short`, and `invalid_source_state` added.
 * `scn::basic_scan_context` is now templated on the range type

### Features

 * `<chrono>` scanning
 * Scanning of pointers (`void*` and `const void*`)
 * Ability to disable dependency on FastFloat with `SCN_DISABLE_FAST_FLOAT`
   * FastFloat used to be the only required external dependency
   * If disabled, `std::from_chars` for floating-point values is required

### Changes

 * Deprecate `scn::visit_scan_arg`, add `scn::basic_scan_arg::visit`
 * Remove thousands separator checking when scanning localized numbers
 * `scn::scan_error::invalid_source_state` is now returned if syncing with the underlying source fails after `scn::scan`
   (like for example, `std::ungetc` fails)

## 3.0.2

_Released 2024-11-03_

 * Fix formatting options of user-defined types sometimes being ignored
 * Fix unnecessary blocking in `scn::input`
 * Fix usage of `std::regex_constants::multiline` on libstdc++ v11 (#130, thanks [@jiayuehua (Jia Yue Hua)](https://github.com/jiayuehua))
 * Fix build failures on Emscripten
 * Update documentation to have a version-dropdown
 * Fix typo in documentation about manual indexing (#122, thanks [@lynxlynxlynx (Jaka Kranjc)](https://github.com/lynxlynxlynx))
 * Fix typos in README (#120, thanks [@zencatalyst (Kasra Hashemi)](https://github.com/zencatalyst))

## 3.0.1

_Released 2024-06-15_

 * Bump `SOVERSION` to 3. (reported in #117, thanks [@xvitaly (Vitaly)](https://github.com/xvitaly))
 * Call `find_package` in CMake iff a target is not already defined (reported in #118, thanks [@ajtribick](https://github.com/ajtribick))
 * Remove `find_dependency(Threads)` from `scn-config.cmake`

## 3.0.0

_Released 2024-06-08_

### Breaking changes

 * The default behavior for scanning integers is now `d` (decimal) instead of `i` (detect base from prefix).

```cpp
// v3
auto result = scn::scan<int>("077", "{}");
// result->value() == 77
result = scn::scan<int>("078", "{}");
// result->value() == 78

// v2
auto result = scn::scan<int>("077", "{}");
// result->value() == 63
result = scn::scan<int>("078", "{}");
// result->value() == 7
// result->range() == "8"
// (Because of the '0' prefix, an octal number is expected,
//  and '8' is not a valid octal digit, so reading is stopped)
```

 * A large part of the bundled `<ranges>`-implementation is removed.
   Only the parts strictly needed for the library are included.
   * The library no longer uses a stdlib provided `<ranges>`, even if available.
   * This cut down compile times massively for library consumers
   * You now may need to specialize `scn::ranges::enable_borrowed_range` for your own range types,
     even if you've already specialized `std::ranges::enable_borrowed_range`.
     Specializations of `std::basic_string_view` are already borrowed out of the box.

```cpp
// std::span is a borrowed_range,
// but scnlib doesn't know about it
auto result = scn::scan<...>(std::span{...}, ...);
// decltype(result->range()) is scn::ranges::dangling

namespace scn::ranges {
template <typename T, size_t E>
inline constexpr bool enable_borrowed_range<std::span<T, E>> = true;
}

auto result = scn::scan<...>(std::span{...}, ...);
// decltype(result->range()) is a scn::ranges::subrange<const T*>
```

 * `scn::span` is removed
 * `scan_arg_store` and `borrowed_subrange_with_sentinel` are removed from the public interface
 * `scan_arg_store` is changed to be non-copyable and non-movable, for correctness reasons
   (it holds references to itself, copying and moving would be needlessly expensive)
 * The interface of `make_scan_result` is changed to take a `std::tuple` instead of the now unmovable `scan_arg_store`.

```cpp
// v3
auto args = make_scan_args<scan_context, Args...>();
auto result = vscan(source, format, args);
return make_scan_result(std::move(result), std::move(args.args()));

// v2
auto args = make_scan_args<scan_context, Args...>();
auto result = vscan(source, format, args);
return make_scan_result(std::move(result), std::move(args));
```

 * The meaning of the "width" field in format specifiers is changed to mean the _minimum_ field width
   (like in `std::format`), instead of the _maximum_ (sort of like in `std::scanf`)

```cpp
// v3
auto result = std::scan<int>("123", "{:2}");
// result->value() == 123
// result->range() == ""

// v2
auto result = std::scan<int>("123", "{:2}");
// result->value() == 12
// result->range() == "3"
```

### Features

 * The "precision" field is added as a format specifier,
   which specifies the maximum fields width to scan (like in `std::format`)

```cpp
// Scan up to 2 width units
auto result = scn::scan<int>("123", "{:.2}");
// result->value() == 12
// result->range() == "3"
```

* Support for field fill and alignment is added.
  This interacts well with the new width and precision fields

```cpp
// Read an integer, aligned to the right ('>'), with asterisks ('*')
auto result = std::scan<int>("***42", "{:*>}");
// result->value() == 42
// result->range() == ""

// Read an integer, aligned to the left ('<'), with whitespace (default),
// with a maximum total width of 3
auto result = std::scan<int>("42  ", "{:<.3}");
// result->value() == 42
// result->range() == " "
```

 * In general, there seems to be a ~10% to 20% improvement in run-time performance.
 
### Changes

 * The dependency on `simdutf` is removed. The library now has no external dependencies to compiled libraries
   (FastFloat, an internal dependency, is a header-only library)
 * The number of source files is dramatically decreased: there are now just the public headers, 
   a private implementation header, and a private implementation source file. This cuts down the time needed to
   compile the library, and any user code including it to a half (yes, really!)

### Fixes

 * Fix skipping of multiple characters of whitespace in the format string (reported in #116, thanks [@Jonathan-Greve (Jonathan Bjørn Greve)](https://github.com/Jonathan-Greve))

## 2.0.3

_Released 2024-05-19_

### Fixes

 * Fix documentation: default format type specifier for integers is `i`, not `d`:
   when not explicitly specified by a format specifier, the base of an integer is determined based on its prefix:
   `0x...` is hexadecimal, `0...` or `0o...` is octal, `0b...` is binary, and everything else is decimal.
 * Fix a compilation error which would occur when scanning more than 11 arguments with `scn::scan`.
 * Small CMake adjustments to better support use as a subproject (#113, thanks [@frankmiller (Frank Miller)](https://github.com/frankmiller))
 * Fix misplaced include of `GNUInstallDirs` in CMake (#111, thanks [@WangWeiLin-MV](https://github.com/WangWeiLin-MV))
 * Allow for externally installed versions for GTest and Google Benchmark (#112, thanks [@xvitaly (Vitaly)](https://github.com/xvitaly))
 * Adjust the definition of `SCN_COMPILER` to fix usage with a recent Clang using modules (#109, thanks [@Delta-dev-99 (Armando Martin)](https://github.com/Delta-dev-99))
 * Allow for more versions of dependencies (simdutf, fast_float)
 * Fix C++23 build failure caused by missing inclusion of `<utility>` for `std::unreachable`

## 2.0.2

_Released 2024-02-19_

### Fixes

 * Fix segfault when runtime-parsing `{:[^` as a format string.
 * Fix compilation of `scan_buffer.cpp` on some MSVC versions.
 * Remove stray `test/` folder

## 2.0.1

_Released 2024-02-12_

### Fixes

 * Fix detection of support for `std::regex_constants::multiline` (#98, thanks [@jiayuehua (Jia Yue Hua)](https://github.com/jiayuehua))
 * Fix builds of `float_reader` on Android SDK < v28 (#99, thanks [@jiayuehua (Jia Yue Hua)](https://github.com/jiayuehua))
 * Fix typo in README example (#100, thanks [@ednolan (Eddie Nolan)](https://github.com/ednolan))

## 2.0.0

_Released 2024-01-19_

Changes are in comparison to 2.0.0-beta.

### Features

 * Regular expression scanning in `<scn/regex.h>`
 * Scanning of `FILE*`s.
 * Faster integer scanning with `scn::scan_int`
 * Allow disabling support for:
   * Specific types (`SCN_DISABLE_TYPE_*`)
   * Locales (`SCN_DISABLE_LOCALE`)
   * IOStreams (`SCN_DISABLE_IOSTREAM`)
   * Regex support (`SCN_DISABLE_REGEX`)
   * Floating-point scanning fallbacks (`SCN_DISABLE_FROM_CHARS`, `SCN_DISABLE_STRTOD`)
   * These can be useful in some constrained environments, where these facilities are either not available or not used
   * Thanks [@cjvaughter (CJ Vaughter)](https://github.com/cjvaughter) for the original implementation in v1 in #70 and #71

### Changes

 * `scn::basic_istreambuf_view` removed.
   * Sort-of replaced by being able to scan from `FILE`s.
 * `scn::basic_erased_range` and `scn::basic_caching_view` removed.
   * Replaced by library-internal `scn::detail::basic_scan_buffer`.
 * String scanners now require valid Unicode as input, and don't spew out garbage when given some.
 * Specifiers like `:alpha:` and `\w` removed from `[character set]` matching.
   Use regular expressions instead, or explicitly state the character range (like `a-zA-Z` for `:alpha:`).
 * `scn::runtime` renamed to `scn::runtime_format`.

### Fixes and small changes

 * Update implementation of `scn::span`
 * Fix handling of literal space characters in format strings
 * Add `COMPONENT` values to CMake install targets
 * Default to Release-builds in CMake if no `CMAKE_BUILD_TYPE` is set, and we're the top project
   (see #89, thanks [@ldeng-ustc (Long Deng)](https://github.com/ldeng-ustc) for the idea)
 * Add install destination for shared libraries (thanks [@uilianries (Uilian Ries)](https://github.com/uilianries))

## 2.0.0-beta

_Released 2023-10-29_

Major overhaul, both internally and in terms of the library interface. The library is rewritten in its entirety.
See the documentation (note the new URL: https://v2.scnlib.dev/),
namely the [Migration guide](https://v2.scnlib.dev/v2.0.0-beta/migration-2-0.html) for more details.

This is a beta pre-release.
There may still be some bugs.
Changes before v2.0.0 proper are possible, but aren't probably going to be major.

Major changes include:

* C++17 is required.
* Several names are changed to include the `scan_` prefix.
* `scn::scan` returns the scanned values by value. Output parameters are no longer used.
* `scn::scan` now accepts all `forward_range`s (v1: `bidirectional_range` + default- and move constructible).
* `scn::scan` returns a view (`subrange`) into its input, and never takes ownership.
* Scope is more focused: list operations, `ignore`, `getline`, and `file` have been removed.
* Performance improvements
* Completely reworked internals

## 1.1.3

_Released 2023-11-05_

Expected to be the last release in the v1-branch.
Development efforts are now fully directed towards v2.

### Features

* Allow disabling support for individual types with `SCN_DISABLE_TYPE_*` (#70, thanks [@cjvaughter (CJ Vaughter)](https://github.com/cjvaughter))
    * Also, allow for disabling the fallbacks to `std::from_chars` and `std::strtod` when scanning floats
    * This provides possible binary size reductions, and allows better use in e.g. embedded platforms
* Allow disabling runtime localization with `SCN_DISABLE_LOCALE` (#71, thanks [@cjvaughter (CJ Vaughter)](https://github.com/cjvaughter))
* Parse leading `+` signs in floats (reported in #77)

### Fixes

* Fix `scn::wrap(std::string_view&&)` being ambiguous on gcc (reported in #83)
* Fix compiler error in `scn::basic_string_view<CharT>::substr` (reported in #86)
* Fix memory safety issues found with ASan and UBsan in
  `small_vector`, `detail::utf16::validate_next`, and `detail::get_buffer`.
* Add `COMPONENT` to CMake install targets (#80, thanks [@pawelwod](https://github.com/pawelwod))
* Fix calculation of `SCN_MSVC` from `_MSC_FULL_VER` (#62, thanks [@matbech (Mathias Berchtold)](https://github.com/matbech))
* Fix MSVC `C4146` warning in `integer_scanner` (#64, thanks [@matbech (Mathias Berchtold)](https://github.com/matbech))
* Use `if constexpr` and `std::unreachable` if available (#61, #78, thanks [@matbech (Mathias Berchtold)](https://github.com/matbech))
* Improve error messages given from the float parser

## 1.1.2

_Released 2022-03-19_

* Change `SCN_VERSION` to report the correct version number: 1.1.0 -> 1.1.2 (#57)

## 1.1.1

_Released 2022-03-16_

* Fix issue with values being skipped when using files and `file.sync()` (#56)
    * Every call to `file.sync()` needs to be accompanied by a call to `reset_begin_iterator()` to the result object
    * This is a temporary fix, permanent fix coming in v2.0.0

```cpp
int i;
auto ret = scn::scan(scn::cstdin(), "{}", i);
scn::cstdin().sync();
ret.range().reset_begin_iterator();

// Not necessary with input and prompt
ret = scn::input("{}", i);
```

## 1.1

_Released 2022-03-12_

* Add support for scanning 8-bit integers (`(un)signed char`, `(u)int8_t`),
  and characters (`char`, `wchar_t`) as integers

```cpp
int8_t i1, i2;
char c1, c2;
auto ret = scn::scan("1 2 3 4", "{} {:c} {} {:i}", i1, i2, i3, i4);
// ret == true
// i1 == 1
// i2 == '2'
// c1 == '3'
// c2 == 4
```

* Fix usage of external fast_float in CMake (#53, thanks [@xvitaly (Vitaly Zaitsev)](https://github.com/xvitaly))
* Fix tests on big endian architectures (#54)
* Fix alignment issues with `small_vector` on 32-bit architectures

## 1.0

_Released 2022-02-28_

* Fix bugs
* Reorganize and expand fuzzing, making it oss-fuzz ready
* Expand test suite with more localized tests
* Update benchmark results
* Update submodules

## 1.0-rc1

_Released 2022-02-21_

### Additions

* Unicode support
    * Ranges and format strings are assumed to be UTF-8 (narrow), UTF-16 (wide, Windows), or UTF-32 (wide, POSIX), regardless of locale
    * Add `scn::code_point`, which can be passed as a delimeter to `scn::getline`, `scn::scan_list` and others
    * Locales rewritten
* Add `lemire/fast_float` as a dependency, and use it for parsing floats if possible
* Add `scn::scan_usertype` and `scn::vscan_usertype`
* Add `<scn/fwd.h>` header file

### Changes

* Redesigned format strings
    * Alignment, fill, width flags
    * Position-dependent `L`
    * Reworked integer and float parsing flags (new defaults)
    * String set parsing
    * Add `scn::common_parser`
    * Redesigned whitespace skipping
* `scn::buffer_scanner` -> `scn::span_scanner`, spans are more like strings now when scanned
* Massive folder and file reorganization
* Rewritten list scanning
* `result::string_view()`, `::span()`, `::string()` -> `result::range_as_*()`
* `read_char()` -> `read_code_unit()`
* Decrease generated code sizes by redesigning vscan, and adding more vscan definitions
    * Move `scn::wrap` to the public interface
    * Add `scn::make_args_for`, `scn::make_context`, and `scn::make_parse_context`
* `[[nodiscard]]` added where necessary

### Fixes and minor stuff

* Update bloat benchmarks
* Add a lot more tests
* Update GitHub workflows: add gcc11, clang12, msvc2022, gcc on macOS, and Alpine, remove clang9
* Radically increase fuzzing coverage, fix bugs found with it
* Fix typo in README example (#41, #44, thanks [@andreasbuykx (Andreas Buykx)](https://github.com/andreasbuykx) and [@phoebe-leong](https://github.com/phoebe-leong))
* Fix leftover typo in docs/guide (#49, thanks [@danra (Dan Raviv)](https://github.com/danra))
* New .clang-format file
* Tweaks to README badges
* Update submodules
* Numerous bug fixes

## 0.4

_Released 2020-11-13_

### Changes and removals

* Rework source range handling:
    * Non-views are now accepted as source ranges --
      this includes types like `std::string` and `std::vector<char>`
    * Non-reconstructible ranges are now also accepted --
      scanning functions no longer return a reconstructed source range.
      The member function `.range()` can be used to scan the range again,
      and `.reconstruct()` reconstructs the range, if possible.
      Other helper member functions are also available.
    * Source ranges are now either taken by const lvalue reference or rvalue reference,
      so they are no longer modified by scanning functions.
      To access the leftover range, use the return value of the scanning function.
* Rewrite file handling, with hopefully way less bugs this time around
    * Remove `file_view` and caching ranges
    * Move memory mapped files to the public API
* Remove `default_tag`, replace with `scan_default` function template
* Remove support for `scanf` syntax, including `scn::scanf` and `scn::basic_scanf_parse_context`.
* Improve Ranges integration:
    * Move custom Ranges implementation to the public API (out from `scn::detail::ranges`): `scn::custom_ranges`
    * Integrate standard library Ranges, if available: `scn::std_ranges` aliased to `std::ranges`
    * Use stdlib Ranges, if available, fall back to custom implementation: namespace alias `scn::ranges`, control behavior with `SCN_USE_STD_RANGES`

### Additions

* Add more thorough documentation, tests, benchmarks and examples
* Add `scan_list_until`

### Fixes and minor stuff

* Fix float parsing not being locale-agnostic when global C locale was not `"C"`
  (#24, thanks [@petrmanek (Petr Mánek)](https://github.com/petrmanek) and
  [@amyspark](https://github.com/amyspark) for reporting)
* Fix `SONAME` (#32, thanks [@xvitaly (Vitaly Zaitsev)](https://github.com/xvitaly))
* Use system doctest and google-benchmark if available
  (#28, #30, #31,
  thanks [@xvitaly (Vitaly Zaitsev)](https://github.com/xvitaly), and
  [@leha-bot (Alex)](https://github.com/leha-bot) for reporting)
* Fix CUDA compilation (#22, thanks [@invexed (James Beach)](https://github.com/invexed) for reporting)
* Move to readthedocs (https://scnlib.readthedocs.com) from https://scnlib.dev
* Move to GitHub Actions from Travis and Appveyor

## 0.3

_Released 2020-02-19_

Largely a bugfix release

### Changes

* Remove support for partial successes
    * If the reading of any of the arguments given to `scan` fail, the whole function fails
    * `read`-field removed from `result`
* Overhaul list scanning
    * Add `scan_list`

### Fixes

* Fix issues with `std::string_view` and MSVC debug iterators (#11, #14, #18, #20)
* Fix some issues with scanning customized types (#15)
* Add missing support for custom-allocator `std::string`s (#16)
* Fix erroneous `git` command in README (#13)
* Fix README example
* Fix erroneous usage of library feature test macros

Thanks to [@nanoric](https://github.com/nanoric) and [@SuperWig (Daniel Marshall)](https://github.com/SuperWig) for bug reports!

### Removals

* Remove support for non-`std::char_traits` `std::string`s
* Remove support for clang 3.6

## 0.2

_Released 2019-10-18_

### Major changes

* Remove the notion of streams, and replace them with ranges
    * Using a hand-written minimal ranges implementation based on NanoRange: https://github.com/tcbrindle/NanoRange
    * A lot of things changed, renamed or removed because of this

### Additions

* Add zero-copy parsing of `string_view`s from a `contiguous_range`
    * Works like parsing a `std::string`, except the pointers of the `string_view` are directed to the input range
* Add `scan_list`
* Add experimental memory mapped file reading support

### Removals

* Remove `scn::options` and `scn::scan` overloads taking one
* Remove parsing algorithms based on `std::strto*`, `std::sto*` and `std::from_chars`
* Remove reading from a `std::basic_istream`

### Changes

* Rename `scan(locale, ...)` to `scan_localized(...)`
* Fix UB in `small_vector` using `std::aligned_storage` and `std::aligned_union`
* _And probably tons more that I've not written down anywhere_

## 0.1.2

_Released 2019-06-25_

* Add `SCN_RANGES` CMake option
* Add `scn::temp` helper function
* Fix `-Wpadded` warnings on clang
* Fix `-Wfloat-equal` and `-Wconversion` warnings on gcc
* Fix `C4146` error on UWP MSVC
* Add CONTRIBUTING.md

## 0.1.1

_Released 2019-06-25_

* Add more examples
* Fix #8: Fix segfault when using `scn::cstdin()` or `scn::wcstdin()`,
  caused by the copy and move constructor of `small_vector` setting data pointer to `nullptr`
  if copying/moving from an empty `small_vector`.
  (Thanks [@SuperWig (Daniel Marshall)](https://github.com/SuperWig) for reporting!)
* Fix compilation error when using `scn::ranges::get_value`.
* Fix a badge in README (thanks [@p1v0t](https://github.com/p1v0t))

## 0.1

_Released 2019-06-23_

Initial release
