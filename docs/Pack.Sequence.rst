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
  separated by spaces to allow for easier reading of the formatting string.
  Endianess formatters ``<`` and ``>`` can be separated from the following
  field definitions because they affect **all of** the following elements
  within the same formatting string.  However, length definitions following
  a field specifier cannot be separated from the specifier by a space.

  .. code:: lua

    -- Sequence of Int5ul, Int2sb, Int1sb, Int2sb, Int1ub
    p1 = Pack( "<I5>i2bhB" )
    p2 = Pack( "<I5 > i2 b h B" )  -- same but more readable


Instance Members
----------------

A ``Pack.Sequence`` has no instance members because access to its members is
controlled via table syntax.  As such the following behaviour has the
following effects:

``Pack.Field pf = Pack.Sequence p[ integer idx ]``
  Returns ``Pack p_idx`` wrapped in ``Pack.Field pf`` which contains the offset
  information of ``Pack p_idx`` within ``Pack.Sequence p``.


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


Potentially unexpected Constructor behaviour
============================================

This will point out some pitfalls which may be the result of misconceptions.
However, the behaviour is fully in-line with other behaviour.


Constructor handling in regard to Endianess
-------------------------------------------

Using the ``Pack.Sequence`` constructor with a single format string vs.
multiple arguments has an effect on endianess.  For example, on an x86 based
machine the default endianess is Little endian, meaning each packer will be
created indiviually using the default or the specified endianess.

.. code:: lua

  p = Pack( "i5",">i2","i4" )
  print( p[1],p[2],p[3] )
  -- t.Pack.Index(Int40sl)   t.Pack.Index(Int16sb)   t.Pack.Index(Int32sl)

``p[3]`` was created with the Little endian default of the machine.  If the
sequence is created with a single format string that defines the entire
sequence an endian modifier sticks until the end of the string or until
another endian modifier is encountered.  ``p[3]`` in this case will be Big
endian.

.. code:: lua

  p = Pack( "i5 >i2 i4" )
  print( p[1], p[2], p[3] )
  -- t.Pack.Index(Int40sl)   t.Pack.Index(Int16sb)   t.Pack.Index(Int32sb)



Constructor handling in regard to mixing atomic and sequence packers
--------------------------------------------------------------------

If a sequence is created by passing multiple arguments to the constructor
the number of elements in the resulting sequence is always equal to the
number of arguments in the constructor.  It is important to keep that in
mind.  Otherwise the following example might be confusing:

.. code:: lua

  p = Pack( "i2", "i3i4", "i6" )
  print( #p )
  -- 3
  print( p[1], p[2], p[3] )
  -- t.Pack.Index(Int16sl)   t.Pack.Index(Sequence[2])   t.Pack.Index(Int48sl)
  print( p[2][1], p[2][2] )
  -- t.Pack.Index(Int24sl)   t.Pack.Index(Int32sl)

This code does **NOT** create a sequence of four integer packers.  Instead
it creates a sequence of ``<Integer> <Sequence of 2 Integers> Integer``,
where ``p[2]`` becomes a nested structure.

