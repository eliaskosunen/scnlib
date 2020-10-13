===========
Input range
===========

This page is about the types of ranges that can be passed as inputs to
various functions within scnlib.

Fundamentally, a range is something that has a beginning and an end. Examples
of ranges are string literals, arrays, and ``std::vector``s. All of these can
be passed to ``std::begin`` and ``std::end``, which then return an iterator to
the range. This notion of ranges was standardized in C++20 with the Ranges
TS. This library provides barebone support of this functionality.

Range requirements
------------------

The range must be:
 * bidirectional
 * a view
 * reconstructible

Using C++20 concepts:

.. code-block:: cpp

    template <typename Range>
    concept scannable_range =
        std::ranges::bidirectional_range<Range> &&
        std::ranges::view<Range> &&
        std::ranges::pair-reconstructible-range<Range>;

Bidirectional?
**************

A bidirectional range is a range, the iterator type of which is
bidirectional: <a
href="http://eel.is/c++draft/iterator.concepts#iterator.concept.bidir">C++
standard</a>. Bidirectionality means, that the iterator can be moved both
forwards: ``++it`` and backwards ``--it``.

Note, that both random-access and contiguous ranges are refinements of
bidirectional ranges, and can be passed to the library. In fact, the library
implements various optimizations for contiguous ranges.

View?
*****

A view is a range that is cheap to copy and doesn't own its elements: <a
href="http://eel.is/c++draft/range.view">C++ standard</a>.

Basically no container within the standard library is a view. This means,
that for example a ``std::string`` can't be passed to scnlib. This can be
worked around with ``scn::make_view``, which returns a ``string_view`` for a
``std::string``, which is a view.

.. code-block:: cpp

    std::string str = ...;
    scn::scan(scn::make_view(str), ...);

Reconstructible?
****************

A reconstructible range is a range that can be constructed from a begin
iterator and an end iterator (sentinel): <a
href="http://wg21.link/p1664">P1664</a>.

Character type
--------------

The range has an associated character type.
This character type can be either ``char`` or ``wchar_t``.
The character type is determined by the result of ``operator*`` of the range
iterator. If dereferencing the iterator returns
 * ``char`` or ``wchar_t``: the character type is ``char`` or ``wchar_t``
 * ``expected<char>`` or ``expected<wchar_t>``: the character type is ``char`` or ``wchar_t``
