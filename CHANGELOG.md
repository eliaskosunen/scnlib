# 0.4 (in development)

_Released 2020-xx-xx_

 * Move to readthedocs
 * Rewritten file handling
 * Add `scan_list_until`

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

Thanks to @nanoric and @SuperWig for bug reports!

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
   (Thanks @SuperWig for reporting!)
 * Fix compilation error when using `scn::ranges::get_value`.
 * Fix a badge in README (thanks @p1v0t)

# 0.1

_Released 2019-06-23_

Initial release
