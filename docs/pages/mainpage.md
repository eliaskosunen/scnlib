\mainpage scnlib
\tableofcontents

`scnlib` is a modern C++ library for scanning values.
Think of it as a more C++-y `scanf`, or the inverse of
<a href="https://fmt.dev">{fmt}</a> / `std::format`.

The code lives over at GitHub, at https://github.com/eliaskosunen/scnlib.

\code{.cpp}
#include <scn/scan.h>
#include <print> // for std::print, C++23

int main() {
    auto result = scn::scan<int, double>("[42, 3.14]", "[{}, {}]");
    if (result) {
        auto [first, second] = result->values();
        std::println("first: {}, second: {}", first, second);
        // Prints "first: 42, second: 3.14\n"
    }
}
\endcode

\section about About this documentation

This documentation is for the version 2.0 of the library.
This version is currently experimental, and under development, with no stability guarantees.
For a stable release, see version 1.1.
Its documentation is hosted over at Read the Docs:
https://v1.scnlib.dev/.

An introductory guide to the library can be found at \ref guide "Guide".
Instructions for migrating to v2.0 from v1.1 can be found at \ref migration-2-0 "Migration Guide v1.1 -> v2.0".

The API documentation is organized into modules, that can be found under Modules, behind the link at the top of the page.
It can be searched directly using the search function in the navbar, or by pressing the TAB key.
The most important modules are probably:

 - \ref scan 
 - \ref scannable
 - \ref format-string
 - \ref result

\section install Installation

\note The following instructions assume a Unix-like system with a command line.
If your development environment is different (e.g. Visual Studio), these steps cannot be followed verbatim.
Some IDEs, like Visual Studio and CLion, have CMake integration built into them.
In some other environments, you may have to use the CMake GUI or your system's command line.
To build a CMake project without `make`, use `cmake --build .`.

scnlib uses CMake for building.
If you're already using CMake for your project, integration is easy with `find_package`.

\code{.sh}
$ mkdir build
$ cd build
$ cmake ..
$ make -j
$ make install
\endcode

\code{.cmake}
# Find scnlib package
find_package(scn CONFIG REQUIRED)

# Target which you'd like to use scnlib
add_executable(my_program ...)
target_link_libraries(my_program scn::scn)
\endcode

Another option would be usage through CMake's `FetchContent` module.

\code{.cmake}
FetchContent_Declare(
        scn
        GIT_REPOSITORY  https://github.com/eliaskosunen/scnlib
        GIT_TAG         v2.0.0-beta
        GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(scn)

# ...

target_link_libraries(my_program scn::scn)
\endcode

\subsection deps Dependencies

scnlib internally depends on
<a href="https://github.com/fastfloat/fast_float">fast_float</a> and
<a href="https://github.com/simdutf/simdutf">simdutf</a>.

By default, the CMake machinery automatically fetches, builds, and links these libraries through `FetchContent`.
These libraries are only used in the implementation, and they are not visible to the users of the library.

Alternatively, by setting the CMake options `SCN_USE_EXTERNAL_FAST_FLOAT` or `SCN_USE_EXTERNAL_SIMDUTF` to `ON`,
these libraries are searched for using `find_package`. Use these options, if you already have these libraries
installed.

If your standard library doesn't have an available C++20 `<ranges>` implementation,
a single-header version of <a href="https://github.com/tcbrindle/nanorange">NanoRange</a>
is also bundled with the library, inside the directory `include/scn/external`.

The tests and benchmarks described below depend on GTest and Google Benchmark, respectively.
These libraries are also fetched with `FetchContent`, if necessary.

\subsection tests Tests and benchmarks

To build and run the tests and benchmarks for scnlib, clone the repository, and build it with CMake.

Building and running tests:

\code{.sh}
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make -j
$ ctest -j
\endcode

Building and running the runtime performance benchmarks:

\code{.sh}
# create build directory like above
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DSCN_NATIVE_ARCH=ON ..
$ make -j

# disable CPU frequency scaling
$ sudo cpupower frequency-set --governor performance

# run the benchmark of your choosing
$ ./benchmark/runtime/integer/scn_int_bench

# re-enable CPU frequency scaling
$ sudo cpupower frequency-set --governor powersave
\endcode

\subsection without-cmake Without CMake

As mentioned above, the implementation of scnlib depends on fast_float and simdutf.
If you're not using CMake, you'll need to download and build these libraries yourself.

Headers for scnlib can be found from the `include/` directory, and source files from the `src/` directory.

Building and linking the library:

\code{.sh}
$ mkdir build
$ cd build
$ c++ -c -I../include/ ../src/*.cpp -Ipath-to-fast-float -Ipath-to-simdutf
$ ar rcs libscn.a *.o
\endcode

`libscn.a` can then be linked, as usual, with your project.
Don't forget to also include `fast_float` and `simdutf`.

\code{.sh}
# in your project
$ c++ ... -Lpath-to-scn/build -lscn -Lpath-to-fast-float -lfast_float -Lpath-to-simdutf -lsimdutf
\endcode

Note, that scnlib requires at least C++17,
so --std=c++17 (or equivalent, or newer) may need to be included in the build flags.

\section license License

The library is open source, licensed under the Apache License, version 2.0.  
Copyright (c) 2017 Elias Kosunen  
For further details, see the LICENSE file in the repository.
