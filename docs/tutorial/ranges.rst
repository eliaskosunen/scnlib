======
Ranges
======

We can, of course, read from other sources than ``stdin``.
In fact, with scnlib, we can read from any ``range``, as long as it fulfills
certain requirements. If you're not familiar with C++20 ranges, don't worry;
conceptually, they're quite simple. A range is simply something that one can
call ``begin()`` and ``end()`` on. For example, a ``std::string`` or a
``std::vector`` are ranges.

The library can't work with every range, though.
Most importantly, it needs to be a ``view``, meaning that it doesn't own its
elements, and is fast to copy. Examples of ``view`` s are ``std::string_view`` and
``std::span``.

This range can then be passed as the first parameter to ``scn::scan``:

.. code-block:: cpp

    int i;
    // A string literal is something one _can_ pass to scn::scan
    scn::scan("42", "{}", i);
    // i == 42

``scn::scan`` takes the input by forwarding reference.
This means, that if it's given a modifiable lvalue (``T&``),
the same variable can easily be used in multiple calls to ``scn::scan``.

.. code-block:: cpp

    auto input = scn::string_view("123 foo");

    int i;
    scn::scan(input, "{}", i);
    // i == 123
    // input == " foo"

    std::string str;
    scn::scan(input, "{}", str);
    // str == "foo"
    // input is empty

A convenience function, ``scn::make_view``, is provided,
which makes converting a range to an appropriate ``view`` easier.

.. code-block:: cpp

    std::string str = ...;
    auto view = scn::make_view(str);

    scn::scan(view, ...);

Note, that ``const char*`` is _not_ a range, but ``const char(&)[N]`` is.
This has the unfortunate consequence that this works:

.. code-block:: cpp

    // "foo" is a const char(&)[4]
    scn::scan("foo", ...);

But this doesn't:

.. code-block:: cpp

    auto str = "foo";
    // str is a const char*
    scn::scan(str, ...);
    // Error will be along the lines of
    // "Cannot call begin on a const char*"

This is caused by the way string literals and array decay work in the
language.

This can be worked around with ``scn::make_view``:

.. code-block:: cpp

    auto str = scn::make_view("foo");
    // str is a scn::string_view
    scn::scan(str, ...);

Reading from files is also supported, with a range wrapping a ``FILE*``.

.. code-block:: cpp

    auto f = std::fopen(...);
    // Non-owning wrapper around a FILE*
    auto file = scn::file(f);
    // Alternatively, use owning_file (takes similar arguments to fopen)
    scn::owning_file file(...);
    // To get a view out of a file, use .lock()
    // A file can be locked multiple times, but not in separate threads
    scn::scan(file.lock(), ...);
    // Locking/unlocking can be expensive, so you can do it once in block scope
    {
      auto lock = file.lock();
      scn::scan(lock, ...);
      scn::scan(lock, ...);
      // When the lock is released, the file is synced with <cstdio>
      // You can also do this explicitly with .sync(),
      // if you want to mix and match
      lock.sync();
    }
    // scn::file doesn't take ownership, and doesn't close
    // scn::owning_file does
    std::fclose(f);

``scn::cstdin()`` returns a ``scn::file`` pointing to ``stdin``.
Note, that this object is effectively a global,
and needs to be ``.lock()``ed in order to be used in ``scn::scan``
