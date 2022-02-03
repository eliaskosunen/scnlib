`[[fill]align][*][width][L][type]`  
`*` 

# type

## strings

### Parsing specifiers

`s` (default): Non-whitespace characters

`L`:  
if `[set]` -> Use locale for parsing, see below  
otherwise, match non-whitespace charaters, according to locale

`[` _set_ `]`:

| Specifier  | Description                                  | Default behavior                |
|:-----------|:---------------------------------------------|:--------------------------------|
| `:alnum:`  | Alphanumeric characters                      | `[a-zA-Z0-9]`                   |
| `:alpha:`  | Letters                                      | `[a-zA-Z]`                      |
| `:blank:`  | Blank characters                             | space and `\t`                  |
| `:cntrl:`  | Control characters                           | `\x0` to `\x1F` and `\x7F`      |
| `:digit:`  | Digits                                       | `[0-9]`                         |
| `:graph:`  | Graphic characters                           | NOT (`\x0` to `\x20` OR `\x7F`) |
| `:lower:`  | Lowercase letters                            | `[a-z]`                         |
| `:print:`  | Printable characters (`graph` + space)       | NOT (`\x0` to `\x1F` OR `\x7F`) |
| `:punct:`  | Punctuation characters                       | `std::ispunct`                  |
| `:space:`  | Whitespace                                   | space, `\t\n\v\f\r`             |
| `:upper:`  | Uppercase letters                            | `[A-Z]`                         |
| `:xdigit:` | Hexadecimal digits                           | `[0-9a-fA-F]`                   |
| `\l`       | Letters (`:alpha:`)                          | `[a-zA-Z]`                      |
| `\w`       | Letters, numbers, underscore (`alnum` + `_`) | `[a-zA-Z0-9_]`                  |
| `\s`       | Whitespace (`space`)                         | space, `\t\n\v\f\r`             |
| `\d`       | Digits (`digit`)                             | `[0-9]`                         |
| `\x__`     | Hex character (2 chars, range 0x00 to 0x7F)  |                                 |
| `\U______` | Unicode codepoint (6 chars, hex)             |                                 |

`l`, `w`, `s`, `d` can be inverted with capitalization: `L`, `W`, `S`, `D`, respectively.  
`^` can also be used for inverting, which must be the first character in the set.

`-` denotes a range (e.g. `0-9`).  
`\:`, `\]` `\^`, `\\` are literal `:`, `]`, `^`, `\`, respectively.

With `L`, characters are classified with `std::ctype` and the supplied locale,
and not according to the table above.

```
[abc] -> a, b, and c
[a-z] -> lower
[a-z-] -> lower + -
[a-z\n\\] -> lower + \n + \
[a-zA-Z0-9] -> alnum
[^a-zA-Z0-9] -> NOT alnum
[:alnum:] -> alnum
[^:alnum:] -> NOT alnum
[\w] -> w = alnum + _
[^\w] -> NOT w
[\W] -> NOT w
[^\W] -> w
L[\w] -> w (localized)
L[:alnum:] -> alnum (localized)
```

## ints

### 1. Number

All are case insensitive

* `b`: binary, accept prefix `0b` or `0B`
* `B__`: custom base, __ in range 2-36, inclusive
* `d`: decimal
* `i`: integer, base determined by prefix
  (`0b` for binary, `0o` or `0` for octal, `0x` for hex, default to decimal)
* `u`: unsigned decimal: disallow negative numbers even for signed types
* `o`: octal, accept prefix `0o`, `0O` or `0`
* `x`: hex, accept prefix `0x` or `0X`
* (default): `d`

### 2. Locale to use

* `L`: Use supplied locale
* (default): Use `C` locale

### 3. Localization

* `n`: Accept locale digits and digit grouping (implies `L`) ???
* (default): Only digits `[0-9]` are accepted

### 4. Thousands separator

* `'`: Accept thousands separators (`L` = use locale, default=`,`)
* (default): Only digits `[0-9]` are accepted

## floats

### 1. Number

All are case insensitive

* `a`, `A`: hex float
* `e`, `E`: scientific format
* `f`, `F`: fixed-precision
* `g`, `G`: general format (`scientific | fixed`)
* (default): detect format (`general | hex`)

### 2. Locale to use

* `L`: Use supplied locale
* (default): Use `C` locale

### 3. Localization

* `n`: Accept locale digits and digit grouping (implies `L`) ???
* (default): If `L`, use locale decimal separator, default=`.`

### 4. Thousands separator

* `'`: Accept thousands separators (`L` = use locale, default=`,`)
* (default): Don't accept thousands separators

## bool

* `L`: Accept locale values
* `s`: Accept string values (`true` and `false` + possible locale values with `L`)
* `i`: Accept int values `0` and `1`
* `n`: `i` + localized digits ???
* (default): `s` + `i`: Accept `0`, `1`, `true` and `false`

## chars

`c` optional, default

## pointers

`p` optional, default
