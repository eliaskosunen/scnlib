=============================
``scanf``-like format strings
=============================

With ``scn::scanf``, a ``scanf``-like format string syntax can be used, instead.
``scn::ranges::scanf`` is also available. The syntax is not 100% compatible
with C ``scanf``, as it uses the exact same options as the regular format
string syntax. The following snippet demonstrates the syntax.

.. code-block:: cpp

    int i;
    double d;
    std::string s;
    scn::scanf(range, "%i %f %s", i, d, s);
    // How C scanf would do it:
    //   scanf(range, "%i %lf", &i, &d);
    //   reading a dynamic-length string is not possible with scanf
    // How scn::scan would do it:
    //   scn::scan(range, "{} {} {}", i, d, s);
    //   or to be more explicit:
    //   scn::scan(range, "{:i} {:f} {:s}", i, d, s);

Notice, how the options map exactly to the ones used with ``scn::scan``:
``%d -> {:d}``, ``%f -> {:f}`` and ``%s -> {:s}``; and how the syntax is not fully
compatible with C ``scanf``: "%f != %lf", ``scanf`` doesn't support
dynamic-length strings.

To read literal a ``%``-character and immediately discard it, write ``%%`` (``{{``
and ``}}`` with default format string syntax).
