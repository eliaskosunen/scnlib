=============================
Semantics of scanning a value
=============================

In the beginning, with every ``scn::scan`` (or similar) call, the
library calls ``begin()`` on the range, getting an iterator. This iterator is
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

Using ``scn::default_tag`` is equivalent to using ``"{}"`` in the format string
as many times as there are arguments, separated by whitespace.

.. code-block:: cpp

    scn::scan(range, scn::default_tag, a, b);
    // Equivalent to:
    // scn::scan(range, "{} {}", a, b);
