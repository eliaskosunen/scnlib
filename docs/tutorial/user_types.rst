==========
User types
==========

To make your own types scannable with ``scnlib``, you can specialize the struct
template ``scn::scanner``.

.. code-block:: cpp

    struct my_type {
       int i{};
       double d{};
    };

    template <typename Char>
    struct scn::scanner<Char, my_type>
       : public scn::empty_parser<Char> {
       template <typename Context>
       error scan(my_type& val, Context& c) {
           return scn::scan(c.range(), "[{}, {}]", val.i, val.d);
       }
    };

    // Input: "[123, 4.56]"
    // ->
    //   my_type.i == 123
    //   my_type.d == 4.56

Inheriting from ``scn::empty_parser`` means only an empty format string ``"{}"``
is accepted. You can also implement a ``parse()`` method, or inherit from a
``scn::scanner`` for another type (like ``scn::scanner<Char, int>``) to get
access to additional options.
