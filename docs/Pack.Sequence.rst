lua-t Pack.Sequence - Sequence of Packer Fields
+++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

A ``Pack.Sequence`` contains ``Pack.Field`` elements or other ``Pack.*``
combinators and allows to access them by a index.

API
===


Class Metamembers (related to Pack.Sequence)
--------------------------------------------

``Pack.Sequence p = Pack( pck1, pck2, pck3, ...)       [__call]``
  Creates ``Pack.Sequence p`` from multiple arguments to ``Pack()`` where each
  argument is either a ``t.Pack`` instance, a ''t.Pack.Field`` instance or a
  format string according to the definition for packers.

  .. code:: lua

    p1 = Pack( "<I5" )
    p2 = Pack( "<I2 >i2" )

    s = Pack( p1, 'B', p2[2] )

  Endianess setters effect only the element they are specified for and have
  to be specified for each field.  Otherwise the default endianess will be
  used.

``Pack.Sequence p = Pack( string format )       [__call]``
  Creates ``Pack.Sequence p`` from a single argument to ``Pack()`` which is
  a format string that defines multiple packers in a row.  This is a
  convienience constructor that does not require each and every element of
  the sequence to be provided explicitely.  Field definitions can be
  separated by spaces to allow foreasier to read formatting.  Endianess
  formatters ``<`` and ``>`` can be separated from the following field
  definitions because they affect **all of** the following elements.
  However, length definitions following a filed specifier cannot be
  separated from the specifier by a space.

  .. code:: lua

    -- Sequence of Int5ul, Int2sb, Int1sb, Int2sb, Int1ub
    p1 = Pack("<I5>i2bhB")
    p2 = Pack("<I5 >i2 b h B")  -- same but more readable


Instance Members
----------------

A ``Pack.Sequence`` has no instance members because access to the values is
controlled via table syntax.  As such the following behaviour has the
following effects:

``Pack.Field pf = Pack.Sequence p[ integer idx ]``
  Returns ``Pack p`` wrapped in ``Pack.Field pf`` which contains the offset
  information of p within the ``Pack.Sequence p``.


Instance Metamembers
--------------------

``int i = #( Pack.Sequence p )  [__len]``
  Return an integer with how many elements ``Pack.Sequence p`` contains.

``string s = tostring( Pack.Sequence p )  [__tostring]``
  Returns a string representing the ``Pack.Sequence p`` instance.  The String
  contains type, length and memory address information such as
  *`t.Pack.Sequence[11]: 0xdac2e8`*, meaning it has 11 elements.

``function f, table t, val key = pairs( Pack.Sequence p)  [__pairs]``
  Iterator for ``Pack.Sequence p``.  It will iterate over the table in proper
  order returning ``integer idx, Pack.Field pf`` for each iteration.

``function f, table t, val key = ipairs( Pack.Sequence p)  [__ipairs]``
  Iterator for ``Pack.Sequence p``.  It will iterate over the table in proper
  order returning ``integer idx Pack.Field pf`` for each iteration.
