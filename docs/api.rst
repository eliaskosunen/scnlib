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

.. doxygenfunction:: getline(Range &&r, String &str, Until until) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: getline(Range &&r, String &str) -> detail::scan_result_for_range<Range>
.. doxygenfunction:: ignore_until
.. doxygenfunction:: ignore_until_n

Source range
------------

Various kinds of ranges can be passed to scanning functions.

Fundamentally, a range is something that has a beginning and an end.
Examples of ranges are a string literal, a C-style array, and a ``std::vector``.
All of these can be passed to ``std::begin`` and ``std::end``, which then return an iterator to the range.
This notion of ranges was standardized in C++20 with the Ranges TS.
This library provides barebone support of this functionality.

Source range requirements
*************************

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

If the source range is contiguous, and/or its ``value_type`` is its character type,
various fast-path optimizations are enabled inside the library implementation.
Also, a ``string_view`` can only be scanned from such a range.

Character type
**************

The range has an associated character type.
This character type can be either ``char`` or ``wchar_t``.
The character type is determined by the result of ``operator*`` of the range
iterator. If dereferencing the iterator returns

 * ``char`` or ``wchar_t``: the character type is ``char`` or ``wchar_t``, respectively
 * ``expected<char>`` or ``expected<wchar_t>``: the character type is ``char`` or ``wchar_t``, respectively

Note on string literals
***********************

Please note, that only string literals are ranges (``const char(&)[N]``), not pointers to a constant character (``const char*``).
This is because:

 * It's impossible to differentiate if a ``const char*`` is a null-terminated string, a pointer to a single ``char``, or a pointer to an array of ``char``.
   For safety reasons, ``const char*`` is thus not an allowed source range type.
 * It's how ranges in the standard are defined: a ``const char*`` cannot be passed to ``std::ranges::begin`` or ``std::ranges::end``
   (it doesn't have a clear beginning or an end, for the reason explained above), so it's not even a range to begin with.

Therefore, this code is allowed, as it uses a string literal (``const char(&)[N]``) as the source range type:

.. code-block:: cpp

    int i;
    scn::scan_default("123", i);

But this code isn't, as the source range type used is not a range, but a pointer to constant character (``const char*``):

.. code-block:: cpp

    const char* source = "123";
    int i;
    scn::scan_default(source, i); // compiler error

This issue can be avoided by using a ``string_view``:

.. code-block:: cpp

    const char* source = "123";
    int i;
    scn::scan_default(scn::string_view{source}, i);
    // std::string_view would also work

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

Lists
-----

.. doxygenfunction:: scan_list
.. doxygenfunction:: scan_list_ex
.. doxygenfunction:: scan_list_localized

.. doxygenstruct:: scn::scan_list_options
    :members:
.. doxygenfunction:: list_separator
.. doxygenfunction:: list_until
.. doxygenfunction:: list_separator_and_until

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

Format string
-------------

Every value to be scanned from the source range is marked with a pair of
curly braces ``"{}"`` in the format string. Inside these braces, additional
options can be specified. The syntax is not dissimilar from the one found in
fmtlib.

The information inside the braces consist of two parts: the index and the
scanning options, separated by a colon ``':'``.

The index part can either be empty, or be an integer.
If the index is specified for one of the arguments, it must be set for all of
them. The index tells the library which argument the braces correspond to.

.. code-block:: cpp

    int i;
    std::string str;
    scn::scan(range, "{1} {0}", i, str);
    // Reads from the range in the order of:
    //   string, whitespace, integer
    // That's because the first format string braces have index '1', pointing to
    // the second passed argument (indices start from 0), which is a string

After the index comes a colon and the scanning options.
The colon only has to be there if any scanning options are specified.

For ``span`` s, there are no supported scanning options.

Integral types
**************

There are localization specifiers:

 * ``n``: Use thousands separator from the given locale
 * ``l``: Accept characters specified as digits by the given locale. Implies ``n``
 * (default): Use ``,`` as thousands separator
   (not accepted by default, though, use ``'`` to also parse thousands separators) and ``[0-9]`` as digits

And base specifiers:

 * ``d``: Decimal (base-10)
 * ``x``: Hexadecimal (base-16)
 * ``o``: Octal (base-8)
 * ``b..`` Custom base; ``b`` followed by one or two digits
   (e.g. ``b2`` for binary). Base must be between 2 and 36, inclusive
 * ``i`` and ``u``: Detect base.
   ``0x``/``0X`` prefix for hexadecimal,
   ``0`` prefix for octal, decimal otherwise.
   Argument must be signed (``i``) or unsigned (``u``), respectively.
 * (default): Decimal (base-10)

And other options:

 * ``'``: Accept thousands separator characters,
   as specified by the given locale
 * (default): Thousands separator characters aren't accepted

These specifiers can be given in any order, with up to one from each
category.

Floating-point types
********************

First, there's a localization specifier:

 * ``n``: Use decimal and thousands separator from the given locale
 * (default): Use ``.`` as decimal point and ``,`` as thousands separator

After that, an optional ``a``, ``A``, ``e``, ``E``, ``f``, ``F``, ``g`` or ``G`` can be
given, which has no effect.

``bool``
********

First, there are a number of specifiers that can be given, in any order:

 * ``a``: Accept only ``true`` or ``false``
 * ``n``: Accept only ``0`` or ``1``
 * ``l``: Implies ``a``. Expect boolean text values as specified as such by the
   given locale
 * (default): Accept ``0``, ``1``, ``true``, and ``false``, equivalent to ``an``

After that, an optional ``b`` can be given, which has no effect.

Strings (``std::string``, ``string_view``)
******************************************

Only supported option is ``s``, which has no effect

Characters (``char``, ``wchar_t``)
**********************************

Only supported option is ``c``, which has no effect

Whitespace
**********

Any amount of whitespace in the format string tells the library to skip until
the next non-whitespace character is found from the range. Not finding any
whitespace from the range is not an error.

Literal characters
******************

To scan literal characters and immediately discard them, just write the
characters in the format string. ``scanf``-like ``[]``-wildcard is not supported.
To read literal ``{`` or ``}``, write ``{{`` or ``}}``, respectively.

.. code-block:: cpp

    std::string bar;
    scn::scan("foobar", "foo{}", bar);
    // bar == "bar"

Semantics of scanning a value
-----------------------------

In the beginning, with every ``scn::scan`` (or similar) call, the library
wraps the given range in a ``scn::detail::range_wrapper``.
This wrapper provides an uniform interface and lifetime semantics over all possible ranges.
The arguments to scan are wrapped in a ``scn::arg_store``.
The appropriate context and parse context types are then constructed based on these values,
the format string, and the requested locale.

These are passed to ``scn::vscan``, which then calls ``scn::visit``.
There, the library calls ``begin()`` on the range, getting an iterator. This iterator is
advanced until a non-whitespace character is found.

After that, the format string is scanned character-by-character, until an
unescaped ``'{'`` is found, after which the part after the ``'{'`` is parsed,
until a ``':'`` or ``'}'`` is found. If the parser finds an argument id,
the argument with that id is fetched from the argument list, otherwise the
next argument is used.

The ``parse()`` member function of the appropriate ``scn::scanner``
specialization is called, which parses the parsing options-part of the format
string argument, setting the member variables of the ``scn::scanner``
specialization to their appropriate values.

After that, the ``scan()`` member function is called. It reads the range,
starting from the aforementioned iterator, into a buffer until the next
whitespace character is found (except for ``char``/``wchar_t``: just a single
character is read; and for ``span``: ``span.size()`` characters are read). That
buffer is then parsed with the appropriate algorithm (plain copy for
``string`` s, the method determined by the ``options`` object for ints and
floats).

If some of the characters in the buffer were not used, these characters are
put back to the range, meaning that ``operator--`` is called on the iterator.

Because how the range is read until a whitespace character, and how the
unused part of the buffer is simply put back to the range, some interesting
situations may arise. Please note, that the following behavior is consistent
with both ``scanf`` and ``<iostream>``.

.. code-block:: cpp

    char c;
    std::string str;

    // No whitespace character after first {}, no range whitespace is skipped
    scn::scan("abc", "{}{}", c, str);
    // c == 'a'
    // str == "bc"

    // Not finding whitespace to skip from the range when whitespace is found in
    // the format string isn't an error
    scn::scan("abc", "{} {}", c, str);
    // c == 'a'
    // str == "bc"

    // Because there are no non-whitespace characters between 'a' and the next
    // whitespace character ' ', ``str`` is empty
    scn::scan("a bc", "{}{}", c, str);
    // c == 'a'
    // str == ""

    // Nothing surprising
    scn::scan("a bc", "{} {}", c, str);
    // c == 'a'
    // str == "bc"

Using ``scn::scan_default`` is equivalent to using ``"{}"`` in the format string
as many times as there are arguments, separated by whitespace.

.. code-block:: cpp

    scn::scan_default(range, a, b);
    // Equivalent to:
    // scn::scan(range, "{} {}", a, b);

Files
-----

.. doxygenclass:: scn::basic_file
    :members:
.. doxygenclass:: scn::basic_owning_file
    :members:
.. doxygenclass:: scn::basic_mapped_file
    :members:

.. doxygentypedef:: file
.. doxygentypedef:: wfile

.. doxygentypedef:: owning_file
.. doxygentypedef:: owning_wfile

.. doxygentypedef:: mapped_file
.. doxygentypedef:: mapped_wfile

.. doxygenfunction:: stdin_range
.. doxygenfunction:: cstdin
.. doxygenfunction:: wcstdin

Lower level parsing and scanning operations
-------------------------------------------

``vscan``
*********

.. doxygenfunction:: vscan
.. doxygenfunction:: vscan_default
.. doxygenfunction:: vscan_localized
.. doxygenfunction:: visit(Context &ctx, ParseCtx &pctx, basic_args<typename Context::char_type> args) -> error

Low-level parsing
*****************

``parse_integer`` and ``parse_float`` will provide super-fast parsing from a string, at the expense of some safety and usability guarantees.
Using these functions can easily lead to unexpected behavior or UB if not used correctly and proper precautions are not taken.

.. doxygenfunction:: parse_integer
.. doxygenfunction:: parse_float

Scanner
*******

Values are eventually scanned using a ``scn::scanner``.

.. doxygenstruct:: scn::parser_base
    :members:
.. doxygenstruct:: scn::empty_parser
    :members:
.. doxygenstruct:: scn::common_parser
    :members:
.. doxygenstruct:: scn::common_parser_default
    :members:

.. doxygenfunction:: scan_usertype
.. doxygenfunction:: vscan_usertype

Low-level range reading
***********************

The following functions abstract away the source range in easier to understand parsing operations.

.. doxygenfunction:: read_code_unit
.. doxygenstruct:: scn::read_code_point_result
    :members:
.. doxygenfunction:: read_code_point
.. doxygenfunction:: read_zero_copy
.. doxygenfunction:: read_all_zero_copy
.. doxygenfunction:: read_into
.. doxygenfunction:: read_until_space_zero_copy
.. doxygenfunction:: read_until_space
.. doxygenfunction:: read_until_space_ranged
.. doxygenfunction:: putback_n
.. doxygenfunction:: skip_range_whitespace

Tuple scanning
--------------

.. doxygenfunction:: scan_tuple
.. doxygenfunction:: scan_tuple_default

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
