=================
API Documentation
=================

The public API of scnlib consists of these headers:

 * ``scn/fwd.h``: Forward declarations
 * ``scn/scan.h``: Scanning API
 * ``scn/istream.h``: Support for scanning user-defined types with an ``operator>>``
 * ``scn/ranges.h``: (Experimental) Scanning of range types
 * ``scn/xchar.h``: Support for ``wchar_t``
 * ``scn/all.h``: All of the above

Basic Scanning API
------------------

The following functions use a format string syntax similar to that of ``std::format``:
:ref:`Format Strings`.

When taking a range ``source`` as input, the range must model the ``scannable_range`` concept:
:ref:`Scannable Ranges`.

.. doxygenfunction:: scan(Source&& source, format_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: scan_value(Source&& source) -> scan_result_type<Source, T>

.. doxygenfunction:: input(format_string<Args...> format) -> scan_result_type<scn::istreambuf_view&, Args...>

.. doxygenfunction:: prompt(const char* msg, format_string<Args...> format) -> scan_result_type<scn::istreambuf_view&, Args...>

Scannable Ranges
----------------

A range is considered scannable, if it models at least ``forward_range``, and its character type is correct
(its value type is the same with the format string).
If the range additionally models ``contiguous_range`` and ``sized_range``, additional optimizations are enabled.

.. code-block:: cpp

    template <typename Range, typename CharT>
    concept scannable_range =
        ranges::forward_range<Range> &&
        std::same_as<ranges::range_value_t<Range>, CharT>;

To turn an ``input_range`` into a ``scannable_range``, ``scn::caching_view`` can be used:

.. doxygenclass:: scn::basic_caching_view
    :members:

Result Types
------------

.. doxygenclass:: scn::scan_error
    :members:
    :undoc-members:

``scan`` and others return an object of type ``scan_result``, wrapped in a ``scan_expected``.
`std::expected on cppreference <https://en.cppreference.com/w/cpp/utility/expected>`_.

.. doxygenstruct:: scan_expected
    :members:
    :undoc-members:

.. doxygenclass:: scn::expected
    :members:

.. doxygentypedef:: scan_result_type

.. doxygenfunction:: make_scan_result(scan_expected<ResultRange>&& result, scan_arg_store<Context, Args...>&& args)

.. doxygenclass:: scn::scan_result
    :members:
    :undoc-members:

Format Strings
--------------

The format string syntax is heavily influenced by {fmt} and ``std::format``, and is largely compatible with it.
Scanning functions, such as ``scn::scan`` and ``scn::input``, use the format string syntax described in this section.

Format strings consist of:

 * Replacement fields, which are surrounded by curly braces ``{}``.

 * Non-whitespace characters (except ``{}``; for literal braces, use ``{{`` and ``}}``),
   which consume exactly one identical character from the input

 * Whitespace characters, which consume any and all available consecutive whitespace from the input.

Literal characters are matched by code point one-to-one, with no normalization being done.
``Ä`` (U+00C4, UTF-8 0xc3 0x84) only matches another U+00C4, and not, for example,
U+00A8 (DIAERESIS) and U+0041 (LATIN CAPITAL LETTER A).

Characters are considered to be whitespace characters as if by calling ``isspace`` with the ``"C"`` locale
(so, ``"\t\n\v\f\r"`` and SPACE).

The grammar for a replacement field is as follows:

.. code-block::

    replacement-field   ::= '{' [arg-id] [':' format-spec] '}'
    arg-id              ::= positive-integer

    format-spec         ::= [width] ['L'] [type]
    width               ::= positive-integer
    type                ::= 'a' | 'A' | 'b' | 'B' | 'c' | 'd' | 'e' | 'E' | 'f' | 'F' |
                            'g' | 'G' | 'o' | 'p' | 's' | 'x' | 'X' | 'i' | 'u'

Argument IDs
************

The ``arg-id`` specifier can be used to index arguments manually.
If manual indexing is used, all of the indices in a format string must be stated explicitly.
The same ``arg-id`` can appear in the format string only once, and must refer to a valid argument.

.. code-block:: cpp

    // Format string equivalent to "{0} to {1}"
    auto a = scn::scan<int, int>("2 to 300", "{} to {}");
    // a->values() == (2, 300)

    // Manual indexing
    auto b = scn::scan<int, int>("2 to 300", "{1} to {0}");
    // b->values() == (3, 200)

    // INVALID:
    // Automatic and manual indexing is mixed
    auto c = scn::scan<int, int>("2 to 300", "{} to {0}");

    // INVALID:
    // Same argument is referred to multiple times
    auto d = scn::scan<int, int>("2 to 300", "{0} to {0}");

    // INVALID:
    // {2} does not refer to an argument
    auto e = scn::scan<int, int>("2 to 300", "{0} to {2}");

Width
*****

Width specifies the maximum number of characters that will be read from the source range.
It can be any unsigned integer. When using the ``'c'`` type specifier for strings, specifying the width is required.

.. code-block:: cpp

    auto r = scn::scan<std::string>("abcde", "{:3}");
    // r->value() == "abc"

For the purposes of width calculation, the same algorithm is used that in {fmt}.
Every code point has a width of one, except the following ones have a width of 2:

 * any code point with the East_Asian_Width="W" or East_Asian_Width="F" Derived Extracted Property as described by UAX #44 of the Unicode Standard
 * U+4dc0 – U+4dff (Yijing Hexagram Symbols)
 * U+1f300 – U+1f5ff (Miscellaneous Symbols and Pictographs)
 * U+1f900 – U+1f9ff (Supplemental Symbols and Pictographs)

Localized
*********

The ``L`` flag enables localized scanning.
Its effects are different for each type it is used with:

 * For integers, it enables locale-specific thousands separators
 * For floating-point numbers, it enables locale-specifi thousands and radix (decimal) separators
 * For booleans, it enables locale-specific textual representations (for ``true`` and ``false``)
 * For other types, it has no effect

Type specifier
**************

The type specifier determines how the data is to be scanned.
The type of the argument to be scanned determines what flags are valid.

Type specifier: strings
#######################

.. list-table:: String types (``std::basic_string``, and ``std::basic_string_view``)
    :widths: 20 80
    :header-rows: 1

    * - Type
      - Meaning
    * - none, ``s``
      - Copies from the input until a whitespace character is encountered. Preceding whitespace is skipped.
    * - ``c``
      - Copies from the input until the field width is exhausted. Does not skip preceding whitespace.
        Errors if no field width is provided.

Type specifier: integers
########################

Integer values are scanned as if by using ``std::from_chars``, except:

 * A positive ``+`` sign and a base prefix (like ``0x``) are always allowed to be present
 * Preceding whitespace is skipped.

.. list-table:: Integer types (``signed`` and ``unsigned`` variants of ``char``, ``short``, ``int``, ``long``, and ``long long``)
    :widths: 20 80
    :header-rows: 1

    * - Type
      - Meaning
    * - ``b``, ``B``
      - ``from_chars`` with base 2. The base prefix is ``0b`` or ``0B``.
    * - ``o``, ``O``
      - ``from_chars`` with base 8. The base prefix is ``0o``, ``0O``, or just ``0``.
    * - ``x``, ``X``
      - ``from_chars`` with base 16. The base prefix is ``0x``, ``0X``.
    * - ``d``
      - ``from_chars`` with base 10. No base prefix.
    * - ``u``
      - ``from_chars`` with base 10. No base prefix. No ``-`` sign allowed.
    * - ``i``
      - Detect the base from a possible prefix, defaulting to decimal (base-10).
    * - ``rXX`` (from 2 to 36)
      - Custom base, without a base prefix (r is for radix).
    * - ``c``
      - Copies a character (code unit) from the input.
    * - none
      - Same as ``d``.

Type specifier: characters
##########################

.. list-table:: Character types (``char`` and ``wchar_t``), and code points (``char32_t``)
    :widths: 20 80
    :header-rows: 1

    * - Type
      - Meaning
    * - none, ``c``
      - Copies a character (code point for ``char32_t``, code unit otherwise) from the input.
    * - ``b``, ``B``, ``d``, ``i``, ``o``, ``O``, ``u``, ``x``, ``X``
      - Same as for integers, see above. Not allowed for ``char32_t``.

Note, that when scanning characters (``char`` and ``wchar_t``), the source range is read a single code unit at a time,
and encoding is not respected.

Type specifier: floating-point values
#####################################

Floating-point values are scanned as if by using ``std::from_chars``, except:

 * A positive ``+`` sign and a base prefix (like ``0x``) are always allowed to be present
 * Preceding whitespace is skipped.

.. list-table:: Floating-point types (``float``, ``double``, and ``long double``)
    :widths: 20 80
    :header-rows: 1

    * - Type
      - Meaning
    * - ``a``, ``A``
      - ``from_chars`` with ``chars_format::hex``. Prefix ``0x``/``0X`` is allowed.
    * - ``e``, ``E``
      - ``from_chars`` with ``chars_format::scientific``.
    * - ``f``, ``F``
      - ``from_chars`` with ``chars_format::fixed``.
    * - ``g``, ``G``
      - ``from_chars`` with ``chars_format::general``.
    * - none
      - ``from_chars`` with ``chars_format::general | chars_format::hex``. Prefix ``0x``/``0X`` is allowed.

Type specifier: booleans
########################

.. list-table:: ``bool``
    :widths: 20 80
    :header-rows: 1

    * - Type
      - Meaning
    * - ``s``
      - Allows the textual representation (``true`` or ``false``).
    * - ``b``, ``B``, ``d``, ``i``, ``o``, ``O``, ``u``, ``x``, ``X``
      - Allows the integral/numeric representation (``0`` or ``1``).
    * - none
      - Allows for both the textual and the integral/numeric representation.

Type-Erased Scanning API
------------------------

.. doxygentypedef:: vscan_result

.. doxygentypedef:: scan_args_for

.. doxygentypedef:: scan_arg_for

.. doxygenfunction:: vscan(Range&& range, std::string_view format, scan_args_for<Range, char> args) -> vscan_result<Range>

.. doxygenfunction:: vscan_value(Range&& range, scan_arg_for<Range, char> arg) -> vscan_result<Range>

Contexts and Scanners
---------------------

.. doxygenclass:: scn::basic_scan_context
    :members:
    :undoc-members:

.. doxygenclass:: scn::basic_scan_parse_context
    :members:
    :undoc-members:

.. doxygenstruct:: scn::scanner

Localization
------------

.. doxygenfunction:: scan(const Locale &loc, Source &&source, format_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: vscan(const Locale& loc, Range&& range, std::string_view format, scan_args_for<Range, char> args) -> vscan_result<Range>

Wide Character APIs
-------------------

.. doxygenfunction:: scn::scan(Source&& source, wformat_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: scn::input(wformat_string<Args...> format) -> scan_result_type<wistreambuf_view&, Args...>

.. doxygenfunction:: scn::prompt(const wchar_t* msg, wformat_string<Args...> format) -> scan_result_type<wistreambuf_view&, Args...>

.. doxygenfunction:: scn::scan(const Locale &loc, Source&& source, wformat_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: vscan(Range&& range, std::wstring_view format, scan_args_for<Range, wchar_t> args) -> vscan_result<Range>

.. doxygenfunction:: vscan_value(Range&& range, scan_arg_for<Range, wchar_t> arg) -> vscan_result<Range>

.. doxygenfunction:: vscan(const Locale& loc, Range&& range, std::wstring_view format, scan_args_for<Range, wchar_t> args) -> vscan_result<Range>
