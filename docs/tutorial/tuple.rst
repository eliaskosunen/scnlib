===========================
Alternative tuple-based API
===========================

By including ``<scn/tuple_return.h>`` an alternative API becomes available,
returning a ``std::tuple`` instead of taking references.

.. code-block:: cpp

    // Use structured bindings with C++17
    auto [result, i] = scn::scan_tuple<int>(range, "{}");
    // result is a ``scan_result``, similar to the return value of ``scn::scan``
    // Error handling is further touched upon later
    // i is an ``int``, scanned from the range

