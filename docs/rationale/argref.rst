================================
Why take arguments by reference?
================================

Relevant GitHub issue:
https://github.com/eliaskosunen/scnlib/issues/2

Another frequent complaint is how the library requires default-constructing
your arguments, and then passing them by reference.
A proposed alternative is returning the arguments as a tuple, and then
unpacking them at call site.

This is covered pretty well by the above GitHub issue, but to summarize:

 * ``std::tuple`` has measurable overhead (~5% slowdown)
 * it still would require your arguments to be default-constructible

To elaborate on the second bullet point, consider this example:

.. code-block:: cpp

    auto [result, i, str] =
        scn::scan_tuple<int, non_default_constructible_string>(
            range, scn::default_tag);

Now, consider what would happen if an error occurs during scanning the
integer. The function would need to return, but what to do with the string?
It *must* be default-constructed (``std::tuple`` doesn't allow
unconstructed members).

Would it be more convenient, especially with C++17 structured bindings?
One could argue that, and that's why an alternative API, returning a ``tuple``,
is available, in the header ``<scn/tuple_return.h>``.
The rationale of putting it in a separate header is to avoid pulling in the
entirety of very heavy standard headers ``<tuple>`` and ``<functional>``.

TODO: flesh this section out
