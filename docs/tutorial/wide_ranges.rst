===========
Wide ranges
===========

Ranges can also be wide (terminology borrowed from iostreams), meaning that
their character type is ``wchar_t`` instead of ``char``. This has some usage
implications.

The format string must be wide:

.. code-block:: cpp

    scn::scan(range, L"{}", value);

``char`` s and ``std::string`` s cannot be read from a wide range, but ``wchar_t`` s
and ``std::wstring`` s can.

.. code-block:: cpp

    std::wstring word;
    scn::scan(range, L"{}", word);

Ranges with character types other that ``char`` and ``wchar_t`` are not
supported, due to lacking support for them in the standard library.
Converting between character types is out-of-score for this library.

Encoding and Unicode
--------------------

Because of the rather lackluster Unicode support of the standard library,
this library doesn't have any significant Unicode support either.

Narrow ranges are expected to be ASCII encoded, and using multibyte
encodings (like UTF-8) with them is probably going to cause problems (blame
``std::locale``). If you need some sort of Unicode support, your best bet is
going to be wide ranges, encoded in the way your platform expects (UTF-32 in
POSIX, the thing resembling UCS-2 in Windows)
