# Changelog

## 4.0.1

_Released 2024-11-04_

 * Fix documentation generation

## 4.0.0

_Released 2024-11-03_

### Breaking changes

 * `scanner::parse` now returns `Iterator`, not `scan_expected<Iterator>`
   * Errors are reported by throwing a `scan_format_string_error`, or by calling `ParseContext::on_error`.
 * Optimize the way `scan` calls `vscan` to remove extra copies/moves
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
 * Changes to `scan_error`
   * Success state removed: use `expected<void, scan_error>` instead.
   * `scan_error::value_out_of_range` split into `value_positive_overflow`, `value_negative_overflow`,
     `value_positive_underflow`, and `value_negative_overflow`.
   * `end_of_range` renamed to `end_of_input`.
   * `invalid_literal`, `invalid_fill`, `length_too_short`, and `invalid_source_state` added.
 * `basic_scan_context` is now templated on the range type

### Features

 * `<chrono>` scanning
 * Scanning of pointers (`void*` and `const void*`)
 * Ability to disable dependency on FastFloat with `SCN_DISABLE_FAST_FLOAT`
   * FastFloat used to be the only required external dependency
   * If disabled, `std::from_chars` for floating-point values is required

### Changes

 * Deprecate `visit_scan_arg`, add `basic_scan_arg::visit`
 * Remove thousands separator checking when scanning localized numbers
 * `scan_error::invalid_source_state` is now returned if syncing with the underlying source fails after `scan`
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
 * The interface of `make_scan_result` is changed to take a `tuple` instead of the now unmovable `scan_arg_store`.

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
   (like in `std::format`), instead of the _maximum_ (sort of like in `scanf`)

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
