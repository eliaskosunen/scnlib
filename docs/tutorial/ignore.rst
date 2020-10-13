==========
``ignore``
==========

``scnlib`` has various functions for skipping characters from a range.

``scn::ignore_until(range, ch)`` will skip until ``ch`` is read.

``scn::ignore_n_until(range, n, ch)`` will skip until either ``n`` characters
have been skipped or ``ch`` is read.
