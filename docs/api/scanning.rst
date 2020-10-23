========
Scanning
========

Main part of the public API.

Generally, the functions in this group take a range, a format string, and
a list of arguments. The arguments are parsed from the range based on the
information given in the format string.

If the function takes a format string and a range, they must share
character types. Also, the format string must be convertible to
``basic_string_view<CharT>``, where ``CharT`` is that aforementioned
character type.

Scanning functions
------------------

.. doxygenfunction:: scn::scan
.. doxygenfunction:: scan_default
.. doxygenfunction:: scan_localized
.. doxygenfunction:: scan_value
.. doxygenfunction:: input
.. doxygenfunction:: prompt
.. doxygenfunction:: getline(Range &&r, String &str) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: getline(Range &&r, String &str, CharT until) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: ignore_until
.. doxygenfunction:: ignore_until_n
.. doxygenfunction:: scan_list
.. doxygenfunction:: scan_list_until

Return type
-----------

The return type of these functions is based on the type of the given range.
It contains an object of that range type, representing what was left over of the range after scanning.
The type is designed in such a way as to minimize copying and dynamic memory allocations.
The type also contains an error value.

Note, that the values scanned are only touched iff the scanning succeeded, i.e. ``operator bool()`` returns ``true``.
This means, that reading from a default-constructed value of a built-in type on error will cause UB:

.. doxygenclass:: scn::detail::scan_result_base
    :members:

.. code-block:: cpp

    int i;
    auto ret = scn::scan("foo", "{}", i);
    // ret == false
    // i is still default-constructed -- reading from it is UB
