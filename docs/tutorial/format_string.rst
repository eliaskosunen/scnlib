=============
Format string
=============

Every value to be scanned from the input range is marked with a pair of
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
--------------

There are localization specifiers:

 * ``n``: Use thousands separator from the given locale
 * ``l``: Accept characters specified as digits by the given locale. Implies ``n``
 * (default): Use ``,`` as thousands separator and ``[0-9]`` as digits

And base specifiers:

 * ``d``: Decimal (base-10)
 * ``x``: Hexadecimal (base-16)
 * ``o``: Octal (base-8)
 * ``b..`` Custom base; ``b`` followed by one or two digits
   (e.g. ``b2`` for binary). Base must be between 2 and 36, inclusive
 * (default): Detect base. ``0x``/``0X`` prefix for hexadecimal,
   ``0`` prefix for octal, decimal by default
 * ``i``: Detect base. Argument must be signed
 * ``u``: Detect base. Argument must be unsigned

And other options:

 * ``'``: Accept thousands separator characters,
   as specified by the given locale (only with ``custom``-scanning method)
 * (default): Thousands separator characters aren't accepter

These specifiers can be given in any order, with up to one from each
category.

Floating-point types
--------------------

First, there's a localization specifier:
 * ``n``: Use decimal and thousands separator from the given locale
 * (default): Use ``.`` as decimal point and ``,`` as thousands separator

After that, an optional ``a``, ``A``, ``e``, ``E``, ``f``, ``F``, ``g`` or ``G`` can be
given, which has no effect.

``bool``
--------

First, there are a number of specifiers that can be given, in any order:

 * ``a``: Accept only ``true`` or ``false``
 * ``n``: Accept only ``0`` or ``1``
 * ``l``: Implies ``a``. Expect boolean text values as specified as such by the
    given locale
 * (default): Accept ``0``, ``1``, ``true``, and ``false``, equivalent to ``an``

After that, an optional ``b`` can be given, which has no effect.

Strings
-------

Only supported option is ``s``, which has no effect

Characters
----------

Only supported option is ``c``, which has no effect

Whitespace
----------

Any amount of whitespace in the format string tells the library to skip until
the next non-whitespace character is found from the range. Not finding any
whitespace from the range is not an error.

Literal characters
------------------

To scan literal characters and immediately discard them, just write the
characters in the format string. ``scanf``-like ``[]``-wildcard is not supported.
To read literal ``{`` or ``}``, write ``{{`` or ``}}``, respectively.

.. code-block:: cpp

    std::string bar;
    scn::scan("foobar", "foo{}", bar);
    // bar == "bar"

Default format string
---------------------

If you wish to not pass any custom parsing options, you should probably pass
a ``scn::default_tag`` instead. This will increase performance, as an useless
format string doesn't need to be parsed.

.. code-block:: cpp

    scn::scan(range, scn::default_tag, value);
    // Equivalent to:
    // scn::scan(range, "{}", value);
