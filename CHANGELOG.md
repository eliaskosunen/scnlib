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
