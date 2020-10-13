===================
Strings and getline
===================

Reading a ``std::string`` with ``scnlib`` works the same way it does with
``operator>>`` and ``<iostream>``: the input range is read until a whitespace
character or EOF is found. This effectively means that scanning a
``std::string`` reads the input range a "word" at a time.

.. code-block:: cpp

    auto source = scn::make_view("Hello world!");

    std::string word;
    scn::scan(source, "{}", word);
    // word == "Hello"

    scn::scan(source, "{}", word);
    // word == "world!"

If reading word-by-word isn't what you're looking for, you can use
``scn::getline``. It works pretty much the same way as ``std::getline`` does for
``std::string`` s.

.. code-block:: cpp

    // Using the source range from the earlier example
    std::string word;
    // A third parameter could be given, denoting the delimeter
    // Defaults to '\n'
    scn::getline(source, word);
    // word == "Hello world!"
    // The delimeter is not included in the output
