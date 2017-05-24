lua-t Pack.Array - Array of Packer Fields
+++++++++++++++++++++++++++++++++++++++++


Overview
========

A ``Pack.Array`` contains ``Pack.Field`` elements or other ``Pack.*``
combinators and allows to access them by a index.  Each field is of the same
``t.Pack`` type.  Internally ``Pack.Array`` is reprented as the definition
of a single ``t.Pack`` instance and as a length.  This saves memory and
overhead.

API
===


Class Metamembers (related to Pack.Array)
-----------------------------------------

``Pack.Array p = Pack( Pack pck, int length)       [__call]``
  Creates ``Pack.Array p`` from a single arguments to ``Pack()`` where pck
  can be one of the following::

    # ``t.Pack`` format string
    # ``t.Pack`` atomic Packer
    # ``t.Pack`` Packer combinator
    # ``t.Pack.Field`` Packer field

  .. code:: lua

    pSeq = Pack("<I2>i2bH")
    p1  = Pack( "r3", 7 )
    p11 = Pack( Pack( "r3"), 7) -- same as p1
    p2  = Pack( pSeq, 20 ) -- array of sequences; access via p[x][y]
    p2  = Pack( pSeq[1], 12 )


Instance Members
----------------

A ``Pack.Array`` has no instance members because access to the values is
controlled via table syntax.  As such the following behaviour has the
following effects:

``Pack.Field pf = Pack.Array p[ integer idx ]``
  Returns ``Pack p`` wrapped in ``Pack.Field pf`` which contains the offset
  information of p within the ``Pack.Array p``.


Instance Metamembers
--------------------

``int i = #( Pack.Array p )  [__len]``
  Return an integer with how many elements ``Pack.Array p`` contains.

``string s = tostring( Pack.Array p )  [__tostring]``
  Returns a string representing the ``Pack.Array p`` instance.  The String
  contains type, length and memory address information such as
  *`t.Pack.Array[11]: 0xdac2e8`*, meaning it has 11 elements.

``function f, table t, val key = pairs( Pack.Array p)  [__pairs]``
  Iterator for ``Pack.Array p``.  It will iterate over the table in proper
  order returning ``integer idx, Pack.Field pf`` for each iteration.

``function f, table t, val key = ipairs( Pack.Array p)  [__ipairs]``
  Iterator for ``Pack.Array p``.  It will iterate over the table in proper
  order returning ``integer idx Pack.Field pf`` for each iteration.
