\page faq FAQ
\tableofcontents

\section faq-1 Why doesn't `scn::scan(input, "{},{}")` work like I expected it to work?

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
`std::sscanf(input, "%s,%s", output1, output2)`.

To reiterate,
the way values are scanned is only influenced by the type of the value to be scanned,
and the options given in the replacement field (inside the `{}`).
The context around the replacement field in the format string is not considered.

To scan until a <code>','</code>, one can do:
`scn::scan<std::string, std::string>("{:[^,]},{}")`.
The `[^,]` in the formatting options for the first argument modifies the scanning behavior to
accept all characters except <code>','</code>. Thus, it won't read until whitespace,
but until a comma, after which the comma is consumed with the literal <code>','</code> in the format string.
