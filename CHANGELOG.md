# 0.1.1

_Released 2019-06-25_

 * Add more examples
 * Fix #8: Fix segfault when using `scn::cstdin()` or `scn::wcstdin()`,
   caused by the copy and move constructor of `small_vector` setting data pointer to `nullptr`
   if copying/moving from an empty `small_vector`.
   (Thanks @SuperWig for reporting!)
 * Fix compilation error when using `scn::ranges::get_value`.

# 0.1

_Released 2019-06-23_

Initial release
