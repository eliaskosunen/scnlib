=================
API Documentation
=================

The public API of scnlib consists of these headers:

 * ``scn/fwd.h``: Forward declarations
 * ``scn/scan.h``: Scanning API
 * ``scn/istream.h``: Support for scanning user-defined types with an ``operator>>``
 * ``scn/ranges.h``: (Experimental) Scanning of range types
 * ``scn/xchar.h``: Support for ``wchar_t``
 * ``scn/all.h``: All of the above

Basic Scanning API
------------------

The following functions use a format string syntax similar to that of ``std::format``:
:ref:`Format Strings`.

When taking a range ``source`` as input, the range must model the ``scannable_range`` concept:
:ref:`Scannable Ranges`.

.. doxygenfunction:: scan(Source&& source, format_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: scan_value(Source&& source) -> scan_result_type<Source, T>

.. doxygenfunction:: input(format_string<Args...> format) -> scan_result_type<scn::istreambuf_view&, Args...>

.. doxygenfunction:: prompt(const char* msg, format_string<Args...> format) -> scan_result_type<scn::istreambuf_view&, Args...>

Scannable Ranges
----------------

A range is considered scannable, if it models at least ``forward_range``, and its character type is correct
(its value type is the same with the format string).
If the range additionally models ``contiguous_range`` and ``sized_range``, additional optimizations are enabled.

.. code-block:: cpp

    template <typename Range, typename CharT>
    concept scannable_range =
        ranges::forward_range<Range> &&
        std::same_as<ranges::range_value_t<Range>, CharT>;

To turn an ``input_range`` into a ``scannable_range``, ``scn::caching_view`` can be used:

.. doxygenclass:: scn::basic_caching_view
    :members:

Result Types
------------

.. doxygentypedef:: scan_result_type

``scan`` and others return an object of type ``scan_result``, wrapped in a ``scan_expected``.
`std::expected on cppreference <https://en.cppreference.com/w/cpp/utility/expected>`_.

.. doxygentypedef:: scan_expected

.. doxygenfunction:: make_scan_result(scan_expected<ResultRange>&& result, scan_arg_store<Context, Args...>&& args)

.. doxygenclass:: scn::scan_result
    :members:
    :undoc-members:

.. doxygenclass:: scn::scan_error
    :members:
    :undoc-members:

.. doxygenclass:: scn::expected
    :members:

Format Strings
--------------

TODO

Type-Erased Scanning API
------------------------

.. doxygentypedef:: vscan_result

.. doxygentypedef:: scan_args_for

.. doxygentypedef:: scan_arg_for

.. doxygenfunction:: vscan(Range&& range, std::string_view format, scan_args_for<Range, char> args) -> vscan_result<Range>

.. doxygenfunction:: vscan_value(Range&& range, scan_arg_for<Range, char> arg) -> vscan_result<Range>

Contexts and Scanners
---------------------

.. doxygenclass:: scn::basic_scan_context
    :members:
    :undoc-members:

.. doxygenclass:: scn::basic_scan_parse_context
    :members:
    :undoc-members:

.. doxygenclass:: scn::scanner

Localization
------------

.. doxygenfunction:: scn::scan(const Locale &loc, Source &&source, format_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: vscan(const Locale& loc, Range&& range, std::string_view format, scan_args_for<Range, char> args) -> vscan_result<Range>

Wide Character APIs
-------------------

.. doxygenfunction:: scn::scan(Source&& source, wformat_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: scn::input(wformat_string<Args...> format) -> scan_result_type<wistreambuf_view&, Args...>

.. doxygenfunction:: scn::prompt(const wchar_t* msg, wformat_string<Args...> format) -> scan_result_type<wistreambuf_view&, Args...>

.. doxygenfunction:: scn::scan(const Locale &loc, Source&& source, wformat_string<Args...> format) -> scan_result_type<Source, Args...>

.. doxygenfunction:: vscan(Range&& range, std::wstring_view format, scan_args_for<Range, wchar_t> args) -> vscan_result<Range>

.. doxygenfunction:: vscan_value(Range&& range, scan_arg_for<Range, wchar_t> arg) -> vscan_result<Range>

.. doxygenfunction:: vscan(const Locale& loc, Range&& range, std::wstring_view format, scan_args_for<Range, wchar_t> args) -> vscan_result<Range>
