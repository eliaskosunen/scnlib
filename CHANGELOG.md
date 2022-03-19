# 1.1.2

_Released 2022-03-19_

 * Change `SCN_VERSION` to report the correct version number: 1.1.0 -> 1.1.2 (#57)

# 1.1.1

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

# 1.1

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

# 1.0

_Released 2022-02-28_

 * Fix bugs
 * Reorganize and expand fuzzing, making it oss-fuzz ready
 * Expand test suite with more localized tests
 * Update benchmark results
 * Update submodules

# 1.0-rc1

_Released 2022-02-21_

## Additions

 * Unicode support
   * Ranges and format strings are assumed to be UTF-8 (narrow), UTF-16 (wide, Windows), or UTF-32 (wide, POSIX), regardless of locale
   * Add `scn::code_point`, which can be passed as a delimeter to `scn::getline`, `scn::scan_list` and others
   * Locales rewritten
 * Add `lemire/fast_float` as a dependency, and use it for parsing floats if possible
 * Add `scn::scan_usertype` and `scn::vscan_usertype`
 * Add `<scn/fwd.h>` header file

## Changes

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

## Fixes and minor stuff

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

# 0.4

_Released 2020-11-13_

## Changes and removals

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

## Additions

 * Add more thorough documentation, tests, benchmarks and examples
 * Add `scan_list_until`  

## Fixes and minor stuff

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

# 0.3

_Released 2020-02-19_

Largely a bugfix release

## Changes

 * Remove support for partial successes
   * If the reading of any of the arguments given to `scan` fail, the whole function fails
   * `read`-field removed from `result`
 * Overhaul list scanning
   * Add `scan_list`

## Fixes

 * Fix issues with `std::string_view` and MSVC debug iterators (#11, #14, #18, #20)
 * Fix some issues with scanning customized types (#15)
 * Add missing support for custom-allocator `std::string`s (#16)
 * Fix erroneous `git` command in README (#13)
 * Fix README example
 * Fix erroneous usage of library feature test macros

Thanks to [@nanoric](https://github.com/nanoric) and [@SuperWig (Daniel Marshall)](https://github.com/SuperWig) for bug reports!

## Removals

 * Remove support for non-`std::char_traits` `std::string`s
 * Remove support for clang 3.6

# 0.2

_Released 2019-10-18_

## Major changes

 * Remove the notion of streams, and replace them with ranges
   * Using a hand-written minimal ranges implementation based on NanoRange: https://github.com/tcbrindle/NanoRange
   * A lot of things changed, renamed or removed because of this

## Additions

 * Add zero-copy parsing of `string_view`s from a `contiguous_range`
   * Works like parsing a `std::string`, except the pointers of the `string_view` are directed to the input range
 * Add `scan_list`
 * Add experimental memory mapped file reading support

## Removals

 * Remove `scn::options` and `scn::scan` overloads taking one
 * Remove parsing algorithms based on `std::strto*`, `std::sto*` and `std::from_chars`
 * Remove reading from a `std::basic_istream`

## Changes

 * Rename `scan(locale, ...)` to `scan_localized(...)`
 * Fix UB in `small_vector` using `std::aligned_storage` and `std::aligned_union`
 * _And probably tons more that I've not written down anywhere_

# 0.1.2

_Released 2019-06-25_

 * Add `SCN_RANGES` CMake option
 * Add `scn::temp` helper function
 * Fix `-Wpadded` warnings on clang
 * Fix `-Wfloat-equal` and `-Wconversion` warnings on gcc
 * Fix `C4146` error on UWP MSVC
 * Add CONTRIBUTING.md

# 0.1.1

_Released 2019-06-25_

 * Add more examples
 * Fix #8: Fix segfault when using `scn::cstdin()` or `scn::wcstdin()`,
   caused by the copy and move constructor of `small_vector` setting data pointer to `nullptr`
   if copying/moving from an empty `small_vector`.
   (Thanks [@SuperWig (Daniel Marshall)](https://github.com/SuperWig) for reporting!)
 * Fix compilation error when using `scn::ranges::get_value`.
 * Fix a badge in README (thanks [@p1v0t](https://github.com/p1v0t))

# 0.1

_Released 2019-06-23_

Initial release
