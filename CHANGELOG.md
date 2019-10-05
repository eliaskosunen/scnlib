# 0.2

_Released 2019-10-xx_

## Major changes

 * Remove the notion of streams, and replace them with ranges
   * Using a hand-written minimal ranges implementation based on NanoRange: https://github.com/tcbrindle/NanoRange
   * A lot of things changed, renamed or removed because of this

## Additions

 * Add zero-copy parsing of `string_view`s from a `contiguous_range`
   * Works like parsing a `std::string`, except the pointers of the `string_view` are directed to the input range
 * Add `list` (see #9)
 * Add experimental memory mapped file reading support

## Removals

 * Remove `scn::options` and `scn::scan` overloads taking one
 * Remove parsing algorithms based on `std::strto*`, `std::sto*` and `std::from_chars`
 * Remove reading from a `std::basic_istream`

## Changes

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
