==================================================
Why take just views? Why not every possible range?
==================================================

First off, it's not possible to take every ``range``; ``operator--`` is
required for error recovery, so at least ``bidirectional_range`` is needed.

``view`` s have clearer lifetime semantics, and make it more difficult to write
less performant code.

.. code-block:: cpp

    std::string str = "verylongstring";
    auto ret = scn::scan(str, ...);
    // str would have to be reallocated and its contents moved
