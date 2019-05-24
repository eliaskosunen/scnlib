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

An example of a `Stream` is `scn::basic_bidirectional_iterator_stream`.

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
`s.read_char()`       | `expected<S::char_type>` | Reads a character from `s`, or returns an error. Precondition: `s` must be _readable_ 
`s.putback(ch)`       | `error`     | Puts a character back into the _putback buffer_ of `s`. Postcondition: if the operation is successful, `s` will be _readable_ 
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

```cpp
// exposition-only
template <typename S>
concept Stream =
   std::Movable<S> && std::Destructible<S> &&
   requires(S& s, typename S::char_type ch, typename S::is_sized_stream) {
      using char_type = typename S::char_type;
      { s.read_char() } -> expected<char_type>;
      { s.putback(ch) } -> error;
      { s.bad() } -> std::Boolean;
      { s } -> std::Boolean;
      { s.set_roll_back() } -> error;
      { s.roll_back() } -> error;
      { s.rcount() } -> std::size_t;
      { S::is_sized_stream::value } -> std::Boolean;
   }
```

### `SizedStream`

`SizedStream` is a refinement of `Stream`.

The size (number of characters) of a `SizedStream` shall not change after construction.

An example of a `SizedStream` is `scn::basic_static_container_stream`.

#### Valid expressions

A type `S` satisfies `SizedStream`, if
 * the type `S` satisfies `Stream`, and

given
 * `s`, an lvalue of type `S`, and
 * `ch`, a value of type `S::char_type`
 * `sz`, a value of type `std::size_t`
 * `buf`, a value of type `span<S::char_type>`

the following expressions must be valid and have their specified effects:

Expression          | Return type | Requirements 
:------------------ | :---------- | :----------- 
`s.read_sized(buf)` | `void`   | Fills `buf` with characters from `s`. Precondition: `s` must be _readable_ and `s.chars_to_read() >= buf.size()`
`s.putback_n(sz)`   | `void`   | Put back the last `sz` characters read into `s`. Precondition: `s.rcount() >= sz` must be true. Postcondition: `s` will be _readable._ 
`s.chars_to_read()` | `size_t` | Returns the number of characters `s` has available to read. 
`s.skip(sz)`        | `error`  | Skips `sz` characters 
`s.skip_all()`      | `error`  | Skips to the end of `s` 

#### Notes

```cpp
// exposition-only
template <typename S>
concept SizedStream = 
   Stream<S> &&
   requires(S& s, typename S::char_type ch, std::size_t sz, span<typename S::char_type> buf) {
      { s.read_sized(buf) } -> void;
      { s.putback_n(sz) } -> void;
      { s.chars_to_read() } -> std::size_t;
      { s.skip(sz) } -> error;
      { s.skip_all() } -> error;
   }
```

## Types

## Functions
