=====
Guide
=====

Installation and building
-------------------------

NOTE: the following instructions assume a Unix-like system with a command line.
If your development environment is different (e.g. Visual Studio), these steps cannot be followed verbatim.
Some IDEs, like Visual Studio and CLion, have CMake integration built into them.
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
    $ ctest -j

Building and running the runtime performance benchmarks:

.. code-block:: sh

    # create build directory like above
    $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DSCN_NATIVE_ARCH=ON ..
    $ make -j

    # disable CPU frequency scaling
    $ sudo cpupower frequency-set --governor performance

    # run the benchmark of your choosing
    $ ./benchmark/runtime/integer/scn_int_bench

    # re-enable CPU frequency scaling
    $ sudo cpupower frequency-set --governor powersave

Without CMake
*************

``scnlib`` internally depends on `fast_float`_ and `simdutf`_.
If not using CMake, you'll need to download and build these libraries yourself.

Headers for the library can be found from the ``include/`` directory, and source files from the ``src/`` directory.

Building and linking the library:

.. code-block:: sh

    $ mkdir build
    $ cd build
    $ c++ -c -I../include/ ../src/*.cpp -Ipath-to-fast-float -Ipath-to-simdutf
    $ ar rcs libscn.a *.o

``libscn.a`` can then be linked, as usual, with your project.
Don't forget to also include ``fast_float`` and ``simdutf``.

.. code-block:: sh

    # in your project
    $ c++ ... -Lpath-to-scn/build -lscn -Lpath-to-fast-float -lfast_float -Lpath-to-simdutf -lsimdutf

.. _fast_float: https://github.com/fastfloat/fast_float
.. _simdutf: https://github.com/simdutf/simdutf

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
Because ``scnlib`` uses templates, type information is not required in the format string,
like it is with ``scanf`` (e.g. ``%d``).

The list of the types of the values of the scan are given as template parameters to ``scn::scan``.
``scn::scan`` then returns a tuple-like object, with the first element being a result object,
and the others being the scanned values.

.. code-block:: cpp

    // Scanning an int
    auto [result, i] = scn::scan<int>("123", "{}"):
    // i == 123

    // Scanning a double
    auto [result, d] = scn::scan<double>("3.14", "{}");
    // d == 3.14

    // Scanning multiple values
    auto [result, a, b] = scn::scan<int, int>("0 1 2", "{} {}");
    // a == 0
    // b == 1
    // Note, that " 2" was not scanned,
    // because only two integers were requested

    // Scanning a string means scanning a "word" --
    //   that is, until the next whitespace character
    // this is the same behavior as with iostreams
    auto [result, str] = scn::scan<std::string>("hello world", "{}");
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

If an error occurs, the returned values are value-initialized.

.. code-block:: cpp

    // "foo" is not an integer
    auto [result, i] = scn::scan<int>("foo", "{}");
    // fails, i still uninitialized
    if (!result) {
        std::cout << result.error().msg() << '\n';
    }

Unlike with ``scanf``, partial successes are not supported.
Either the entire scanning operation succeeds, or a failure is returned.

.. code-block:: cpp

    // "foo" is still not an integer
    auto [result, a, b] = scn::scan<int, int>("123 foo", "{} {}");
    // fails -- result == false
    // a scan succeeded, a == 123
    // b is value-initialized, b == 0

Oftentimes, the entire source range is not scanned, and the remainder of the range may be useful later.
The leftover range can be accessed with the member function ``.range()``.

.. code-block:: cpp

    auto [result, i] = scn::scan<int>("123 456", "{}");
    // result == true
    // i == 123
    // result.range() == " 456"

    auto [other_result, i] = scn::scan<int>(result.range(), "{}");
    // other_result == true
    // i == 456
    // other_result.range() == ""

The return type of ``.range()`` is a view into the range ``scn::scan`` is given (more precisely, a ``ranges::subrange``).
Its type is not the same as the source range.

To enable multiple useful patterns, the library provides a function ``scn::scan_map_input_range``.
This function will return a view into the range that it's given,
the type of which is the same as if it had been passed to ``scn::scan``.
This view can then be given to ``scn::scan``, and be assigned to again.
For example:

.. code-block:: cpp

    auto range = scn::make_scan_result_range("foo");
    if (auto [result, i] = scn::scan<int>(range, "{}")) {
        // success
    } else {
        // failure
        // result contains more info
    }

Or:

.. code-block:: cpp

    auto range = scn::make_scan_result_range("123 456");
    while (auto [result, i] = scn::scan<int>(range, "{}")) {
        range = result.range();

        // success
        // iteration #1: i == 123
        // iteration #2: i == 456
    }
    // failure (scn::scan returned a result that's falsy)
    // can either be an invalid value or EOF
    // in this case, it's EOF
