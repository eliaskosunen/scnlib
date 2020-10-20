===========
Input range
===========

This page is about the types of ranges that can be passed as inputs to
various functions within scnlib.

Fundamentally, a range is something that has a beginning and an end. Examples
of ranges are string literals, arrays, and ``std::vector`` s. All of these can
be passed to ``std::begin`` and ``std::end``, which then return an iterator to
the range. This notion of ranges was standardized in C++20 with the Ranges
TS. This library provides barebone support of this functionality.

Range requirements
------------------

The range must be:
 * bidirectional
 * default and move constructible

Using C++20 concepts:

.. code-block:: cpp

    template <typename Range>
    concept scannable_range =
        std::ranges::bidirectional_range<Range> &&
        std::default_constructible<Range> &&
        std::move_constructible<Range>;

Bidirectional?
**************

A bidirectional range is a range, the iterator type of which is
bidirectional: http://eel.is/c++draft/iterator.concepts#iterator.concept.bidir.
Bidirectionality means, that the iterator can be moved both
forwards: ``++it`` and backwards ``--it``.

Note, that both random-access and contiguous ranges are refinements of
bidirectional ranges, and can be passed to the library. In fact, the library
implements various optimizations for contiguous ranges.

Recommended range requirements
------------------------------

In addition, to limit unnecessary copies and possible dynamic memory allocations,
the ranges should be passed as an lvalue, and/or be a `view`: http://eel.is/c++draft/range.view.
Passing a non-view as an rvalue will work, but it may cause worse performance, especially with larger source ranges.

.. code-block:: cpp

    // okay: view
    scn::scan(std::string_view{...}, ...);

    // okay: lvalue
    std::string source = ...
    scn::scan(source, ...);

    // worse performance: non-view + rvalue
    scn::scan(std::string{...}, ...);

In order for the `.reconstruct()` member function to compile in the result object,
the range must be a `pair-reconstructible-range` as defined by https://wg21.link/p1664r1,
i.e. be constructible from an iterator and a sentinel.

Character type
--------------

The range has an associated character type.
This character type can be either ``char`` or ``wchar_t``.
The character type is determined by the result of ``operator*`` of the range
iterator. If dereferencing the iterator returns

 * ``char`` or ``wchar_t``: the character type is ``char`` or ``wchar_t``, respectively
 * ``expected<char>`` or ``expected<wchar_t>``: the character type is ``char`` or ``wchar_t``, respectively
