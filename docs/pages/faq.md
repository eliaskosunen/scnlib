\page faq FAQ
\tableofcontents

\section faq-1 Why doesn't scn::scan(input, "{},{}") work like I expected it to work?

The format string given to `scn::scan` is not a pattern, or a regular expression.
Instead, not unlike `std::scanf`, the format string is evaluated left-to-right,
and each replacement field (`{}`) is scanned based on the rules given
by the type to be scanned, and the options inside the replacement field.

For example, `scn::scan<std::string, std::string>(input, "{},{}")` means:
 1. scan a `string` with default options (empty replacement field `{}`),
    which means to scan until whitespace
 2. scan a literal <code>','</code> character, and discard it
 3. scan another `string`, again with default options

Compare this with a call to `scanf`, with the same behavior
(discounting buffer overflow prevention):
`std::sscanf(input, "%%s,%%s", output1, output2)`.

To reiterate,
the way values are scanned is only influenced by the type of the value to be scanned,
and the options given in the replacement field (inside the `{}`).
The context around the replacement field in the format string is not considered.
It's a deliberate design choice that the format strings are not regular expressions,
or can't be used to formulate a regular grammar.

To scan until a <code>','</code>, one can do:
`scn::scan<std::string, std::string>(input, "{:[^,]},{}")`.
The `[^,]` in the formatting options for the first argument modifies the scanning behavior to
accept all characters except <code>','</code>. Thus, it won't read until whitespace,
but until a comma, after which the comma is consumed with the literal <code>','</code> in the format string.

\section faq-standard-mode Can the library and my code be compiled in different C++ standard modes?

Not when scanning `<chrono>` types.
Depending on the standard mode, `scn::month`, `scn::weekday` and the other calendar types
are either the C++20 `std::chrono` types, or scnlib's own types.
These are passed to the compiled part of the library,
so the library and the code using it must be compiled in the same C++ standard mode
(`CMAKE_CXX_STANDARD`): for example, a C++17 build of the library used from C++20 code isn't supported.

\section faq-fixed-width How do I scan fixed-width fields?

Use the width of the replacement fields, which reads exactly that many characters:
`scn::scan<int, int, int>("20260131", "{:4}{:2}{:2}")` gives 2026, 1, and 31.
If a value is shorter than its field, the scan fails with
`scn::scan_error::length_too_short`.
For a field of varying length, give both a width and a precision:
`{:2.4}` reads from 2 to 4 characters.

Whitespace skipped before a value doesn't count toward the width, like in `scanf`.
Because of that, fields padded with leading spaces can't be read this way:
`{:5}` on `"   42"` fails, since the value is only two characters long.

\section faq-regex Is it safe to use regular expressions with untrusted input?

Not with the `std::regex` or Boost.Regex backends, which are backtracking engines:
a crafted pattern and input can take exponential time.
If either the pattern or the input is untrusted, use the `re2` backend (`SCN_REGEX_BACKEND=re2`),
which guarantees linear-time matching.

\section faq-input-retry Why does my scn::input retry loop never end?

A failed `scn::input` doesn't consume any input, so the next call sees the same input again:
`while (!scn::input<int>("{}")) {}` loops forever on `abc`.
Discard the rest of the line before retrying, for example with
`std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n')`
(`scn::input` reads from `std::cin`, unless iostreams are disabled).

Also, like with `scanf("%d\n")`, whitespace at the end of a format string (like in `"{}\n"`)
keeps reading until a non-whitespace character or the end of the input,
which waits for more input when reading from a terminal.

\section faq-input-nested Can I call scn::input inside a custom scanner?

No. `scn::input`, `scn::prompt` and the other functions scanning from stdin hold a lock while they run,
so calling one of them from a scanner used by another one is undefined behavior (in practice, a deadlock).
Read all the values you need in a single call instead.

\section faq-ranges Can I scan containers, tuples and pairs with scn/ranges.h?

`<scn/ranges.h>` is experimental, and its behavior may change.
It lets `scn::scan` read ranges (like `std::vector<int>`), sets, maps, pairs and tuples
in the `[1, 2, 3]` and `(1, 2)` notation of `std::format`.
Limits:
 - Elements that read past the separator or the closing bracket don't work.
   Strings are read until whitespace, so scanning a `std::vector<std::string>` from `[a, b, c]`
   fails with "Expected a range bracket or separator", and so does a `std::vector<char>`,
   because a `char` reads the space after a separator.
   In practice, only element types like numbers work reliably.
 - The range format specifiers of `std::format` (like `n` and separator options) are not supported.
 - A range type with its own `scn::scanner` partial specialization
   (like `template <typename T> struct scn::scanner<my_range<T>, char>`)
   becomes ambiguous when `<scn/ranges.h>` is included. Specialize `scn::range_format_kind` for it,
   inheriting from `std::integral_constant<scn::range_format, scn::range_format::disabled>`.
