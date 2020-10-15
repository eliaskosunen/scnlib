====================
Scanning temporaries
====================

``scnlib`` provides a helper type for scanning into a temporary value:
``scn::temporary``. which can be created with the helper function ``scn::temp``.
This is useful, for example, for scanning a ``scn::span``.

.. code-block:: cpp

    // Doesn't work, because arguments must be lvalue references
    scn::scan(range, "{}", scn::make_span(...));

    // Workaround
    auto span = scn::make_span(...);
    scn::scan(range, "{}", span);

    // Using scn::temporary
    // Note the () at the end
    scn::scan(range, "{}", scn::temp(scn::make_span(...))());
