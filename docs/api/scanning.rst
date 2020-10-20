========
Scanning
========

Core part of the public scanning API.

Generally, the functions in this group take a range, a format string, and
a list of arguments. The arguments are parsed from the range based on the
information given in the format string.

If the function takes a format string and a range, they must share
character types. Also, the format string must be convertible to
``basic_string_view<CharT>``, where ``CharT`` is that aforementioned
character type.

The majority of the functions in this category return a
``scan_result<Range, result<ptrdiff_t>>``, which has the following member
functions:

 * ``operator bool``: ``true`` when successful
 * ``range() -> Range``
 * ``error() -> error``

.. doxygenfunction:: scan
.. doxygenfunction:: scan_default
.. doxygenfunction:: scan_localized
.. doxygenfunction:: scan_value
.. doxygenfunction:: input
.. doxygenfunction:: prompt
