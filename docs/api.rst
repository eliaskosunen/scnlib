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

Range wrapper
*************

.. doxygenclass:: scn::detail::range_wrapper
    :members:

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

The format of the format specifiers are as follows. ``[foo]`` means an optional flag.

::

    [[fill]align][width][L][type]

Fill and align
**************

Values to be parsed can be aligned in the source, using fill characters.
For example, an integer could be aligned in a field using asterisks, like ``"****42****"``.
This can be parsed without extra trickery, by specifying the fill character ``*``, and center alignment ``^``:

.. code-block:: cpp

    int i;
    auto ret = scn::scan("*****42*****", "{:*^}", i);
    // i == 42
    // ret.empty() == true

The fill character can be any character other than ``}``.
If no fill character is given, a space is used.

Supported alignment options are:

 * ``<``: for left-alignment (value is in the left of the field, fill characters come after it)
 * ``>``: for right-alignment
 * ``^``: for center-alignment -- fill characters both before and after the value.

Width
*****

Width specifies the maximum number of characters (= code units) that can be read from the source stream.
Width can be any unsigned integer.

.. code-block:: cpp

    std::string str{};
    auto ret = scn::scan("abcde", "{:3}", s);
    // str == "123"
    // ret.range() == "45"

Localized
*********

Specifying the ``L``-flag will cause the library to use localized scanning for this value.
If a locale was passed to the scanning function (for example, with ``scn::scan_localized``), it will be used.
Otherwise, the global C++ locale will be used (``std::locale{}``, set with ``std::locale::global()``).

.. code-block:: cpp

    double d{};

    // uses global locale ("C", because std::locale::global() hasn't been called)
    auto ret = scn::scan("3.14", "{:L}", d);
    // ret.empty() == true
    // d == 3.14

    // uses the passed-in locale
    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "3,14", "{:L}", d);
    // ret.empty() == true
    // d == 3.14

Itself, the ``L`` flag has an effect with floats, where it affects the accepted decimal separator.
In conjunction with other flags (``n`` and ``'``) it can have additional effects.

Type
****

The type flag can be used to further specify how the value is to be parsed, what values are accepted, and how they are interpreted.
The accepted flags and their meanings depend on the actual type to be scanned.

Type: integral
**************

For integral types the flags are organized in three categories.
Up to one from each category can be present in the format string.

First category:

 * ``b``: Binary, optional prefix ``0b`` or ``0B`` accepted
 * ``Bnn``: (``B`` followed by two digits): custom base, ``n`` in range 2-36 (inclusive), no base prefix accepted
 * ``c``: Read a code unit (do not skip preceding whitespace)
 * ``d``: Decimal, no base prefix accepted
 * ``i``: Integer, detect base by prefix: ``0b``/``0B`` for binary, ``0o``/``0O``/``0`` for octal, ``0x``/``0X`` for hex, decimal otherwise
 * ``u``: Unsigned decimal, no sign or base prefix accepted (even for signed types)
 * ``o``: Octal, optional prefix ``0o``, ``0O`` or ``0`` accepted
 * ``x``: Hex, optional prefix ``0x`` or ``0X`` accepted
 * (default): ``d`` if type is not ``char`` or ``wchar_t``, ``c`` otherwise

Second category (if the first category was not ``c``):

 * ``n``: Accept digits as specified by the supplied locale, implies ``L``
 * (default): Only digits ``[0-9]`` are accepted, no custom digits

Third category (if the first category was not ``c``):

 * ``'``: Accept thousands separators: default to ``,``, use locale if ``L`` set
 * (default): Only digits ``[0-9]`` are accepted, no thousands separator

Types considered 'integral', are the types specified by ``std::is_integral``, except for ``bool``, ``char8_t``, ``char16_t``, and ``char32_t``.
This includes signed and unsigned variants of ``char``, ``short``, ``int``, ``long``, ``long long``, and ``wchar_t``.

Type: float
***********

For floats (``float``, ``double`` and ``long double``), there are also three categories,
where up to one from each category can be present in the format string.

First category:

 * ``a`` and ``A``: Hex float accepted (e.g. ``0x1f.0p2``)
 * ``e`` and ``E``: Scientific format accepted (``123.456e2``)
 * ``f`` and ``F``: Fixed-precision format accepted (``123.456``)
 * ``g`` and ``G``: General format (implies ``e`` AND ``f``)
 * (default): Accept all (implies ``e`` AND ``f`` AND ``a``)

Second category:

 * ``n``: Accept digits as specified by the supplied locale, implies ``L``
 * (default): Only digits ``[0-9]`` are accepted, no custom digits

Third category:

 * ``'``: Accept thousands separators: default to ``,``, use locale if ``L`` set
 * (default): Only digits ``[0-9]`` are accepted, no thousands separator

Type: string
************

For strings (``std::basic_string``, ``scn::/std::basic_string_view``, ``scn::span``), the supported options are as follows:

 * ``s``: Accept any non-whitespace characters (if ``L`` is set, use the supplied locale, otherwise use ``std::isspace`` with ``"C"`` locale). Skips leading whitespace.
 * ``[`` *set* ``]``: Accept any characters as specified by _set_, further information below. Does not skip leading whitespace.
 * (default): ``s``

*set* can consist of literal characters (``[abc]`` only accepts ``a``, ``b``, and ``c``),
ranges of literal characters (``[a-z]`` only accepts characters from ``a`` to ``z``),
or specifiers, that are detailed in the table below.

Literals can also be specified as hex values:

 * ``\xnn``: ``\x`` followed by two hexadecimal digits -> hex value
 * ``\unnnn``: ``\u`` followed by four hexadecimal digits -> Unicode code point
 * ``\Unnnnnnnn``: ``\U`` followed by eight hexadecimal digits -> Unicode code point (max value ``0x10FFFF``)

.. list-table:: Specifiers
    :widths: 20 40 40
    :header-rows: 1

    * - Specifier
      - Description
      - Accepted characters
    * - ``:all:``
      - All characters
      - ``true``
    * - ``:alnum:``
      - Alphanumeric characters
      - ``std::isalnum``
    * - ``:alpha:``
      - Letters
      - ``std::isalpha``
    * - ``:blank:``
      - Blank characters
      - ``std::isblank`` (space and ``\t``)
    * - ``:cntrl:``
      - Control characters
      - ``std::iscntrl`` (``0x0`` to ``0x1f``, and ``0x7f``)
    * - ``:digit:``
      - Digits
      - ``std::isdigit``
    * - ``:graph:``
      - "Graphic" characters
      - ``std::isgraph``
    * - ``:lower:``
      - Lowercase letters
      - ``std::islower``
    * - ``:print:``
      - Printable characters
      - ``std::isprint`` (``std::isgraph`` + space)
    * - ``:punct:``
      - Punctuation characters
      - ``std::ispunct``
    * - ``:space:``
      - Whitespace characters
      - ``std::isspace``
    * - ``:upper:``
      - Uppercase letters
      - ``std::isupper``
    * - ``:xdigit:``
      - Hexadecimal digits
      - ``std::isxdigit``
    * - ``\l``
      - Letters
      - ``std::isalpha`` (= ``:alpha:``)
    * - ``\w``
      - Letters, numbers, and underscore
      - ``std::isalnum`` + ``_`` (= ``:alnum:`` + ``_``)
    * - ``\s``
      - Whitespace
      - ``std::isspace`` (= ``:space:``)
    * - ``\d``
      - Digits
      - ``std::isdigit`` (= ``:digit:``)

``\l``, ``\w``, ``\s`` and ``\d`` can be inverted with capitalization: ``\L``, ``\W``, ``\S`` and ``\D``, respectively.

If the first character in the set is ``^``, all options are inverted.

``\:``, ``\]``, ``\\^``, ``\\`` specify literal ``:``, ``]``, ``^``, and ``\``, respectively.

``-`` ranges accept any value numerically between the two ends,
e.g. ``[A-z]`` accepts every ascii value between ``0x41`` and ``0x7a``, including characters like ``[``, ``\``, and ``]``.
``[a-Z]`` is an error, because the range end must be greater or equal to its beginning.

If the ``L`` flag is used, the supplied locale is used.
If not, the ``<cctype>`` detailed in the above table is used, with the ``"C"`` locale.

.. list-table:: Example format strings
    :widths: 50 50
    :header-rows: 1

    * - Format string
      - Accepted characters
    * - ``"{:[abc]}"``
      - ``a``, ``b`` and ``c``
    * - ``"{:[a-z]}"``
      - ``std::islower``
    * - ``"{:[a-z-]}"``
      - ``std::islower`` + ``-``
    * - ``"{:[a-z\n]}"`` (1)
      - ``std::islower`` + ``\n``
    * - ``"{:[a-z\\\\]}"`` (2)
      - ``std::islower`` + ``\``
    * - ``"{:[a-zA-Z0-9]}"``
      - ``std::isalnum``
    * - ``"{:[^a-zA-Z0-9]}"``
      - ``!std::isalnum``
    * - ``"{:[:alnum:]}"``
      - ``std::isalnum``
    * - ``"{:[^:alnum:]}"``
      - ``!std::isalnum``
    * - ``"{:[\\w]}"`` (3)
      - ``std::isalnum`` + ``_``
    * - ``"{:[^\\w]}"``
      - NOT ``std::isalnum)`` + ``_``
    * - ``"{:[\\W]}"``
      - NOT ``std::isalnum`` + ``_``
    * - ``"{:[^\\W]}"``
      - ``std::isalnum`` + ``_``

(1): ``\n`` means literal line break 0x0a

(2): Note the quadruple backslash: ``\\\\`` is turned into ``[0x5c 0x5c]`` (two backslash characters) in the actual string by C++,
which, in turn, is interpreted as an escaped backslash by scnlib.
Just a double backslash ``\\]`` would lead to the closing square parenthesis being escaped, and the format string being invalid.

(3): Same as above: ``\\w`` means the format string specifier ``\w``, literal ``\w`` would be an invalid C++ escape sequence.

Type: bool
**********

Any number of flags accepted

 * ``s``: Accept string values (``true`` and ``false`` AND possible locale values, if using ``L``)
 * ``i``: Accept int values (``0`` and ``1``)
 * ``n``: ``i``, except accepts localized digits, implies ``L``
 * (default): ``s`` + ``i``: Accept ``0``, ``1``, ``true``, ``false``, and possible locale string values if using ``L``

Type: code_point
****************

Only flag ``c`` accepted, does not affect behavior.

Note, ``code_point``, leading whitespace is not skipped.

Whitespace
**********

Any amount of whitespace in the format string tells the library to skip until the next non-whitespace character is found from the range.
Not finding any whitespace from the range is not an error.

Literal characters
******************

To scan literal characters and immediately discard them, just write the
characters in the format string.
To read literal ``{`` or ``}``, write ``{{`` or ``}}``, respectively.

.. code-block:: cpp

    std::string bar;
    scn::scan("foobar", "foo{}", bar);
    // bar == "bar"

Semantics of scanning a value
-----------------------------

In the beginning, with every ``scn::scan`` (or similar) call, the library
wraps the given range in a ``scn::detail::range_wrapper``, using ``scn::wrap``.
This wrapper provides an uniform interface and lifetime semantics over all possible ranges.
The arguments to scan are wrapped in a ``scn::arg_store``.
These are then passed, alongside the format string, to ``scn::vscan`` (or similar).

The appropriate context and parse context types are then constructed based on these values,
the format string, and the requested locale, and ``scn::visit`` is called.
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
buffer is then parsed with the appropriate algorithm.

If some of the characters in the buffer were not used, these characters are
put back to the range, meaning that ``operator--`` is called on the iterator.

Because how the range is read until a whitespace character, and how the
unused part of the buffer is simply put back to the range, some interesting
situations may arise. Please note, that the following behavior is consistent
with both ``scanf`` and ``<iostream>``.

.. code-block:: cpp

    // chars do not skip leading whitespace by default
    // strings do
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

    // string scanners skip leading whitespace
    scn::scan("a bc", "{}{}", c, str);
    // c == 'a'
    // str == "bc"

    // char scanners do not
    scn::scan("ab c", "{}{}", str, c);
    // str == "ab"
    // c == ' '

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

.. doxygenfunction:: make_scan_result
.. doxygenfunction:: make_args_for

``vscan``
*********

.. doxygenfunction:: vscan
.. doxygenfunction:: vscan_default
.. doxygenfunction:: vscan_localized
.. doxygenstruct:: vscan_result
    :members:

.. doxygenfunction:: visit(Context &ctx, ParseCtx &pctx, basic_args<typename Context::char_type> args)

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
