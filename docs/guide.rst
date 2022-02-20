=====
Guide
=====

Installation and building
-------------------------

NOTE: the following instructions assume a Unix-like system with a command line.
If your development environment is different (e.g. Visual Studio), these steps cannot be followed verbatim.
Some IDEs, like Visual Studio, have CMake integration built into them.
In some other environments, you may have to use the CMake GUI or your system's command line.
To build a CMake project without ``make``, use ``cmake --build .``.

``scnlib`` uses CMake for building.
If you're already using CMake for your project, integration is easy with ``find_package``.

.. code-block:: sh

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j
    $ make install

.. code-block:: cmake

    # Find scnlib package
    find_package(scn CONFIG REQUIRED)

    # Target which you'd like to use scnlib
    # scn::scn-header-only to use the header-only version
    add_executable(my_program ...)
    target_link_libraries(my_program scn::scn)

Tests and benchmarks
********************

To build and run the tests and benchmarks for scnlib, clone the repository, and build it with CMake.

Building and running tests:

.. code-block:: sh

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_BUILD_TYPE=Debug ..
    $ make -j
    $ ctest -j4

Building and running the runtime performance benchmarks:

.. code-block:: sh

    # create build directory like above
    $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DSCN_NATIVE_ARCH=ON ..
    $ make -j

    # disable CPU frequency scaling
    $ sudo cpupower frequency-set --governor performance

    # run the benchmark of your choosing
    $ ./benchmark/runtime/integer/bench-int

    # re-enable CPU frequency scaling
    $ sudo cpupower frequency-set --governor powersave

Without CMake
*************

Headers for the library can be found from the ``include/`` directory, and source files from the ``src/`` directory.

Building and linking the library:

.. code-block:: sh

    $ mkdir build
    $ cd build
    $ c++ -c -I../include/ ../src/*.cpp
    $ ar rcs libscn.a *.o

``libscn.a`` can then be linked, as usual.

.. code-block:: sh

    # in your project
    $ c++ ... -Lpath-to-scn/build -lscn

When building as header-only, ``src/`` has to be in the include path, and ``SCN_HEADER_ONLY`` must be defined to ``1``:
In this case, a separate build stage obviously isn't required

.. code-block:: sh

    # in your project
    $ c++ ... -Ipath-to-scn/include -Ipath-to-scn/src -DSCN_HEADER_ONLY=1

Basic usage
-----------

``scn::scan`` can be used to parse various values from a source range.

A range is an object that has a beginning and an end.
Examples of ranges are string literals, ``std::string`` and ``std::vector<char>``.
Objects of these types, and more, can be passed to ``scn::scan``.
To learn more about the requirements on these ranges, see the API documentation on source ranges.

After the source range, ``scn::scan`` is passed a format string.
This is similar in nature to ``scanf``, and has virtually the same syntax as ``std::format`` and {fmt}.
In the format string, arguments are marked with curly braces ``{}``.
Each ``{}`` means that a single value is to be scanned from the source range.
Because ``scnlib`` uses variadic templates, type information is not required in the format string,
like it is with ``scanf`` (like ``%d``).

After the format string, references to arguments to be parsed are given.

.. code-block:: cpp

    // Scanning an int
    int i;
    scn::scan("123", "{}", i):
    // i == 123

    // Scanning a double
    double d;
    scn::scan("3.14", "{}", d);
    // d == 3.14

    // Scanning multiple values
    int a, b;
    scn::scan("0 1 2", "{} {}", a, b);
    // a == 0
    // b == 1
    // Note, that " 2" was not scanned,
    // because only two integers were requested

    // Scanning a string means scanning a "word" --
    //   that is, until the next whitespace character
    // this is the same behavior as with iostreams
    std::string str;
    scn::scan("hello world", "{}", str);
    // str == "hello"

Compare the above example to the same implemented with ``std::istringstream``:

.. code-block:: cpp

    int i;
    std::istringstream{"123"} >> i;

    double d;
    std::istringstream{"3.14"} >> d;

    int a, b;
    std::istringstream{"0 1 2"} >> a >> b;

    std::string str;
    std::istringstream{"hello world"} >> str;

Or with ``sscanf``:

.. code-block:: cpp

    int i;
    std::sscanf("123", "%d", &i);

    double d;
    std::sscanf("3.14", "%lf", &d);

    int a, b;
    std::sscanf("0 1 2", "%d %d", &a, &b);

    // Not really possible with scanf!
    char buf[16] = {0};
    std::sscanf("hello world", "%15s", buf);
    // buf == "hello"

Error handling and return values
--------------------------------

scnlib does not use exceptions.
The library compiles with ``-fno-exceptions -fno-rtti`` and is perfectly usable without them.

Instead, it uses return values to signal errors.
This return value is truthy if the operation succeeded.
Using the ``.error()`` member function more information about the error can be gathered.

``scn::scan`` and others are marked with ``[[nodiscard]]``-attributes,
so not checking the return value will cause a compiler warning.

If an error occurs, the arguments that were not scanned are not written to.
Beware of using possibly uninitialized variables.

.. code-block:: cpp

    int i;
    // "foo" is not an integer
    auto result = scn::scan("foo", "{}", i);
    // fails, i still uninitialized
    if (!result) {
        std::cout << result.error().msg() << '\n';
    }

Unlike with ``scanf``, partial successes are not supported.
Either the entire scanning operation succeeds, or a failure is returned.

.. code-block:: cpp

    int a, b;
    // "foo" is still not an integer
    auto result = scn::scan("123 foo", "{} {}", a, b);
    // fails -- result == false
    // a is written to, a == 123
    // b is still uninitialized

Oftentimes, the entire source range is not scanned, and the remainder of the range may be useful later.
The leftover range can be accessed with the member function ``.range()``.

.. code-block:: cpp

    int i;
    auto result = scn::scan("123 456", "{}", i);
    // result == true
    // i == 123
    // result.range() == " 456"

    result = scn::scan(result.range(), "{}", i);
    // result == true
    // i == 456
    // result.range() == ""

The return type of ``.range()`` is a valid range, but it is of an library-internal, user-unnameable type.
Its type is not the same as the source range.
If possible for the given source range type, the ``.reconstruct()`` member function can be used to create a range of the original source range type.
Note, that ``.reconstruct()`` is not usable with string literals.

.. code-block:: cpp

    std::string source{"foo bar"};
    std::string str;
    auto result = scn::scan(source, "{}", str);
    // result == true
    // str == "foo"
    // result.reconstruct() == " bar"

The result type has some additional useful member functions.
These include:

 * ``.empty()``: returns ``true`` if the leftover range is empty, meaning that there are definitely no values to scan from the source range any more.
 * ``.range_as_string()``, ``.range_as_string_view()``, and ``.range_as_span()``: like ``.reconstruct()``, except they work for every contiguous range, and return a value of the type specified in the function name

See the API documentation for more details.

To enable multiple useful patterns, the library provides a function ``scn::make_result``.
This function will return the result object for a given source range, that can be later reassigned to.
For example:

.. code-block:: cpp

    auto result = scn::make_result("foo");
    int i;
    if (result = scn::scan(result.range(), "{}", i)) {
        // success
        // i is usable
    } else {
        // failure
        // result contains more info, i not usable
    }

Or:

.. code-block:: cpp

    auto result = scn::make_result("123 456");
    int i;
    while (result = scn::scan(result.range(), "{}", i)) {
        // success
        // i is usable:
        // iteration #1: i == 123
        // iteration #2: i == 456
    }
    // failure
    // can either be an invalid value or EOF
    // in this case, it's EOF
    // i not modified, still i == 456

Files and ``stdin``
-------------------

To easily read from ``stdin``, use ``scn::input`` or ``scn::prompt``.
They work similarly ``scn::scan``.

.. code-block:: cpp

    // Reads an integer from stdin
    int i;
    auto result = scn::input("{}", i);

    // Same, but with an accompanying message sent to stdout
    int i;
    auto result = scn::prompt("Write an integer: ", "{}", i);

To use ``scn::scan`` with ``stdin``, use ``scn::cstdin()``.
It returns a ``scn::file&``, which is a range mapping to a ``FILE*``.

.. code-block:: cpp

    int i;
    auto result = scn::scan(scn::cstdin(), "{}", i);

``scn::input`` and ``scn::prompt`` sync with ``<cstdio>`` automatically,
so if you wish to mix-and-match ``scn::input`` and ``scanf``, it's possible without any further action.
``scn::scan`` and ``scn::cstdin()`` don't do this, but you must explicitly call ``scn::cstdin().sync()`` when synchronization is needed.

.. code-block:: cpp

    int i, j;
    scn::input("{}", i);
    std::scanf("%d", &j);

    int i, j;
    scn::scan(scn::cstdin(), "{}", i);
    scn::cstdin().sync(); // needed here, because we wish to use <cstdio>
    std::scanf("%d", &j);

You can also scan from other file handles than ``stdin``.
You can either use ``scn::file`` or ``scn::owning_file``, depending on if you want to handle the lifetime of the ``FILE*`` yourself, or let the library handle it, respectively.

.. code-block:: cpp

    auto f = std::fopen("file.txt", "r");
    scn::file file{f};
    f.close();
    // file now unusable

    scn::owning_file file{"file.txt", "r"};

Both ``scn::file`` and ``scn::owning_file`` are valid source ranges, and can be passed to ``scn::scan`` and other scanning functions.
``scn::owning_file`` is a child class of ``scn::file``, so ``scn::owning_file& -> scn::file&`` is a valid conversion.

There's also ``scn::mapped_file`` for easier management of memory mapped files, see the API documentation for more.

Other scanning functions
------------------------

``scn::scan_default``
*********************

Oftentimes, specific parsing configuration through the format string is not required.
In this case, ``scn::scan_default`` can be used.
Using it has some performance benefits, as a format string doesn't need to be parsed.

Using ``scn::scan_default`` with N args has the same semantics as
using ``scn::scan`` with a format string with N space-separated ``"{}"`` s.

.. code-block:: cpp

    int a, b;
    auto result = scn::scan_default("123 456", a, b);
    // result == true
    // a == 123
    // b == 456

    // Equivalent to:
    int a, b;
    auto result = scn::scan("123 456", "{} {}", a, b);

``scn::scan_value``
*******************

If you only wish to scan a single value with default options, you can avoid using output arguments by using ``scn::scan_value``.
The return value of ``scn::scan_value<T>`` contains a ``.value()`` member function that returns a ``T`` if the operation succeeded.

.. code-block:: cpp

    auto result = scn::scan_value<int>("123");
    // result == true
    // result.value() == 123

As is evident by the presence of an extra member function, the return type of ``scan_value`` is not the same as the one of ``scan``.
The return type of ``scan`` inherits from ``scn::wrapped_error``, but the return type of ``scan_value`` inherits from ``scn::expected<T>``.
To use ``make_result`` with ``make_value``, this needs to be taken into account:

.. code-block:: cpp

    auto result = scn::make_result<scn::expected<int>>(...);
    result = scn::scan_value<int>(result.range());

The return types of ``scan`` and ``scan_value`` are not compatible, and cannot be assigned to each other.

Localization: ``scn::scan_localized``
*************************************

By default, ``scnlib`` isn't affected by changes to the global C or C++ locale.
All functions will behave as if the global locale was set to ``"C"``.

A ``std::locale`` can be passed to ``scn::scan_localized`` to scan with a locale.
This is mostly used with numbers, especially floats, giving locale-specific decimal separators.

Because of the way ``std::locale`` is, parsing with a locale is significantly slower than without one.
This is because the library effectively has to use iostreams for parsing.

.. code-block:: cpp

    // Reads a localized float
    double d;
    auto result = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "2,73", "{}", d);
    // result == true
    // d == 2.73

Because ``scan_localized`` uses iostreams under the hood, the results will not be identical to ``scn::scan``,
even if ``std::locale::classic()`` was passed.

``scn::getline``
****************

``scn::getline`` works similarly to ``std::getline``.
It takes a range to read from, a string to read into, and optionally a delimeter character defaulting to ``'\n'``.

.. code-block:: cpp

    std::string line;
    auto result = scn::getline("first\nsecond\nthird", line);
    // result == true
    // line == "first"
    // result.range() == "second\nthird" (note that the delim '\n' is skipped)

    // setting '\n' explicitly
    result = scn::getline(result.range(), line, '\n');
    // result == true
    // line == "second"
    // result.range() == "third"

    // delim doesn't have to be '\n' or even whitespace
    result = scn::getline(result.range(), line, 'r');
    // result == true
    // line == "thi"
    // result.range() == "d"

If the string to read into passed to ``scn::getline`` is a ``scn::string_view``, and the source range is contiguous,
the ``string_view`` is modified to point into the source range.
This increases performance (no copying or memory allocations) at the expense of lifetime safety.

.. code-block:: cpp

    std::string source = "foo\nbar";
    scn::string_view line;
    auto result = scn::getline(source, line);
    // result == true
    // line == "foo"
    // result.range() == "bar"
    // line.data() == source.data() (point to the same address -- `line` points to `source`)

``scn::ignore_until`` and ``scn::ignore_until_n``
*************************************************

``scn::ignore_until_n`` is like ``std::istream::ignore``.
It takes an integer N and a character C,
and reads the source range until either N characters were read or character C was found from the source range.

``scn::ignore_until`` works in the same way, except the only condition for stopping to read is finding the end character.
This is effectively equivalent to passing ``std::numeric_limits<std::ptrdiff_t>::max()`` as N to ``scn::ignore_until_n``.

``scn::scan_list`` and temporaries
**********************************

To easily scan multiple values of the same type, ``scn::scan_list`` can be used.
It takes a source range and a container to write the scanned values to.
Its return type is similar to that of ``scn::scan``.

.. code-block:: cpp

    std::vector<int> list;
    auto result = scn::scan_list("123 456 789", list);
    // result == true
    // list == [123, 456, 789]

For more customization, ``scn::scan_list_ex`` can be used.
It takes a third parameter, of type ``scn::scan_list_options``, which can set a delimeter and an until-character.
For easier use, factory functions ``scn::list_delimeter``, ``scn::list_until``, and ``scn::list_delimeter_and_until`` exist.

The delimeter character can be set to skip a character, like a comma, between list items.

.. code-block:: cpp

    std::vector<int> list;
    auto result = scn::scan_list_ex("123, 456, 789", list, scn::list_delimeter(','));
    // result == true
    // list == [123, 456, 789]

``scn::scan_list`` will read until an invalid value or delimeter is found, or the source range is exhausted.
Reading can also be stopped by setting an until-character.

.. code-block:: cpp

    std::vector<int> list;
    auto result = scn::scan_list_ex("123 456 789\n123", list, scn::list_until('\n'));
    // result == true
    // list == [123, 456, 789]
    // result.range() == "123"

If you've already allocated memory for the list, ``scan_list`` and ``scan_list_until`` can be passed a ``scn::span``.
Because the container must be passed to ``scan_list`` as an lvalue reference, the ``span`` must be constructed separately,
which can be tedious.
The library provides some helpers for this.

.. code-block:: cpp

    // Doing everything explicitly
    std::vector<int> list(64, 0);
    auto span = scn::make_span(list);
    auto result = scn::scan_list("123 456 789", span);
    // result == true
    // list == span == [123, 456, 789]

    // Using scn::temp
    // Takes an rvalue and makes it usable as an argument to scanning functions requiring an lvalue reference
    // Useful with spans and other views
    std::vector<int> list(64, 0);
    auto result = scn::scan_list("123 456 789", scn::temp(scn::make_span(list)));
    // result == true
    // list == span == [123, 456, 789]

    // Using scn::make_span_list_wrapper
    // Takes a container and returns a span into it, already wrapped with scn::temp
    // Effectively equivalent to the example above
    std::vector<int> list(64, 0);
    auto result = scn::scan_list("123 456 789", scn::make_span_list_wrapper(list));
    // result == true
    // list == span == [123, 456, 789]

``scn::temp`` can be also utilized elsewhere

.. code-block:: cpp

    std::vector<char> buffer(64, 0);
    // Reads up to 64 chars into the buffer
    auto result = scn::scan_default(source, scn::temp(scn::make_span(buffer)));

User types
----------

To scan a value of a program-defined type, specialize ``scn::scanner``.
There's a helper function for this purpose, ``scn::scan_usertype``.
In addition to other already familiar parameters, it takes a scanning context
that it uses to scan the value, instead of a source range.

.. code-block:: cpp

    struct int_and_double {
        int i;
        double d;
    };

    template <>
    struct scn::scanner<int_and_double> : scn::empty_parser {
        template <typename Context>
        error scan(int_and_double& val, Context& ctx)
        {
            return scn::scan_usertype(ctx, "[{}, {}]", val.i, val.d);
        }
    };

    // ...

    int_and_double val;
    auto result = scn::scan("[123, 3.14]", "{}", val);
    // result == true
    // val.i == 123
    // val.d == 3.14

The above example inherits from ``scn::empty_parser``.
This implements the format string functionality for this type.
``scn::empty_parser`` is a good default choice, as it only accepts empty format strings.
You could also inherit from other scanner types (like ``scn::scanner<int>``),
or implement ``parse()`` by hand (see ``reader.h`` in the library source code).
There's also ``scn::common_parser`` and ``scn::common_parser_default``, see the API documentation for more information on those.

Should you need more direct control, you could use the scanning functions, like ``scn::scan`` directly.
In this case, make sure to assign the returned range into the scanning context.
This is because these scanning functions create their own internal context that does not sync with the caller.

.. code-block:: cpp

    // This is equivalent to the scanner::scan implementation above
    auto r = scn::scan(ctx.range(), "[{}, {}]", val.i, val.d);
    ctx.range() = std::move(r.range());
    return r.error();

Alternatively, you could also include the header ``<scn/istream.h>``.
This enables scanning of types with a ``std::istream`` compatible ``operator>>``.
Using this functionality is discouraged, as using iostreams to scan these values presents some difficulties with error recovery,
and will lead to worse performance.

Specializing ``scn::scanner`` should be preferred over relying on ``<scn/istream.h>``.

Tuple-based scanning API
------------------------

By including ``<scn/tuple_return.h>``, you'll get access to an alternative API,
which returns the scanned values in a tuple instead of output parameters.
See Rationale for why this is not the default API.

These functions are slightly slower compared to their output-parameter equivalents, both in runtime and compile time.

.. code-block:: cpp

    #include <scn/tuple_return.h>

    // Way more usable with C++17 structured bindings
    // Can also be used without them
    auto [result, i] = scn::scan_tuple<int>("123", "{}");
    // result == true
    // i == 123

Miscellaneous
-------------

``string`` vs ``string_view`` vs ``span<char>``
***********************************************

Three types that at first glance might appear quite similar,
have significant differences what comes to how they're scanned by the library.

``std::string`` works very similarly to how it works with ``<iostream>``.
It scans a "word": a sequence of letters separated by spaces.
More precisely, it reads the source range into the string, until a whitespace character is found or the range reaches its end.

``span<char>`` works like ``istream.read``: it copies bytes from the range into the buffer it's pointing to.

``string_view`` works like ``std::string``, except it doesn't copy, but changes its data pointer to point into the source stream.
Scanning a ``string_view`` works only with contiguous ranges, and may lead to lifetime issues, but it will give you better performace (avoids copying and allocation).

.. code-block:: cpp

    scn::string_view source{"hello world"};

    std::string str;
    scn::scan(source, "{}", str);
    // str == "hello"

    scn::string_view sv;
    scn::scan(source, "{}", sv);
    // sv == "hello"
    // sv.data() == source.data() -- sv points to source
    // Make sure that `source` outlives `sv`

    std::vector<char> buffer(5, '\0'); // 5 bytes, all zero
    scn::span<char> s = scn::make_span(buffer);
    scn::scan(source, "{}", s);
    // s == buffer == "hello"
    // Reads 5 bytes, doesn't care about whitespace
    // No lifetime problems, the data is copied into the span/the buffer it points to

Wide ranges
***********

Source ranges have an associated character type, either ``char`` or ``wchar_t``.
This character type is determined by the type of deferencing an iterator into the range, which is either ``CharT`` or ``scn::expected<CharT>``.
For most use cases, this type is ``char``. In this case, the range is said to be narrow.
If the character type is ``wchar_t``, the range is said to be wide.

The return types of scanning narrow and wide ranges are incompatible and cannot be mixed.

``char``, ``std::string``, ``scn::string_view``, and ``scn::span<char>`` cannot be scanned from a wide range.
``wchar_t``, ``std::wstring``, ``scn::wstring_view``, and ``scn::span<wchar_t>`` cannot be scanned from a narrow range.

Wide ranges are useful if your source data is wide (often the case on Windows).
Narrow ranges should be preferred if possible, however.

Character encoding
******************

The library assumes, that all narrow ranges are UTF-8, and all wide ranges are UTF-16 or UTF-32, depending on ``sizeof(wchar_t)``.
This holds, whether or not a ``std::locale`` is used.

To scan a Unicode code point, use ``scn::code_point``:

.. code-block::cpp

    // Assumed to be UTF-8 (because it's narrow)
    auto source = "ä";
    // source == [0xc3, 0xa4, 0x0]
    scn::code_point cp{};
    auto result = scn::scan("ä", "{}", cp);
    // result.empty() == true
    // cp == 0xe4 (ä is U+00E4)

The encoding of wide ranges is assumed to be whatever is set in the global C locale.
The encoding must be ASCII-compatible.
