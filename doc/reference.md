# `scnlib` API reference

## Concepts

### `Stream`

A `Stream` is something that the library can read values from.
A `Stream` contains a _stream source_ or just _source_ for short inside of it, which is where the stream gets its input.
The _source_ can be a buffer, a file, a socket, or whatever the `Stream` happens to implement.

In addition, a `Stream` contains a _putback buffer_.
In practice, not all concrete stream types have one, but use the underlying source to their advantage, to achieve the same effect, under the as-if rule.

Every `Stream` has an associated _character type_, which must be either `char` or `wchar_t`.
This is the type of the characters that the external interface uses; the stream source can use whatever character type it likes.


Examples of types satisfying the `Stream` concept are `scn::basic_null_stream<CharT>` and `scn::basic_bidirectional_iterator_stream<const char*>`.

#### Valid expressions

A type `S` satisfies `Stream`, if
 * the type `S` satisfies `MoveConstructible`, `MoveAssignable` and `Destructible`, and
 * lvalues of type `S` satisfy `Swappable`, and

given
 * `s`, an lvalue of type `S`, and
 * `ch`, a value of type `S::char_type`

the following expressions must be valid and have their specified effects:

Expression            | Return type | Requirements 
:-------------------- | :---------- | :----------- 
`S::char_type`        |             | _Character type_ of `s`
`S::is_sized_stream::value`  | `bool` |
`s.read_char()`       | `expected<S::char_type>` | Reads a character from `s`, or returns an error. `s` must be _readable_ 
`s.putback(ch)`       | `error`     | Puts a character back into the _putback buffer_ of `s`. If the operation is successful, `s` will be _readable_ 
`s.bad()`             | `bool`      | Returns `true` if `s` is _bad_  
`(bool)s`             | `bool`      | Equivalent to: `!s.bad()`  
`s.set_roll_back()`   | `error`     | Sets the current state of `s` as the _recovery state_  
`s.roll_back()`       | `error`     | Resets the state of `s` into the _recovery state_   
`s.rcount()`          | `size_t`    | Returns the number of characters read, minus the size of the putback buffer, since the last call to `set_roll_back()` or `roll_back()`  

#### Notes

A `Stream` is _bad_ if some non-recoverable error has occurred.

A `Stream` is _readable_ if the stream is:
 * not _bad,_ and
 * the previous call to `read_char()` did not return an error.

A call to `read_char()` first checks the top of the putback buffer, popping that if there's any characters there, and only if there's none, will it reach for the source.

A `Stream` has a _recovery state_, which is the state of the stream at construction, or after any subsequent `set_roll_back()` call.
This state can be then rolled back to using `roll_back()`.
This functionality can be used for error recovery;
if a higher-level operation fails, and `set_roll_back()` was called before the operation, the stream can be rolled back to the _recovery state_ with `roll_back()`.

`is_sized_stream::value` is `true` iff the type also satisfies `SizedStream` (and `false` otherwise).

If `s.putback` is called, and then the underlying stream source is mutated, the behavior is undefined.
Some concrete stream types may relax this requirement.

## Types

## Functions
