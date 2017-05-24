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

``Pack.Sequence p = Pack( string format )       [__call]``
  Creates ``Pack.Sequence p`` from a siingle argument to ``Pack()`` which is
  a format string that defines multiple packers in a row.

  .. code:: lua
    -- Sequence of UInt5L, Int2B, Int1, int2B, Uint1
    p = Pack("<I5>i2bhB")

``Pack.Sequence p = Pack( pck1, pck2, pck3, ...)       [__call]``
  Creates ``Pack.Sequence p`` from multiple arguments to ``Pack()`` where each
  argument is either a ``t.Pack`` instance, a ''t.Pack.Field`` instance or a
  format string according to the definition for packers.

  .. code:: lua
    p1 = Pack("<I5")
    p2 = Pack("<I2>i2")

    s = Pack( p1, 'B', p2[2] )


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
