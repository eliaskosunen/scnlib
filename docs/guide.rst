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
To learn more about the requirements on these ranges, see the API documentation on input ranges.

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
    // Using a fixed size buffer
    char buf[6] = {0};
    std::sscanf("hello world", "%5s", buf);
    // buf == "hello"

Error handling and return values
--------------------------------

scnlib does not use exceptions.
The library compiles with ``-fno-exceptions -fno-rtti`` and is perfectly usable without them.

Instead, it uses return values to signal errors.
This return value is truthy if the operation succeeded.
Using the ``.error()`` member function more information about the error can be gathered.

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
 * ``.string()``, ``.string_view()``, and ``.span()``: like ``.reconstruct()``, except they work for every contiguous range, and return a value of the type specified in the function name

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

TODO: files are going to change before release

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

``scn::ignore``
***************

``scn::scan_list`` and temporaries
**********************************

User types
----------

Tuple-based scanning API
------------------------

Miscellaneous
-------------

``string`` vs ``string_view`` vs ``span``
*****************************************

Positional arguments
********************

Wide ranges
***********
