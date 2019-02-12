# scnlib

[![Travis CI Build Status](https://travis-ci.com/eliaskosunen/scnlib.svg?branch=master)](https://travis-ci.com/eliaskosunen/scnlib)
[![Appveyor CI Build Status](https://ci.appveyor.com/api/projects/status/ex0q59kt5h8yciqa?svg=true)](https://ci.appveyor.com/project/eliaskosunen/scnlib)
[![Codacy Code Quality](https://api.codacy.com/project/badge/Grade/daf649bfab44407fa7afda6cb97add2a)](https://www.codacy.com/app/eliaskosunen/scnlib?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=eliaskosunen/scnlib&amp;utm_campaign=Badge_Grade)
[![License](https://img.shields.io/github/license/eliaskosunen/scnlib.svg)](https://github.com/eliaskosunen/scnlib/blob/doc/LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17%2F20-blue.svg)](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17%2F20-blue.svg)

```cpp
#include <scn/scn.h>
#include <cstdio>

int main() {
    int i;
    scn::prompt("What's your favorite number?", "{}", i);
    printf("Oh, cool, %d!", i);
}

// Example result:
// What's your favorite number? 42
// Oh, cool, 42!
```

## What is this?

`scnlib` is a (work-in-progress) modern C++ library for replacing `scanf` and `std::istream`.
This library attempts to move us ever so closer to replacing `iostream`s and C stdio altogether.
It's (going to, eventually, be) faster than `iostream` (see Benchmarks) and type-safe, unlike `scanf`.
Think [{fmt}](https://github.com/fmtlib/fmt) but in the other direction.

## Installing

### Submodule

The easiest way of integrating `scnlib` to your project would likely be to use a `git submodule`.

```sh
# Replace `external/scnlib` with the directory where you'd like the submodule be located
$ git submodule add external/scnlib https://github.com/eliaskosunen/scnlib
```

Then just do the following in your CMakeLists.txt

```cmake
# Include the submodule directory
add_subdirectory(external/scnlib EXCLUDE_FROM_ALL)

# Target which you'd like to use scnlib
add_executable(my_program ...)
target_link_libraries(my_program scn::scn)
```

### External project

You can also build `scnlib` separately.

```sh
# Whereever you cloned scnlib to
$ mkdir build
$ cd build
$ cmake ..
$ make -j
$ make install
```

Then, in your project:

```cmake
# Find scnlib package
find_package(scn CONFIG REQUIRED)

# Target which you'd like to use scnlib
add_executable(my_program ...)
target_link_libraries(my_program scn::scn)
```

## Usage

More examples can be found from the `examples/` folder.

### Basics

You can read from `stdin` with `scn::input`.
It takes a format string as its first parameters, specifying how to read the parameters.
The following parameters are references to the values to read.

```cpp
// Read an int from stdin
int i;
scn::input("{}", i);
```

`"{}"` as the format string signifies that `scn::input` should read a single value, with default options.
You can read multiple values with a single call:

```cpp
// Read an int, followed by whitespace, and a floating-point value
int i;
double d;
scn::input("{} {}", i, d);
```

With `scn::prompt`, you can also print a message, telling the user what you are expecting them to write.
The function is otherwise identical to `scn::input`, except the first parameter is the message to be printed.

```cpp
// Prints "Gimme integer pls " and reads an int
int i;
scn::prompt("Gimme integer pls ", "{}", i);
```

### Streams

In the core of the public API of `scnlib`, there's `scn::scan`, which reads the given values from a _stream_.
Streams are objects that read characters from a given _source_ object.
There are a number of streams shipped with `scnlib`, all of which can be created with `scn::make_stream`.
In the following example, we are creating a stream which can be used for reading from the source string.

```cpp
std::string source{"42"};
auto stream = scn::make_stream(source);
```

As stated previously, these streams can be read with `scn::scan`.
`scn::scan` is otherwise similar to `scn::input`, except it takes a reference to a stream as its first parameter.

```cpp
// Using the stream from the previous example
int i;
scn::scan(stream, "{}", i);
// Value of i is now 42
```

Streams can be created from multiple different sources:

```cpp
// Containers
std::vector<char> container_source{'1', '2', '3'};
auto container_stream = scn::make_stream(container_source);

// Static buffers
scn::span<char> span_source(container_source.data(), container_source.size());
auto span_stream = scn::make_stream(span_source);

// Pairs of iterators
auto iter_stream = scn::make_stream(container_source.begin(), container_source.end());

// C FILEs
auto cfile_source = fopen("myfile.txt", "r");
auto cfile_stream = scn::make_stream(cfile_source);

// `std::istream`s
// You need to #include <scn/istream.h> as well for this to work
auto cppfile_source = std::ifstream("myfile.txt");
auto cppfile_stream = scn::make_stream(cppfile_source);
```

Because the stream only stores a reference to the source and doesn't take ownership of it,
you need to make sure that the stream outlives the source yourself.

A notable exception to this `make_stream` pattern is the standard input `stdin`.
Even though you could do this:
```cpp
// DON'T DO THIS
auto stdin_stream = scn::make_stream(stdin);
auto cin_stream = scn::make_stream(std::cin);
```
it is likely to lead to synchronization issues between the streams and their underlying sources.

It is better to use `scn::input`, `scn::prompt`, or `scn::scan` with `scn::cstdin()` as its first parameter:

```cpp
int i;
scn::scan(scn::cstdin(), "{}", i);
```

### Format string

Out of the box, `scnlib` supports the reading of following types:
 * `char`
 * `span<char>`
 * `bool`
 * Integral types (`(u)int(8|16|32|64)_t`, expect for `char`)
 * Floating-point types (`float`, `double`, `long double`)
 * `std::string`

To modify the way the values are read, the format string can be used to pass options to the scanner.
These options are put inside the braces `"{}"`.

For `char`, `span<char>` and `std::string` there are no special options, and only the empty option
`"{}"` is valid.

For `bool`, `a` *(for boolalpha) can be used to only accept the text version for the values (`true` and `false`).
By default, only values `0` and `1` are accepted.  
`l` (for localized) can be used to only accept text values as specified by the given locale (see Localization).

By default, for integral types, the scanner detects the base of the number: `0x` or `0X` for hexadecimal, `0` for octal, decimal by default.
Also, `'.'` is used as the decimal point and `','` as the thousands separator.  
`d`, `h`, `o` can be used to explicitly state the base of the number (decimal, hex and octal respectively).  
`b` followed by one or two digits (e.g. `b2` for binary) can also be used to specify the base. 
`bxx` is mutually exclusive with the other base specifiers.  
`n` can be used to use the decimal and thousands separators from the given locale (see Localization).  
`l` can be used to also accept characters that are specified to be digits by the given locale.  
`l` implies `n` and cannot be used simultaneously.

Floating point numbers have a single supported format specifier:
`n`, which uses the decimal and thousands separators from the given locale, instead of the default `'.'` and `','`.

### Localization

To scan localized input, you must use a specialized overload of `scn::scan`, which takes a const-reference to a `std::locale` as a parameter before the string:

```cpp
auto locale = std::locale("fi_FI");

double d;
// ',' is used as the decimal point (comma?)
scn::scan(locale, stream, "{n}", d);
```

If you do not modify the format string, the semantics are equal to just leaving the locale out.
In fact, passing the locale may have some performance implications, and it should probably be avoided if possible.

### Strings and `getline`

Apart from scanning fundamental types, `scnlib` supports scanning of `std::string`s, too.
Using `scn::scan` and its derivatives with `std::string` works the same way like it does with `operator>>`;
the string is first `.clear()`-ed, after which the stream is read until a whitespace character is found or the stream has come to its end.
This effectively means, that `scn::scan`ning a `std::string` reads a word at a time.

```cpp
auto source = std::string{"Hello world!\n"};
auto stream = scn::make_stream(source);

std::string word;
scn::scan(stream, "{}", word);
assert(word == "Hello");

scn::scan(stream, "{}", word);
assert(word == "world!");
```

If reading word-by-word isn't what you are looking for, you can use `scn::getline`.
It works pretty much the same way as `std::getline` does for `std::string`s.

```cpp
// Using the stream from earlier example
std::string word;
// A third parameter could be given, denoting the delimeter
// Defaults to '\n'
scn::getline(stream, word);
assert(word == "Hello world!");
```

### Wide streams

Streams can also be wide, meaning that their character type is `wchar_t` instead of `char`.
This has some usage implications.

The format strings must be wide:
```cpp
scn::scan(stream, L"{}", value);
```

`char`s and `std::string`s cannot be read from a wide stream, but `wchar_t`s and `std::wstring`s can.
```cpp
std::wstring word;
scn::scan(stream, L"{}", word);
```

Wide streams using a `FILE*` as its source must use `make_stream<wchar_t>` or `make_wide_stream` for construction:

```cpp
auto stream = scn::make_wide_stream(stdin);
```

Streams with character types other than `char` and `wchar_t` are not supported, due to lacking support in the standard library for them. Converting between character types is out-of-scope for this
library.

### Error handling

`scnlib` does not use exceptions for error handling.
`scnlib` functions that can fail return a `scn:error` object, containing a `scn::error::code`.

See the API documentation for more details about the error codes.

```cpp
auto ret = scn::scan(...);

if (!ret) {
    // An error occurred

    // Compare to codes with `==`
    if (ret == scn::error::end_of_stream) {
        return;
    }

    // Get code with `.get_code()`
    auto code = ret.get_code();

    // Can the stream be used again?
    auto usable = ret.is_recoverable();
}
```

### TODO

Error handling, `ignore`, `vscan`, user types, lower-level stuff, user stream

## API Documentation

Build CMake target `doc` and look in the folder `doc/html` for Doxygen output.

## Compiler support

Every commit is tested with
 * gcc 4.9 and newer
 * clang 3.5 and newer
 * Visual Studio 2015 and 2017

Older compilers may work, but it is not guaranteed.

## Benchmarks

These benchmarks were run on a Xubuntu 18.04.2 machine running kernel version 4.15.0-45, with an Intel Core i7-8750H processor, and compiled with clang version 6.0.1, with `-O3`.
CPU scaling was enabled for these benchmarks, which may have affected the results.
The source code for the benchmarks can be seen in the `benchmark` directory.

Times are in nanoseconds of CPU time. Lower is better.

### Reading random integers

| Integer type | `scn::scan` | `std::stringstream` |
| :----------- | ----------: | ------------------: |
| `int`        | 79          | 92                  |
| `long long`  | 123         | 146                 |
| `unsigned`   | 76          | 77                  |

### Reading random floating-point numbers

| Floating-point type | `scn::scan` | `std::stringstream` |
| :------------------ | ----------: | ------------------: |
| `float`             | 157         | 240                 |
| `double`            | 163         | 248                 |
| `long double`       | 176         | 256                 |

### Reading random whitespace-separated `std::basic_string`s

| Character type | `scn::scan` | `std::stringstream` |
| :------------- | ----------: | ------------------: |
| `char`         | 52          | 53                  |
| `wchar_t`      | 56          | 116                 |

### Reading random characters

| Character type | `scn::scan` | `std::stringstream` | Control |
| :------------- | ----------: | ------------------: | ------: |
| `char`         | 29          | 9                   | 2       |
| `wchar_t`      | 33          | 14                  | 2       |

TODO: More benchmarks

You can run the benchmarks yourself by enabling `SCN_BUILD_BENCHMARKS` and building the target `bench`.
`SCN_BUILD_BENCHMARKS` is enabled by default if `scn` is the root CMake project, and disabled otherwise.

## Acknowledgements

The contents of this library are heavily influenced by fmtlib and its derivative works.

<https://github.com/fmtlib/fmt>  
<https://fmtlib.net>  
<https://wg21.link/p0645>

fmtlib is licensed under the BSD 2-clause license.  
Copyright (c) 2012-2019 Victor Zverovich

## License

Copyright (c) Elias Kosunen 2017-2019  
Apache License 2.0   
See LICENSE for further details
