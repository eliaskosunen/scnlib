============================
Migration guide v0.4 -> v1.0
============================

This guide outlines the breaking changes between 0.4 and 1.0, and how to migrate to 1.0.

Format strings
--------------

The new syntax for format string specifiers is
``[[fill]align][width][L][type]``.

This means, that the old localization flag ``l`` has been superseded by ``L``, that must come in a specific point in the format string.
"Type" encompasses all type-specific options.

Integers
********

``n`` is now used to accept locale-specific digits, implying ``L``.

``b`` now means binary. Custom base can be specified with ``Bnn``.

``i`` no longer requires the argument to be signed. It is now the only option, which detects the base of the number based on the input.

``u`` no longer detects the base of the number, but expects it to be decimal. It also does not accept any preceding sign.

The default for integers is now ``d`` (decimal), not ``i``/``u`` (detect base).

Accepted base prefixes now include ``0b`` and ``0B`` for binary, and ``0o`` and ``0O`` for octal.

.. list-table:: Integers
    :widths: 50 50
    :header-rows: 1

    * - v0.4
      - v1.0
    * - ``l``
      - ``n`` or ``Ln``
    * - ``n``
      - ``L``
    * - (default)
      - ``i``
    * - ``u``
      - ``i`` (1)
    * - ``b2``
      - ``b`` or ``B02``
    * - ``b36``
      - ``B36``
    * - ``d``
      - ``d`` or (default)

(1): ``i`` in 1.0 accepts both signed and unsigned values

Floats
******

``a``, ``A``, ``e``, ``E``, ``f``, ``F``, ``g``, and ``G`` had no effect in 0.4.
In 1.0 they each have a meaning, see API docs for more.

To get the old behavior, replace the above flag with ``g`` or ``G``.
This is also the same behavior if no type flag is given.

.. list-table:: Floats
    :widths: 50 50
    :header-rows: 1

    * - v0.4
      - v1.0
    * - ``a`` (or any other flag)
      - ``g`` or ``G`` or (default)
    * - (default)
      - (default, no changes)

Buffers/spans
*************

Spans now behave like strings, except they have a built-in size limitation in them.

In 0.4, reading a span would just mean reading as many bytes as is the size of the span.

.. code-block:: cpp

    // 0.4
    std::vector<char> vec(5, '\0');
    auto span = scn::make_span(vec);
    auto result = scn::scan("ab cde", "{}", span);
    // result.range() == "e"
    // vec == "ab cd"

In 1.0, reading a span is like reading a string, except it can't grow

.. code-block:: cpp

    // 1.0, same code
    std::vector<char> vec(5, '\0');
    auto span = scn::make_span(vec);
    auto result = scn::scan("ab cde", "{}", span);
    // result.range() == " cde"
    // vec == "ab"

To achieve the same behavior, use the new width field in the format string, alongside the new string set functionality:

.. code-block:: cpp

    // 1.0, same behavior
    std::vector<char> vec(5, '\0');
    auto span = scn::make_span(vec);
    // max 5 chars, accept all chars
    auto result = scn::scan("ab cde", "{:5[:all:]}", span);
    // result.range() == "e"
    // vec == "ab cd"

Or, if the range is direct, just use ``std::copy``:

.. code-block:: cpp

    // 1.0, same behavior
    std::vector<char> vec(5, '\0');
    auto span = scn::make_span(vec);
    auto source = std::string{"ab cde"};
    auto it = std::copy(source.begin(), source.end(), span.begin());
    // it = "e"
    // vec == "ab cd"

Whitespace skipping
*******************

In 0.4, the rules whether whitespace was to be skipped before a value were based on the format string, and they were complicated.

In 1.0, whitespace is always skipped, except when scanning characters or a `code_point`, or when using string set scanning with strings (the ``[option]``)

Encoding
--------

In 0.4, narrow strings (format strings, ranges) were assumed to be ASCII, and wide strings whatever the current locale said.

In 1.0, all strings are assumed to be Unicode, regardless of locale: narrow strings are UTF-8, wide strings are UTF-16 (Windows) or UTF-32 (POSIX).

Having the global locale, or the locale passed to ``scn::scan_localized``, be something other than UTF-8, is unsupported.

Lists
-----

``scn::scan_list`` and ``scn::scan_list_until`` are replaced by ``scn::scan_list`` and ``scn::scan_list_ex``, augmented by ``scn::scan_list_options`` and ``scn::scan_list_localized``.

``scn::scan_list`` without a separator and an until-character work the same in 0.4 and 1.0.

If a separator and/or an until-character support is desired, ``scn::scan_list_ex`` is to be used.
It takes an instance of a ``scn::scan_list_options``, which can be made with ``scn::list_separator``, ``scn::list_until``, and ``scn::list_separator_and_until``

.. code-block:: cpp

    std::vector<int> vec{};

    // basic scan_list staying the same
    // 0.4
    auto result = scn::scan_list("123 456", vec);
    // 1.0
    auto result = scn::scan_list("123 456", vec);
    // vec == [123, 456]
    // result.empty() == true

    // separator
    // 0.4
    auto result = scn::scan_list("123, 456", vec);
    // 1.0
    auto result = scn::scan_list_ex("123, 456", scn::list_separator(','));
    // vec == [123, 456]
    // result.empty() == true

    // until
    // 0.4
    auto result = scn::scan_list_until("123 456\n789", vec, '\n');
    // 1.0
    auto result = scn::scan_list_ex("123 456\n789", vec, scn::list_until('\n'));
    // vec == [123, 456]
    // result.range() == "789"

    // separator + until
    // 0.4
    auto result = scn::scan_list_until("123, 456,\n789", vec, '\n', ',');
    // 1.0
    auto result = scn::scan_list_ex("123, 456,\n789", vec, scn::list_separator_and_until(',', '\n'));
    // vec == [123, 456]
    // result.range() == "789"

Remaining range in results
--------------------------

Following member functions in ``result`` were renamed:

 * ``string()`` -> ``range_as_string()``
 * ``span()`` -> ``range_as_span()``
 * ``string_view()`` -> ``range_as_string_view()``

Other, more minor stuff
-----------------------

``vscan`` signature
*******************

The signature of ``vscan`` was changed: stuff previously done in ``scan`` is now done in ``vscan``.
In 0.4, ``vscan`` had the signature ``(Context&, ParseContext&, args) -> error``.
In 1.0, ``vscan`` has the signature ``(WrappedRange, format_string, args) -> vscan_result``

Newly stabilized ``scn::wrap``, ``scn::make_context``, and ``scn::make_parse_context`` will help with implementing new ``scan`` and ``vscan`` functions.
Implementing ``scan`` was not really possible in 0.4 without using internal APIs, but now it is.

Its functionality was also split into ``vscan``, ``vscan_default`` and ``vscan_localized``.

.. code-block:: cpp

    // scan in 1.0
    template <typename Range, typename... Args>
    auto scan(Range&& r, string_view f, Args&... a) {
        auto range = scn::wrap(std::forward<Range>(r));
        auto args = scn::make_args_for(range, f, a...);
        auto ret = scn::vscan(std::move(range), f, {args});
        return scn::make_scan_result(std::move(ret));
    }


``read_char()`` -> ``read_code_unit()``
***************************************

``read_char`` was renamed to better reflect the Unicode-awareness of the library.
Its behavior is identical.

``scn::scan_usertype``
**********************

While not a breaking change, ``scn::scan_usertype`` increases the ergonomics of scanning a user-defined type. See the API docs for more.
