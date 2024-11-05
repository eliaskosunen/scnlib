\mainpage scnlib
\tableofcontents

`scnlib` is a modern C++ library for scanning values.
Think of it as a more C++-y `scanf`, or the inverse of
<a href="https://fmt.dev">{fmt}</a> / `std::format`.

This library is the reference implementation of the
ISO C++ standards proposal
<a href="https://wg21.link/p1729">P1729 "Text Parsing"</a>.

The code lives over at GitHub, at https://github.com/eliaskosunen/scnlib.

\code{.cpp}
#include <scn/scan.h>
#include <print> // for std::println, C++23

int main() {
    auto result = scn::scan<int, double>("42, 3.14", "{}, {}");
    if (result) {
        auto [first, second] = result->values();
        std::println("first: {}, second: {}", first, second);
        // Prints "first: 42, second: 3.14\n"
    }
}
\endcode

\section main-about About this documentation

This documentation is for the version 4.0 of the library.
For other versions, see the dropdown in the upper right corner of the website.
See <b><a href="./md_poxy_changelog.html">Changelog</a></b> for more.

An introductory guide to the library can be found at \ref guide "Guide".
Answers to frequently asked questions are listed at \ref faq "FAQ".

The API documentation is organized into modules, that can be found under Modules, behind the link at the top of the page.
It can be searched directly using the search function in the navbar, or by pressing the TAB key.
The most important modules are probably:

 - \ref scan 
 - \ref scannable
 - \ref format-string
 - \ref result

\section main-install Installation

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
        GIT_TAG         v4.0.1
        GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(scn)

# ...

target_link_libraries(my_program scn::scn)
\endcode

\subsection main-deps Dependencies

scnlib internally optionally depends on
<a href="https://github.com/fastfloat/fast_float">fast_float</a>.

By default, the CMake machinery automatically fetches, builds, and links it with `FetchContent`.
It's only used in the implementation, and it isn't visible to the users of the library.

Alternatively, by setting the CMake option `SCN_USE_EXTERNAL_FAST_FLOAT` to `ON`,
fast_float is searched for using `find_package`. Use this option
if you already have the library installed.

To enable support for regular expressions, a regex engine backend is required.
The default option is to use `std::regex`, but an alternative can be picked
by setting `SCN_REGEX_BACKEND` to `Boost` or `re2` in CMake.
These libraries are not downloaded with `FetchContent`, but must be found externally.

The tests and benchmarks described below depend on GTest and Google Benchmark, respectively.
These libraries are also fetched with `FetchContent`, if necessary,
controllable with `SCN_USE_EXTERNAL_GTEST` and `SCN_USE_EXTERNAL_BENCHMARK`, respectively.

<table>
<caption>
Library dependencies
</caption>

<tr>
<th>Dependency</th>
<th>Version</th>
<th>Required</th>
<th>Information</th>
</tr>

<tr>
<td>fast_float</td>
<td>`>= 5.0.0`</td>
<td>⚠️</td>
<td>Required if `SCN_DISABLE_FAST_FLOAT` is `OFF`.<br>Header only. Downloaded by default with `FetchContent`, controlled with `SCN_USE_EXTERNAL_FAST_FLOAT`.</td>
</tr>

<tr>
<td>Boost.Regex</td>
<td>N/A</td>
<td>⚠️</td>
<td>Required if `SCN_REGEX_BACKEND` is `Boost`.<br>Must be available externally (not automatically downloaded).</td>
</tr>

<tr>
<td>re2</td>
<td>`>= 11.0.0`</td>
<td>⚠️</td>
<td>Required if `SCN_REGEX_BACKEND` is `re2`.<br>Must be available externally (not automatically downloaded).</td>
</tr>
</table>

\subsection main-tests Tests and benchmarks

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

\subsection main-without-cmake Without CMake

As mentioned above, the implementation of scnlib depends on fast_float.
If you're not using CMake, you'll need to download it yourself, and
make it available for the build.
Since fast_float is a header-only library, it doesn't need to be built.

Headers for scnlib can be found from the `include/` directory, and source files from the `src/` directory.

Building and linking the library:

\code{.sh}
$ mkdir build
$ cd build
$ c++ -c -I../include/ ../src/*.cpp -Ipath-to-fast-float
$ ar rcs libscn.a *.o
\endcode

`libscn.a` can then be linked, as usual, with your project.

\code{.sh}
# in your project
$ c++ ... -Lpath-to-scn/build -lscn
\endcode

Note, that scnlib requires at least C++17,
so `--std=c++17` (or equivalent, or newer) may need to be included in the build flags.

\subsection configuration Configuration

There are numerous flags that can be used to configure the build of the library.
All of them can be set through CMake at library build time, and some of them
directly on the compiler command line, as indicated on the tables below.

<table>
<caption>
scnlib configuration options
</caption>

<tr>
<th>Option</th>
<th>CMake</th>
<th>CLI</th>
<th>Default</th>
<th>Description</th>
</tr>

<tr>
<td>`SCN_DISABLE_FAST_FLOAT`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable external dependency on FastFloat.<br>Using `ON` requires standard library support for floating-point `std::from_chars`.<sup>1</sup></td>
</tr>

<tr>
<td>`SCN_DISABLE_REGEX`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable regular expression support</td>
</tr>

<tr>
<td>`SCN_REGEX_BACKEND`</td>
<td>✅</td>
<td>⚠️</td>
<td>`"std"`</td>
<td>Regular expression backend to use<br> (use integer values on the command line)<br>Values: `"std"=0`, `"Boost"=1`, `"re2"=2`</td>
</tr>

<tr>
<td>`SCN_REGEX_BOOST_USE_ICU`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Use the ICU when using the Boost regex backend<br>(requires `SCN_REGEX_BACKEND` to be `Boost`/`1`)</td>
</tr>

<tr>
<td>`SCN_DISABLE_IOSTREAM`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable everything related to iostreams</td>
</tr>

<tr>
<td>`SCN_DISABLE_LOCALE`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable everything related to C++ locales (`std::locale`)</td>
</tr>

<tr>
<td>`SCN_DISABLE_FROM_CHARS`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable usage of (falling back on) `std::from_chars` when scanning floating-point values</td>
</tr>

<tr>
<td>`SCN_DISABLE_STRTOD`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable usage of (falling back on) `std::strtod` when scanning floating-point values</td>
</tr>

<tr>
<td>`SCN_DISABLE_CHRONO`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable all `&lt;chrono&gt;` and `&lt;ctime&gt;` scanners</td>
</tr>

<tr>
<td>`SCN_DISABLE_(TYPE)`</td>
<td>✅</td>
<td>✅</td>
<td>`OFF`</td>
<td>Disable support for a specific type</td>
</tr>
</table>

<sup>1</sup>: As on October 2024, `std::from_chars` with floating-point values is supported on libstdc++ v11 and newer,
and MSVC 19.24 (VS 2019 16.4) or newer. libc++ doesn't provide any support yet.

Below, `ENABLE_FULL` is true, if `SCN_CI` is set in CMake, or scnlib
is built as the primary project.

<table>
<caption>
CMake build type configuration
</caption>

<tr>
<th>Option</th>
<th>CMake</th>
<th>CLI</th>
<th>Default</th>
<th>Description</th>
</tr>

<tr>
<td>`SCN_USE_EXTERNAL_FAST_FLOAT`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Use `find_package` to get fast_float, instead of CMake `FetchContent`</td>
</tr>

<tr>
<td>`SCN_USE_EXTERNAL_GTEST`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Use `find_package` to get GTest, instead of CMake `FetchContent`</td>
</tr>

<tr>
<td>`SCN_USE_EXTERNAL_BENCHMARK`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Use `find_package` to get Google Benchmark, instead of CMake `FetchContent`</td>
</tr>

<tr>
<td>`SCN_TESTS`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable building of tests</td>
</tr>

<tr>
<td>`SCN_DOCS`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable building of documentation<br>(`scn_docs` target, requires `poxy`)</td>
</tr>

<tr>
<td>`SCN_EXAMPLES`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable building of examples</td>
</tr>

<tr>
<td>`SCN_INSTALL`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable `install` target</td>
</tr>

<tr>
<td>`SCN_BENCHMARKS`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable building of (runtime) benchmarks</td>
</tr>

<tr>
<td>`SCN_BENCHMARKS_BUILDTIME`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable build-time benchmark targets</td>
</tr>

<tr>
<td>`SCN_BENCHMARKS_BINARYSIZE`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable binary-size benchmark targets</td>
</tr>

<tr>
<td>`SCN_FUZZING`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable building of fuzzer targets</td>
</tr>

<tr>
<td>`SCN_PEDANTIC`</td>
<td>✅</td>
<td>❌</td>
<td>`ENABLE_FULL`</td>
<td>Enable pedantic compilation flags (mostly warnings)</td>
</tr>

<tr>
<td>`SCN_WERROR`</td>
<td>✅</td>
<td>❌</td>
<td>`SCN_CI`</td>
<td>Enable warnings-as-errors</td>
</tr>

<tr>
<td>`SCN_COVERAGE`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Enable code coverage reporting</td>
</tr>

<tr>
<td>`SCN_USE_NATIVE_ARCH`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Add `-march=native`</td>
</tr>

<tr>
<td>`SCN_USE_HASWELL_ARCH`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Add `-march=haswell`<br>(used for benchmarking to get a more recent, but reasonable architecture)</td>
</tr>

<tr>
<td>`SCN_USE_ASAN`, `_UBSAN`, `_MSAN`</td>
<td>✅</td>
<td>❌</td>
<td>`OFF`</td>
<td>Enable sanitizers</td>
</tr>
</table>

\section main-license License

The library is open source, licensed under the Apache License, version 2.0.  
Copyright (c) 2017 Elias Kosunen  
For further details, see the LICENSE file in the repository.
