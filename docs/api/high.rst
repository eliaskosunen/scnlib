=======================
Higher-level operations
=======================

Functions in this category return a ``scan_result<Range, error>``.
It has the following member functions:

 - ``operator bool``: ``true`` when successful
 - ``range() -> Range``
 - ``error() -> error``

.. doxygenfunction:: parse_integer
.. doxygenfunction:: scn::getline(Range &&r, String &str) -> decltype(getline(std::forward<Range>(r), str, detail::ascii_widen<CharT>('\n')))
.. doxygenfunction:: scn::getline(Range &&r, String &str, CharT until) -> decltype(detail::getline_impl(std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(), str, until))
.. doxygenfunction:: ignore_until
.. doxygenfunction:: ignore_until_n
.. doxygenfunction:: scan_list
.. doxygenfunction:: scan_list_until
