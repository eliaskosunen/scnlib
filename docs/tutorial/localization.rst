============
Localization
============

To scan localized input, a ``std::locale`` can be passed as the first argument
to ``scn::scan_localized``.

.. code-block:: cpp

    auto loc = std::locale("fi_FI");

    int a, b;
    scn::scan_localized(
        loc,
        range, "{} {:n}", a, b);

Only reading of ``b`` will be localized, as it has ``{:n}`` as its format string.
