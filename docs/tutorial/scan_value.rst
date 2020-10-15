==============
``scan_value``
==============

If you only wish to scan a single value with all default options, you can
save some cycles and use ``scn::scan_value``. Instead of taking its argument by
reference, it returns the read value. It is functionally equivalent to
``scn::scan(range, scn::default_tag, value)``.

.. code-block:: cpp

    auto ret = scn::scan_value<int>("42 leftovers");
    // ret == true
    // ret.value() == 42
    // ret.range() == " leftovers"
