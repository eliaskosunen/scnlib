======================================================================
What's with all the ``vscan``, ``basic_args`` and ``arg_store`` stuff?
======================================================================

This approach is borrowed (*cough* stolen *cough*) from fmtlib, for the same
reason it's in there as well. Consider this peace of code:

.. code-block:: cpp

    int i;
    std::string str;

    scn::scan(range, scn::default_tag, i, str);
    scn::scan(range, scn::default_tag, str, i);

If the arguments were not type-erased, almost all of the internals would have
to be instantiated for every given combination of argument types.
