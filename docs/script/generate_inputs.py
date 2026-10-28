#!/usr/bin/env python3

# Copyright 2017 Elias Kosunen
# SPDX-License-Identifier: Apache-2.0

"""Copy the Markdown inputs of the documentation into docs/generated.

Run this before poxy (the scn_docs target and the docs workflow do).
The copies work around two bugs, so that the sources don't need to:

 - Doxygen garbles the rest of a page after an inline code span that
   contains `\\"`. In the copies, the quote in it is an HTML entity.
 - poxy links every `@user` in the changelog, even inside the text of an
   existing link, which then breaks. In the copies, the `@` in link text
   is an HTML entity.

poxy hides `&#xNNNN;` entities from Doxygen, and puts them back into the
HTML output. Its sentinel is followed directly by the hex digits, which it
reads greedily, so the entities here always have four digits.
"""

import re
import shutil
from pathlib import Path

DOCS = Path(__file__).resolve().parent.parent
ROOT = DOCS.parent
OUTPUT = DOCS / "generated"

QUOTE = "&#x0022;"
AT = "&#x0040;"

# Fenced code blocks, and Doxygen's \code and \verbatim blocks
CODE_BLOCK = re.compile(
    r"^[ \t]*(```|~~~).*?^[ \t]*\1[^\n]*$"
    r"|[\\@]code\b.*?[\\@]endcode\b"
    r"|[\\@]verbatim\b.*?[\\@]endverbatim\b",
    re.MULTILINE | re.DOTALL,
)
# A code span can't contain a blank line
CODE_SPAN = re.compile(r"(?<!`)(`+)(?!`)((?:[^\n]|\n(?![ \t]*\n))+?)(?<!`)\1(?!`)")
LINK_TEXT = re.compile(r"\[([^\[\]`\n]*)\]\(")


def map_outside(pattern, text, outside, inside=lambda s: s):
    """Apply `outside` to the parts of `text` that `pattern` doesn't match,
    and `inside` to the matches."""
    result = []
    end = 0
    for match in pattern.finditer(text):
        result.append(outside(text[end : match.start()]))
        result.append(inside(match.group(0)))
        end = match.end()
    result.append(outside(text[end:]))
    return "".join(result)


def escape_quotes_in_code_spans(text):
    return CODE_SPAN.sub(
        lambda m: m.group(1) + m.group(2).replace('\\"', "\\" + QUOTE) + m.group(1),
        text,
    )


def escape_ats_in_link_text(text):
    def outside_code_spans(segment):
        return LINK_TEXT.sub(
            lambda m: "[" + m.group(1).replace("@", AT) + "](", segment
        )

    return map_outside(CODE_SPAN, text, outside_code_spans)


def generate(source, destination, *, changelog=False):
    text = source.read_text(encoding="utf-8")

    def transform(segment):
        segment = escape_quotes_in_code_spans(segment)
        if changelog:
            segment = escape_ats_in_link_text(segment)
        return segment

    text = map_outside(CODE_BLOCK, text, transform)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with open(destination, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main():
    shutil.rmtree(OUTPUT, ignore_errors=True)
    generate(ROOT / "CHANGELOG.md", OUTPUT / "CHANGELOG.md", changelog=True)
    for page in sorted((DOCS / "pages").glob("*.md")):
        generate(page, OUTPUT / "pages" / page.name)


if __name__ == "__main__":
    main()
