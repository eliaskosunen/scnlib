# scnlib

[![Travis CI Build Status](https://travis-ci.com/eliaskosunen/scnlib.svg?branch=master)](https://travis-ci.com/eliaskosunen/scnlib)
[![Appveyor CI Build Status](https://ci.appveyor.com/api/projects/status/ex0q59kt5h8yciqa/branch/master?svg=true)](https://ci.appveyor.com/project/eliaskosunen/scnlib/branch/master)
[![Codecov Coverage](https://codecov.io/gh/eliaskosunen/scnlib/branch/master/graph/badge.svg)](https://codecov.io/gh/eliaskosunen/scnlib)
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

_There's bound to be bugs, there's no documentation apart from this file, `master` branch is going to get rebased and
public APIs are going to change in backwards-incompatible ways._

## Table of Contents

* [What is this?](#what-is-this)
* [Installing](#installing)
   * [Submodule](#submodule)
   * [External project](#external-project)
* [Usage](#usage)
   * [Basics](#basics)
   * [Streams](#streams)
   * [Supported types](#supported-types)
   * [Format string](#format-string)
   * [Options](#options)
   * [Strings and getline](#strings-and-getline)
   * [Wide streams](#wide-streams)
   * [Encoding and Unicode](#encoding-and-unicode)
   * [Error handling](#error-handling)
   * [`ignore`](#ignore)
   * [`getchar`](#getchar)
   * [User types](#user-types)
   * [Range-based interface](#range-based-interface)
   * [`scanf` syntax](#scanf-syntax)
   * [TODO docs](#todo-docs)
* [Compiler support](#compiler-support)
* [Benchmarks](#benchmarks)
   * [Run-time performance](#run-time-performance)
   * [Code size](#code-size)
* [Acknowledgements](#acknowledgements)
* [License](#license)

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
# Use scn::scn-header-only if you'd prefer
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
auto iter_stream = scn::make_stream(container_source.begin(),
                                    container_source.begin() + 2);

// C FILEs
auto cfile_source = fopen("myfile.txt", "r"); // Remember to call fclose yourself
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

### Supported types

 * Characters: `char` for narrow streams or `wchar_t` for wide streams
 * Spans: Contiguous buffer views, `span<char>` for narrow streams, `span<wchar_t>` for wide streams
 * Strings: `std::string` for narrow streams, `std::wstring` for wide streams
 * Integral types: Both signed and unsigned varieties of `short`, `int`, `long` and `long long` for narrow streams.
 * Floating-point types: `float`, `double` and `long double`.
 * `bool`
 * User-defined custom types, see "User types"

### Format string

Every value to be scanned from the input stream is marked with a pair of curly braces `"{}"` in the format string.
Inside these braces, additional options can be specified.

The syntax is not dissimilar from the one found in `{fmt}`.

For characters, spans and strings, there are no available options, and only the empty string `"{}"` is valid.

#### Integral types

| Base specifier | Meaning               |
| :------------- | :-------------------- |
| `d`            | Decimal (base-10)     |
| `h`            | Hexadecimal (base-16) |
| `o`            | Octal (base-8)        |
| `b__`          | Custom base. `b` followed by one or two digits (e.g. `b2` for binary). Base must be between 2 and 36, inclusive. |
| (default)      | Detect base. `0x`/`0X` prefix for hexadecimal, `0` prefix for octal, decimal by default |

| Localization | Meaning                                                    |
| :----------- | :--------------------------------------------------------- |
| `n`          | Use decimal and thousands separator from the given locale. |
| `l`          | Implies `n`. Accept characters specified as digits by the given locale. |
| (default)    | Use `.` as decimal point, `,` as thousands separator and `[0-9]` as digits. |

#### Floating-point types

| Localization | Meaning                                                    |
| :----------- | :--------------------------------------------------------- |
| `n`          | Use decimal and thousands separator from the given locale. |
| (default)    | Use `.` as decimal point, `,` as thousands separator. |

#### `bool`

|              | Meaning                                                    |
| :----------- | :--------------------------------------------------------- |
| `a`          | Accept only `true` and `false` (not `0` and `1`, unless `n` is also specified). |
| `n`          | Accept only `0` and `1` (not `true` and `false`, unless `a` is also specified). |
| `l`          | Implies `a`. Expect boolean text values as specified as such by the given locale. |
| (default)    | Accept `0`, `1`, `true` and `false`, equivalent to `an`. |

### Options

An `scn::options` object can be passed as the first argument to `scn::scan` or `scn::input`,
determining additional options for the scanning operation.
`scn::options` cannot be constructed directly, but one can be created with `scn::options::builder`, like so:

```cpp
scn::scan(scn::options::builder{}.make(), /* ... */); 
```

#### Localization

A const-reference to a `std::locale` can be passed to `builder::locale` for scanning localized input.

```cpp
auto loc = std::locale("fi_FI");

int a, b;
scn::scan(
    scn::options::builder{}
        .locale(loc)
        .make(),
    "{} {:n}", a, b);
```

Only reading of `b` will be localized, as it has `{n}` as its format string.
Redundant specification of the locale should be avoided, as it can have some performance implications.

#### Scanning method

An enumeration value of `scn::method` can be passed to `builder::int_method` or `builder::float_method`,
that specifies how the value will be scanned.

| Value             | Meaning |
| :---------------- | :------ |
| `strto` (default) | Use `std::strtol`, `std::strtod` etc. |
| `sto`             | Use `std::stol`, `std::stod` etc. |
| `from_chars`      | Use `std::from_chars` |
| `custom`          | Use custom `scnlib` algorithm |

```cpp
scn::scan(
    scn::options::builder{}
        .int_method(scn::method::sto) 
        .float_method(scn::method::sto) 
        .make(),
    /* ... */);
```

Please note, that:
 * `from_chars` needs support from your standard library. At the time of writing, this only includes the most recent MSVC, and only for integer types.
 * `custom` is a little faster than the other alternatives with the added caveat of being highly likely to contain bugs.
 * `custom` at this time only supports integer types

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

### Encoding and Unicode

Because of the rather lackluster Unicode support of the standard library,
this library doesn't have any significant Unicode support, either. 

Narrow streams are expected to be ASCII encoded,
and using multibyte encodings (like UTF-8) with them is probably going to cause problems (blame `std::locale`).
If you need some sort of Unicode support, your best bet is going to be wide streams, encoded the way your platform
expects (UTF-32 in POSIX, the thing resembling UCS-2 in Windows).

### Error handling

`scnlib` does not use exceptions for error handling.
Instead, `scan`, `input`, `prompt` and `vscan` return a `result<int>`, which is an object containing
an integer telling the number of arguments successfully read, and an `scn::error` object.

```cpp
// successful read:
auto ret = scn::scan(stream, "{}", value);
assert(ret);
assert(ret.value() == 1);
assert(ret.error());

// failing read:
ret = scn::scan(stream, "{}", value);
assert(!ret);
assert(ret.value() == 0);
assert(!ret.error());
```

Other items of the `scnlib` public interface, like `ignore` or `getline` will only return a `scn::error`.

The `scn::error` object can be examined further. It contains an error code `scn::error::code`,
accessible with member function `code()` and a message, that can be get with `msg()`.

```cpp
auto ret = scn::scan(stream, "{}", value);
if (!ret) {
   std::cout << "Read failed with message: '" << ret.error().msg() << "'\n";
}
```

Please note, that EOF is also an error, with error code `scn::error::end_of_stream`.

If the error is of such quality that it cannot be recovered from and the stream is deemed as bad,
the member function `is_recoverable` will return `false` and `stream::bad()` will return `true`.

See the source code (`include/scn/detail/result.h`) for more details about the error codes.

#### Error guarantees

Should the reading of any of the arguments fail, and the stream is not bad,
the stream state will be reset to what it was before the reading of said argument,
and the argument will not be written to.

```cpp
auto source = std::string{"123 foo"};
auto stream = scn::make_stream(source);

int i{}, j{};
// "foo" cannot be read to an integer, so this will fail
auto ret = scn::scan(stream, "{} {}", i, j);
assert(!ret);
// First read succeeded
assert(ret.value() == 1);
assert(i == 123);
// Second read failed, value was not touched
assert(j == 0);
assert(ret.error().code() == scn::error::invalid_scanned_value);

// The stream now contains "foo", as it was reset to the state preceding the read of j
std::string s{};
ret = scn::scan(stream, "{}", s);
// This succeeds
assert(ret);
assert(s == "foo");
```

#### Exceptions

No exceptions will ever be thrown by `scnlib` functions.
If any user-defined operations, like `operator>>` throw, the behavior is undefined.

The library can be compiled with `-fno-exceptions`, but some of its functionality will be disabled, namely `sto` method for integer and float scanning.

### `ignore`

`scnlib` has various functions for skipping characters from a stream.  
`ignore_n(stream, n)` will skip `n` characters.  
`ignore_until(stream, ch)` will skip until `ch` is read.  
`ignore_n_until(stream, n, ch)` will skip until either `n` characters have been skipped or `ch` is read.  
`ignore_all(stream)` will skip to the end of the stream.

### `getchar`

`getchar(stream)` will read a single character (`char` for narrow streams, `wchar_t` for wide streams) from a stream.

### User types

To make your own type usable with `scnlib`, you can specialize the struct template `scanner`.

```cpp
struct my_type {
   int i{};
   double d{};
};

template <typename Char>
struct scn::scanner<Char, my_type>
   : public scn::empty_parser<Char> {
   template <typename Context>
   error scan(my_type& val, Context& c) {
      auto args = make_args<Context>(val.i, val.d);
      auto ctx = Context(c.stream(), "[{}, {}]", args);
      return vscan(ctx);
   }
};

// Input: "[123, 4.56]"
// ->
//   my_type.i == 123
//   my_type.d == 4.56
```

### Range-based interface

`scnlib` also supports a range-based interface, using [range-v3](https://github.com/ericniebler/range-v3).
To use the interface, range-v3 must be installed on your system, you must include the header file `<scn/ranges.h>`, and your program must link against the `scn::scn-ranges` CMake target.

```cmake
target_link_libraries(your-target scn::scn-ranges)
# or scn::scn-ranges-header-only if you wish
```

The interface resides in the namespace `scn::ranges`. Instead of a stream, `scn::ranges::scan` takes a `Range` as its first argument.

```cpp
#include <scn/ranges.h>
// ...
auto range = std::string{"Hello"};
std::string str{};
auto ret = scn::ranges::scan(range, "{}", str);
assert(str == "Hello");
assert(ret);
assert(ret.value() == 1);
```

The `scn::ranges::scan` return value has a member function `.iterator()`, which returns an iterator past the last read character of the stream.

```cpp
// ret from snippet above
assert(ret.iterator() == range.end());
```

The iterator can be used to use the same source range for multiple `scn::ranges::scan` calls.

```cpp
auto range = std::string{"Hello world"};
std::string str{};

auto ret = scn::ranges::scan(range, "{}", str);
assert(str == "Hello");
// ret.iterator points to 'w' in "world"
assert(ret.iterator() == range.begin() + str.length() + 1);

// scn::ranges::subrange_from returns a view
// to a range (second argument) starting from an iterator (first argument)
// It's meant for making ranges for `scn::ranges::scan`
ret = scn::ranges::scan(scn::ranges::subrange_from(ret.iterator(), range), "{}", str);
assert(str == "world");
assert(ret.iterator() == range.end());

```

### `scanf` syntax

With `scn::scanf`, a `scanf`-like format string syntax can be used.
For more information about the syntax, check `man scanf` or some other C standard library document.
`scn::ranges::scanf` is also available.

```cpp
int i{};
scn::scanf(stream, "%d", i);
```

### TODO docs

`vscan`, lower-level stuff, erased stream, user stream

## Compiler support

Every commit is tested with
 * gcc 5.5 and newer
 * clang 3.6 and newer
 * Visual Studio 2015 and 2017

Older compilers may work, but it is not guaranteed.

## Benchmarks

### Run-time performance

These benchmarks were run on a Xubuntu 18.04.2 machine running kernel version 4.15.0-45, with an Intel Core i7-8750H processor, and compiled with clang version 6.0.1, with `-O3`.
CPU scaling was enabled for these benchmarks, which may have affected the results.
The source code for the benchmarks can be seen in the `benchmark` directory.

Times are in nanoseconds of CPU time. Lower is better.

#### Reading random integers

| Integer type | `scn::scan` (`strto`) | `scn::scan` (`sto`) | `scn::scan` (`custom`) | `std::stringstream` |
| :----------- | --------------------: | ------------------: | ---------------------: | ------------------: |
| `int`        | 119                   | 129                 | 94                     | 101                 |
| `long long`  | 136                   | 110                 | 178                    | 156                 |
| `unsigned`   | 107                   | 121                 | 91                     | 85                  |

#### Reading random floating-point numbers

| Floating-point type | `scn::scan` (`strto`) | `scn::scan` (`sto`) | `std::stringstream` |
| :------------------ | --------------------: | ------------------: | ------------------: |
| `float`             | 208                   | 207                 | 253                 |
| `double`            | 211                   | 211                 | 260                 |
| `long double`       | 225                   | 225                 | 273                 |

#### Reading random whitespace-separated `std::basic_string`s

| Character type | `scn::scan` | `std::stringstream` |
| :------------- | ----------: | ------------------: |
| `char`         | 75          | 56                  |
| `wchar_t`      | 92          | 126                 |

#### Reading random characters

| Character type | `scn::scan` | `scn::getchar` | `std::stringstream` |
| :------------- | ----------: | -------------: | ------------------: |
| `char`         | 41          | 14             | 15                  |
| `wchar_t`      | 41          | 14             | 22                  |

TODO: More benchmarks

You can run the benchmarks yourself by enabling `SCN_BUILD_BENCHMARKS` and building the target `bench`.
`SCN_BUILD_BENCHMARKS` is enabled by default if `scn` is the root CMake project, and disabled otherwise.

### Code size

Code size benchmarks test code bloat for nontrivial projects.
It generates 25 translation units and reads values from stdin\* five times to simulate a medium sized project.
The resulting executable size is shown in the following tables.

The code was compiled on Kubuntu 18.10 with g++ 8.2.0.
`scnlib` is linked dynamically to level out the playing field compared to already dynamically linked `libc` and `libstdc++`.
See the directory `benchmark/bloat` for more information, e.g. templates for each TU.

To run these tests yourself:

```sh
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DSCN_INSTALL=OFF -DSCN_BLOAT=ON ..
$ make -j
$ ctest -V
```

\*: `scn::ranges::scan` scans from a `Range`, not from stdin

`(erased)` marks the usage of `scn::make_erased_stream()` instead of `scn::make_stream()`.

#### Release build (-O3 -DNDEBUG)

| Method                              | Executable size (KiB) | Stripped size (KiB) |
| :---------------------------------- | --------------------: | ------------------: |
| empty                               | 20                    | 16                  |
| `scanf`                             | 24                    | 20                  |
| `std::istream` / `std::cin`         | 32                    | 24                  |
| `scn::input`                        | 92                    | 80                  |
| `scn::input` (erased)               | 80                    | 72                  |
| `scn::ranges::scan`                 | 148                   | 132                 |
| `scn::input` (header only)          | 280                   | 240                 |
| `scn::input` (header only & erased) | 260                   | 216                 |
| `scn::ranges::scan` (header only)   | 312                   | 196                 |

#### Debug build (-g)

| Method                              | Executable size (KiB) | Stripped size (KiB) |
| :---------------------------------- | --------------------: | ------------------: |
| empty                               | 32                    | 16                  |
| `scanf`                             | 276                   | 20                  |
| `std::istream` / `std::cin`         | 308                   | 24                  |
| `scn::input`                        | ~2300                 | 72                  |
| `scn::input` (erased)               | ~2100                 | 64                  |
| `scn::ranges::scan`                 | ~5700                 | 196                 |
| `scn::input` (header only)          | ~9500                 | 392                 |
| `scn::input` (header only & erased) | ~8900                 | 380                 |
| `scn::ranges::scan` (header only)   | ~9700                 | 384                 |

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
