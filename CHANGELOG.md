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
