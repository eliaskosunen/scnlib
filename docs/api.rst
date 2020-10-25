=================
API Documentation
=================

Scanning functions
------------------

Main part of the public API.

Generally, the functions in this group take a range, a format string, and
a list of arguments. The arguments are parsed from the range based on the
information given in the format string.

If the function takes a format string and a range, they must share
character types. Also, the format string must be convertible to
``basic_string_view<CharT>``, where ``CharT`` is that aforementioned
character type.


.. doxygenfunction:: scn::scan
.. doxygenfunction:: scan_default
.. doxygenfunction:: scan_localized
.. doxygenfunction:: scan_value
.. doxygenfunction:: input
.. doxygenfunction:: prompt
.. doxygenfunction:: getline(Range &&r, String &str, CharT until) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: getline(Range &&r, String &str) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: ignore_until
.. doxygenfunction:: ignore_until_n
.. doxygenfunction:: scan_list
.. doxygenfunction:: scan_list_until

Input range
-----------

Various kinds of ranges can be passed to scanning functions.

Fundamentally, a range is something that has a beginning and an end.
Examples of ranges are a string literal, a C-style array, and a ``std::vector``.
All of these can be passed to ``std::begin`` and ``std::end``, which then return an iterator to the range.
This notion of ranges was standardized in C++20 with the Ranges TS.
This library provides barebone support of this functionality.

Input range requirements
************************

Ranges passed to scanning functions must be:
 * bidirectional
 * default and move constructible

Using C++20 concepts:

.. code-block:: cpp

    template <typename Range>
    concept scannable_range =
        std::ranges::bidirectional_range<Range> &&
        std::default_constructible<Range> &&
        std::move_constructible<Range>;

A bidirectional range is a range, the iterator type of which is
bidirectional: http://eel.is/c++draft/iterator.concepts#iterator.concept.bidir.
Bidirectionality means, that the iterator can be moved both
forwards: ``++it`` and backwards ``--it``.

Note, that both random-access and contiguous ranges are refinements of
bidirectional ranges, and can be passed to the library. In fact, the library
implements various optimizations for contiguous ranges.

Recommended range requirements
******************************

In addition, to limit unnecessary copies and possible dynamic memory allocations,
the ranges should be passed as an lvalue, and/or be a ``view``: http://eel.is/c++draft/range.view.
A ``view`` is a ``range`` that is cheap to copy: think ``string_view`` or ``span``.

Passing a non-view as an rvalue will work, but it may cause worse performance, especially with larger source ranges.

.. code-block:: cpp

    // okay: view
    scn::scan(std::string_view{...}, ...);

    // okay: lvalue
    std::string source = ...
    scn::scan(source, ...);

    // worse performance: non-view + rvalue
    scn::scan(std::string{...}, ...);

In order for the ``.reconstruct()`` member function to compile in the result object,
the range must be a ``pair-reconstructible-range`` as defined by https://wg21.link/p1664r1,
i.e. be constructible from an iterator and a sentinel.

Character type
**************

The range has an associated character type.
This character type can be either ``char`` or ``wchar_t``.
The character type is determined by the result of ``operator*`` of the range
iterator. If dereferencing the iterator returns

 * ``char`` or ``wchar_t``: the character type is ``char`` or ``wchar_t``, respectively
 * ``expected<char>`` or ``expected<wchar_t>``: the character type is ``char`` or ``wchar_t``, respectively

Return type
-----------

The return type of the scanning functions is based on the type of the given range.
It contains an object of that range type, representing what was left over of the range after scanning.
The type is designed in such a way as to minimize copying and dynamic memory allocations.
The type also contains an error value.

.. doxygenstruct:: scn::wrapped_error
    :members:

.. doxygenclass:: scn::detail::scan_result_base
    :members:

Note, that the values scanned are only touched iff the scanning succeeded, i.e. ``operator bool()`` returns ``true``.
This means, that reading from a default-constructed value of a built-in type on error will cause UB:

.. code-block:: cpp

    int i;
    auto ret = scn::scan("foo", "{}", i);
    // ret == false
    // i is still default-constructed -- reading from it is UB

Error types
***********

.. doxygenclass:: scn::error
    :members:
.. doxygenclass:: scn::expected
    :members:

Convenience scan types
----------------------

These types can be passed to scanning functions (``scn::scan`` and alike) as arguments, providing useful functionality.

.. doxygenstruct:: scn::temporary
    :members:
.. doxygenfunction:: temp

.. doxygenfunction:: discard

.. doxygenstruct:: scn::span_list_wrapper
    :members:
.. doxygenfunction:: make_span_list_wrapper

Lower level parsing and scanning operations
-------------------------------------------

.. doxygenfunction:: vscan

``parse_integer`` and ``parse_float`` will provide super-fast parsing from a string, at the expense of some safety and usability guarantees.
Using these functions can easily lead to unexpected behavior or UB if not used correctly and proper precautions are not taken.

.. doxygenfunction:: parse_integer
.. doxygenfunction:: parse_float

The following functions abstract away the input range in easier to understand parsing operations.

.. doxygenfunction:: read_zero_copy
.. doxygenfunction:: read_all_zero_copy
.. doxygenfunction:: read_into
.. doxygenfunction:: read_until_space_zero_copy
.. doxygenfunction:: read_until_space
.. doxygenfunction:: read_until_space_ranged
.. doxygenfunction:: putback_n
.. doxygenfunction:: skip_range_whitespace

Utility types
-------------

.. doxygenclass:: scn::basic_string_view
    :members:
.. doxygentypedef:: string_view
.. doxygentypedef:: wstring_view

.. doxygenclass:: scn::span
    :members:

.. doxygenclass:: scn::optional
    :members:
