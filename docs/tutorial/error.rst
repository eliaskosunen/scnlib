==============
Error handling
==============

``scnlib`` does not use exceptions for error handling.
Instead, ``scn::scan`` and others return a
``scn::scan_result``, which is an object that contains:

 * an integer, telling the number of arguments successfully read
 * a range, denoting the unused part of the input range
 * an ``scn::error`` object

.. code-block:: cpp

    // successful read:
    int i{}
    auto ret = scn::scan("42 leftovers", "{}", i);
    // ret == true
    // ret.value() == 1
    // ret.range() == " leftovers"
    // ret.error() == true

    // failing read:
    int i{};
    auto ret = scn::scan("foo", "{}", i);
    // ret == false
    // ret.value() == 0
    // ret.range() == "foo"
    // ret.error() == false

The ``scn::error`` object can be examined further. It contains an error code
``scn::error::code``, accessible with member function ``code()`` and a message,
that can be get with ``msg()``.

.. code-block:: cpp

    auto ret = scn::scan(range, "{}", value);
    if (!ret) {
        std::cout << "Read failed with message: '" << ret.error().msg() << "'\n";
    }

Please note, that EOF is also an error, with error code
``scn::error::end_of_range``.

If the error is of such quality that it cannot be recovered from, the range
becomes *bad*, and the member function ``is_recoverable()`` of
``scn::error`` will return ``false``. This means, that the range is unusable and
in an indeterminate state.

See ``scn::error`` for more details about the error codes.

Error guarantees
----------------

Should the reading of any of the arguments fail, and the range is not bad,
the state of the range will be reset to what it was before the reading of
said argument. Also, the argument will not be written to.

.. code-block:: cpp

    int i{}, j{};
    // "foo" cannot be read to an integer, so this will fail
    auto ret = scn::scan("123 foo", "{} {}", i, j);
    assert(!ret);
    // First read succeeded
    assert(ret.value() == 1);
    assert(i == 123);
    // Second read failed, value was not touched
    assert(j == 0);
    assert(ret.error().code() == scn::error::invalid_scanned_value);
    // std::string so operator== works
    assert(ret.range() == std::string{" foo"});

    // The range now contains "foo",
    // as it was reset to the state preceding the read of j
    std::string s{};
    ret = scn::scan(ret.range(), "{}", s);
    // This succeeds
    assert(ret);
    assert(ret.value() == 1);
    assert(s == "foo");
    assert(ret.range().empty() == true);

Exceptions
----------

No exceptions will ever be thrown by ``scnlib`` functions (save for a
``std::bad_alloc``, but that's probably your fault).
Should any user-defined operations, like ``operator*`` on an iterator, or
``operator>>``, throw, the behavior is undefined.

The library can be compiled with ``-fno-exceptions`` and ``-fno-rtti``.
