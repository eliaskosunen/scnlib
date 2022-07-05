============================
Migration Guide v1.0 -> v2.0
============================

For v2.0, the library was rewritten and redesigned in its entirety.
The new design is more focused and powerful, and closer to ``std::format`` than previously.

This guide isn't exhaustive, because the changes are very extensive, but should be enough to get you started.

C++17 required
--------------

v1 required C++11 in order to compile. v2, at least at this point, requires C++17.

Header files changed
--------------------

``<scn/tuple_return.h>`` is removed in v2: no longer necessary, "tuple return" is now the only available API.

``<scn/all.h>`` is removed in v2. Include the required headers explicitly to get the same behavior.

``scan_`` prefix added to many names inside the ``scn`` namespace
-----------------------------------------------------------------

To prepare for standardization, in v2, many names have the prefix ``scan_``,
or otherwise indicate being related to scanning.

Changes include:

.. list-table:: Changes in names
    :widths: 50 50
    :header-rows: 1

    * - v1
      - v2
    * - ``scn::error``
      - ``scn::scan_error``
    * - ``scn::basic_arg``, ``scn::basic_args``, ``scn::arg_store``
      - ``scn::basic_scan_arg``, ``scn::basic_scan_args``, ``scn::scan_arg_store``
    * - ``scn::basic_context``
      - ``scn::basic_scan_context``
    * - ``scn::basic_parse_context``, ``scn::parse_context``
      - ``scn::basic_scan_parse_context``, ``scn::scan_parse_context``

``scn::scan`` argument passing and return value
-----------------------------------------------

The largest change is in how values are returned from ``scn::scan`` and other scanning functions.

In v1, values were passed to ``scn::scan`` by lvalue reference as out parameters.
The return value was used to get information about the leftover input data, and about possible errors.

.. code-block:: cpp

    int i;
    std::string str;
    auto result = scn::scan("123 input", "{} {}", i, str);

In v2, the values are returned from ``scn::scan``, using a tuple-like type.
The types of the arguments are given in an explicit template parameter list,
instead of being deduced from the given arguments.

.. code-block:: cpp

    auto result_tuple = scn::scan<int, std::string>("123 input", "{} {}");
    auto& result = std::get<0>(result_tuple);
    auto& i = std::get<1>(result_tuple);

    // Way better with C++17 structured bindings
    auto [result, i, str] = scn::scan<int, std::string>("123 input", "{} {}");

The ``result`` value above is truthy when the operation was successful (also observable with ``result.good()``).
Use ``result.range()`` to get what's left of the input range, and ``result.error()`` to see any possible errors.

No more "indirect" ranges: revamped source range error handling
---------------------------------------------------------------

The notion of "indirect" ranges from v1 is removed in v2.
Indirect ranges were source ranges, the value type of which was ``expected<CharT>``, instead of ``CharT``.
This was to enable source ranges to report their own errors to the library,
and for it to pass them forward to the user.
In v2, the value type of the source range must either be ``char`` or ``wchar_t``.

This approach was arguably against the principles of Ranges,
and made a lot of things more complicated than they needed to be.

In v2, the separation of I/O and input parsing is more clearly separated.
scnlib is not intended to be an I/O library, and that it shan't try to be.
In the optimal case, if I/O needs to be performed to fetch the data to be passed to scnlib,
that is done by the user, to ensure proper behavior and error recovery.
Also, when scnlib is given plain contiguous strings as input, instead of more complicated ranges,
a number of optimizations are enabled.

.. code-block:: cpp

    std::string input;
    std::getline(file, input);

    auto [result, i] = scn::scan<int>(input, "{}");

If doing your own I/O isn't possible, or is for some reason unfeasible, a number of other options are available:

1) An ``scn::(w)istreambuf_view``/``scn::(w)istreambuf_subrange`` can be given as a source range to ``scn::scan``.
   These types wrap an arbitrary ``std::(w)istream``/``std::streambuf``.
   This can be useful, if you already have a ``std::(w)istream``,
   and don't want to accidentally read anything extra from the stream, like with ``std::cin``.

.. code-block:: cpp

    auto range = scn::istreambuf_view{std::cin};
    auto [result, i] = scn::scan<int>(range, "{}");

2) Signal errors like any other range signals them: by reaching end prematurely, or with exceptions (discouraged).
   If using a custom user-provided range, this is likely the only option.

.. code-block:: cpp

    auto [result, i, d] = scn::scan<int, double>(custom_source_range, "{} {}");
    // result can be true, if both i and d could be scanned, even if the given range reached an error condition
    // We need to do the checking ourselves through custom_source_range, through whatever mechanism it provides
    if (result && custom_source_range.good()) {
        // Use i and d
    }

    // Alternatively, if custom_source_range throws on error
    try {
        auto [result, i, d] = scn::scan<int, double>(custom_source_range, "{} {}");
        if (result) {
            // Use i and d
        }
    } catch (const custom_source_range_error& e) {
        // ...
    }


Relaxed source range requirements
---------------------------------

The set of allowed source ranges to be given to ``scn::scan`` is increased in v2, compared to v1.

In v1, a range was scannable, if it was bidirectional, and default and move constructible.

In v2, the range must be a forward range, and movable.

More narrow set of ranges accepted by ``vscan``: erased ranges
--------------------------------------------------------------

In contrast, in v1, ``scn::vscan`` could take any range that ``scn::scan`` could.
``scn::vscan`` was a template, that would instantiate the library internals for all different source range types.

In v2, ``scn::vscan`` can only take a limited set of ranges.

1) ``std::(w)string_view``

All contiguous+sized ranges passed to ``scn::scan`` are mapped to
``std::(w)string_view`` depending on character type, e.g.
``std::(w)string``, ``std::vector<char/wchar_t>``, ``char/wchar_t[]`` etc.

2) ``scn::(w)istreambuf_subrange``

Constructible from ``scn::(w)istreambuf_view``, which the user can pass to ``scn::scan``.

3) ``scn::(w)erased_range``

All other forward ranges are type-erased, and passed to ``scn::vscan`` as ``scn::(w)erased_range``s.

If possible, option 1) should be preferred, followed by option 2) and 3).
Each of these option is less performant than the option before it.

Returned ranges do not take ownership (may return ``dangling``)
---------------------------------------------------------------

In v1, the lifetime semantics of the range returned from ``scn::scan`` were complicated.
Usually, the returned range was a view over the given range, i.e. reference semantics were used.
But, sometimes, if the range was an rvalue container (or anything else that didn't model ``borrowed_range``),
the return value contained that range, i.e. ownership was taken.

.. code-block:: cpp

    // v1: reference semantics
    int i{};
    auto result = scn::scan("123 456", "{}", i);
    // result contains a string_view over the given string literal

    // v1: reference semantics
    std::string source{"123 456"};
    int i{};
    auto result = scn::scan(source, "{}", i);
    // result contains a string_view over source

    // v1: ownership semantics
    int i{};
    auto result = scn::scan(std::string{"123 456"}, "{}", i);
    // result contains a std::string

In v2, the semantics are clearer: a view (``string_view`` or ``subrange``) over the given range is always returned.
If that view would dangle, ``ranges::dangling`` is returned instead.

.. code-block:: cpp

    // v2: reference semantics (no change)
    auto [result, i] = scn::scan<int>("123 456", "{}");
    // result contains a string_view over the given string literal

    // v2: reference semantics (no change)
    std::string source{};
    auto [result, i] = scn::scan<int>(source, "{}");
    // result contains a string_view over source

    // v2: dangling
    auto [result, i] = scn::scan<int>(std::string{"123 456"}, "{}");
    // result contains a ranges::dangling, the given std::string has gone out of scope and been destroyed

In other words, in v2, ``scn::scan`` always returns a view to the given range.
If that's not possible, it returns ``ranges::dangling`` instead.

Files removed
-------------

In v1, scnlib provided support for reading files with ``scn::file``, ``scn::owning_file``,
and ``scn::mapped_file``. These caused the library to grow in size, blurred its focus, and were the source of many bugs.

In v2, these have been removed.
If you need to read from a file, either do your own I/O and give ``scn::scan`` a string,
or use ``scn::(w)istreambuf_view``.
If you need to use memory mapped files, do the mapping yourself, and give ``scn::scan`` a view into the mapped memory.

In v2, ``scn::cstdin()`` and ``scn::wcstdin()`` have been removed.
For reading from stdin, use ``scn::input`` and ``scn::prompt``,
or create your own ``scn::(w)istreambuf`` from ``std::(w)cin``,
remembering to sync the range afterwards with ``std::(w)cin``.

.. code-block:: cpp

    // v1:
    int i;
    auto result = scn::input("{}", i);
    // or
    auto result = scn::scan(scn::cstdin(), "{}", i);

    // v2:
    auto [result, i] = scn::input<int>("{}");
    // or
    auto in = scn::istreambuf_view{std::cin};
    auto [result, i] = scn::scan<int>(in, "{}");
    in.sync(result.range().begin());

Specializing ``scn::scanner`` changed
-------------------------------------

In v1, ``scn::scanner`` took the type it was used for as a template parameter.
Inside it, ``parse()`` and ``scan()`` returned a ``scn::error``.

.. code-block:: cpp

    struct int_and_double {
        int i;
        double d;
    };

    template <>
    struct scn::scanner<int_and_double> {
        template <typename ParseCtx>
        error parse(ParseCtx& pctx);

        template <typename Context>
        error scan(int_and_double& val, Context& ctx) const;
    };

In v2, ``scn::scanner`` also takes in the character type of the source range.
This is consistent with ``std::formatter``.

``parse()`` and ``scan()`` return a ``scn::expected<iterator>``.

``parse()`` should be ``constexpr``, to support compile-time format string checking.

.. code-block:: cpp

    struct int_and_double {
        int i;
        double d;
    };

    template <typename CharT>
    struct scn::scanner<int_and_double, CharT> {
        template <typename ParseCtx>
        constexpr auto parse(ParseCtx& pctx) -> expected<typename ParseCtx::iterator>;

        template <typename Context>
        auto scan(int_and_double& val, Context& ctx) const -> expected<typename Context::iterator>;
    };

``scn::scan_usertype`` removed
------------------------------

In v1, ``scn::scan_usertype`` could be used to make scanning values of custom types easier.
This helper function was necessary, because the scanning context had complex logic concerning the source range.
In v2, this has been removed, because of the new tuple-return API,
and because the context no longer deals with complicated ranges.

.. code-block:: cpp

    // v1
    template <typename Context>
    error scan(int_and_double& val, Context& ctx) const {
        return scn::scan_usertype(ctx.range(), "[{}, {}]", val.i, val.d);
    }

    // v2
    template <typename Context>
    auto scan(int_and_double& val, Context& ctx) const -> expected<typename Context::iterator> {
        auto [result, i, d] = scn::scan<int, double>(ctx.range(), "[{}, {}]);
        if (result) {
            val = int_and_double{i, d};
            return result.range().begin();
        }
        return unexpected(result.error());
    }

``scn::*_parser`` removed
-------------------------

In v1, there were helper base classes for creating ``scanner::parse``,
including ``scn::empty_parser`` and ``scn::common_parser``.

In v2, these are removed. Create your own ``parse`` member functions, or reuse already existing ``scanner``s.

Including ``<scn/istream.h>`` no longer enables custom scanning for types with ``operator>>`` by default
--------------------------------------------------------------------------------------------------------

In v1, just by including ``<scn/istream.h>``, any type with an ``operator>>`` would be automatically ``scn::scan``able.

In v2, you'll need to explicitly opt-in to this behavior for your own types, by creating a ``scn::scanner``,
and inheriting from the ``scn::basic_istream_scanner<CharT>`` class template.

This is done to avoid potentially surprising behavior.

.. code-block:: cpp

    #include <scn/istream.h>

    struct mytype {
        int i, j;

        friend std::istream& operator>>(std::istream& is, const mytype& val) {
            return is >> val.i >> val.j;
        }
    };

    // v1 would work out of the box:
    mytype val{};
    auto result = scn::scan("123 456", "{}", val);

    // v2 requires a scanner definition
    template <typename CharT>
    struct scn::scanner<mytype, CharT> : public scn::basic_istream_scanner<CharT> {};

    auto [result, val] = scn::scan<mytype>("123 456", "{}");

``scn::scan_localized`` renamed to ``scn::scan``
------------------------------------------------

In v1, to use a ``std::locale`` in scanning, the function ``scn::scan_localized`` had to be used.

In v2, this function is part of the ``scn::scan`` overload set.

.. code-block:: cpp

    // v1
    int i;
    auto ret = scn::scan_localized(locale, "42", "{}", i);

    // v2;
    auto [result, i] = scn::scan<int>(locale, "42", "{}");

List operations removed
-----------------------

In v1, there were ``scn::scan_list`` and ``scn::scan_list_ex``,
that could be used to scan multiple values of the same type into a container.

In v2, these have been removed.
Either scan each value manually, or use the new (experimental) range scanning functionality, in ``<scn/ranges.h>``.

.. code-block:: cpp

    // v1
    std::vector<int> vec{};
    auto result = scn::scan_list("123 456 abc", vec);
    // vec == [123, 456]
    // result.range() == " abc"
    // NOTE: result.error() == invalid_scanner_value (because of "abc")

    // v2
    std::vector<int> vec{};
    auto [result] = scn::scan("123 456 abc", "");
    while (!result.range().empty()) {
        int i{};
        std::tie(result, i) = scn::scan<int>(result.range(), "{}");
        if (!result) {
            break;
        }
        vec.push_back(i);
    }
    // vec == [123, 456]
    // result.range() == " abc"

    // or, if the source range is in the correct format
    // (how std::format would output it)
    auto [result, vec] = scn::scan<std::vector<int>>("[123, 456]", "{}");
    // vec == [123, 456]


``scn::ignore`` and ``scn::getline`` removed
--------------------------------------------

In v2, ``scn::ignore`` can be replaced with simple range operations, like ``std::ranges::views::drop_while``.

``scn::getline`` can be replaced with ``scn::scan<std::string>(..., "{:[^\n]}")``.

Encoding is always Unicode
--------------------------

In v1, when scanning in non-localized mode, the input was assumed to be Unicode
(UTF-8, UTF-16, or UTF-32, based on the character type),
and whatever the locale specified in localized mode.
Because of the limited character encoding handling support provided by the standard library, this was buggy.

In v2, all input is assumed to be Unicode, despite what has been set in a possibly supplied locale.
