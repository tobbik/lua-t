lua-t Pack.Field - Binary Packer
++++++++++++++++++++++++++++++++


Overview
========

A ``Pack.Field`` is a ``Pack`` that is located at a specific position within
any ``Pack`` combinator such as Struct, Sequence or Array.  As such, a
``Pack.Field`` element is returned by the packer cobinators ``__index
metamethod``.  Depending on preceeding ``Pack`` elements, a ``Pack.Field``
may contain information about the offset in the returning context.

.. code:: lua

  p = Pack( 'hHhH' ) -- packer sequence of 4 shorts, each 2 bytes long
  -- returning the `short` packer at 3rd position of sequence which is 4
  -- bytes offset from the beginning
  print( p[3] )      -- Pack.Field[4](Int2ul)


Reuse of existing ``t.Pack.Field`` definitions
==============================================

When an existing ``t.Pack.Field`` is used to define a new ``t.Pack``
instance its contextual information such as offset is discarded and hence
has no effect on the newly created Packer.

.. code:: lua

  p = Pack( 'ihI' ) -- packer sequence of int, short, Int
  p1= Pack(         -- defining a struct, resusing packers
     { one   = p[2] },
     { two   = p[3] },
     { three = p[1] }
  )
  -- this has exactly the same effect
  p1= Pack(         -- defining a struct, defining packers
     { one   = 'h' },
     { two   = 'I' },
     { three = 'i' }
  )

