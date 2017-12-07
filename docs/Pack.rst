lua-t Pack - The Binary Packer
++++++++++++++++++++++++++++++++


Overview
========

A Bit and Byte packer using the same kind of formatting string as Lua 5.3
``string.pack( )`` and ``string.unpack( )`` with two notable exceptions:

   #. it does not deal with alignment or variable length strings
   #. it can pack and unpack Bit wide resolution

``Pack`` preferably works on ``Buffer`` objects, because they are mutable.
This way it is possible to overwrite only parts of a buffer if only selected
fields should be changed.  This safes memory because it prevents Lua from
creating and interning  new strings for each write.

t.Pack format strings
=====================

For Bit Style packing lua-t introduces the following format strings

 - **v:** a boolean represented as a single bit.
 - **r:** a signed and unaligned Integer up to native size.  Size is
   defined in bits.  It can span byte boundaries.
 - **R:** an unsigned an unaligned Integer up to native size.  Size is
   defined in bits.  It can span byte boundaries.

The following formats are missing and probably will be implemented at a
later date:

 - **![n]**  Alignment is not considered yet
 - **=**     Endianess must be explicitely set
 - **z**     C-Strings; zero terminated
 - **s[n]**  Pascal-Strings; prefixed with a interger determining size


t.Pack types
------------

``Pack`` objects can come in multiple flavours.  There is a main separation
between atomic packers and packer collections.  The access to packer
collections follows the same syntax as Lua tables.  Items in packer
collections can be packer collections themselves (nesting).

atomic
  A single byteType or bitType packer which returns a scalar value such as a
  boolean, integer, float or string.

sequence
  Multiple values that are packed in order defined by the format string.

array
  A collection of ``n`` packers of the same type.

struct
  An ordered collection of named packers.


Pack identification
---------------------

``Pack`` objects can identify themselves via a string.  The string is
composed of different elements which vary slightly from type to type.  The
general composition follows a simple schema:

   Type Length Endianess

Type can be any of the following:

   - Int          (includes byte, short, long, LuaInteger)
   - UInt         (includes unsigned byte, short, long, LuaInteger)
   - Float        (includes double and LuaNumber)
   - Boolean      single bit representing a flag
   - BitSigned    Bitfield representing signed integer
   - BitUnsigned  Bitfield representing unsigned integer
   - Raw          string/utf8/binary
   - Array        Array Combinator
   - Sequence     Sequence Combinator
   - Struct       Struct Combinator

What kind of a packer is created is controlled by the constructor.  The
``Pack()`` constructor takes the following paramters and creates the
following datatypes:

atomic
  The constructor takes a format string which defines a single atomic item.
  eg. ``p = Pack( '<I3' )`` defines a little endian unsigned integer of 3
  bytes width (UInt3L).

  An interesting aspect about atomic packers is that they are perfectly
  immutable and therefore don't have to be recreated.  The library performs
  some lazy caching in order to prevent the creation of unnecessary copies:

  .. code:: lua

   Pack = require't.Pack'
   p1, p2, p3 = Pack'R4', Pack'v', Pack'R4'
   print(p1,p2,p3)
   -- Note that p1 and p3 have the same address
   -- t.Pack.Int4ub: 0x55814d1601a8  t.Pack.Bool: 0x55814d1afeb8     t.Pack.Int4ub: 0x55814d1601a8
   print( Pack[ 'Int4ub' ])
   -- t.Pack.Int4ub: 0x55814d1601a8
   -- caching happens by adding a
   -- reference to the internal library table

sequence
  The constructor takes a format string which defines a composition of
  multiple items.  eg. ``p = Pack( '>l', '<H', 'i6' )`` defines a sequence
  of 3 elements and is 16 bytes long on a 64 bit system::

   - p[1]: atomic packer of type (Int8sb) 0  bytes offset (1st element)
   - p[2]: atomic packer of type (Int2ul) 8  bytes offset (length of p[1]+p[2])
   - p[3]: atomic packer of type (Int6sl) 10 bytes offset (length of p[1])

  As a convienience a packer sequence can be created providing just one
  single concatenated string to the constructor.  ``p = Pack( '>l<Hi6' )``
  creates exactly the same sequence as the example above.  This convienience
  creates a peculiar behaviour of the sequence constructor which is very
  logical but may lead to some confusion.  As mentioned, while the call to
  ``p1 = Pack( '>l<Hi6' )`` creates the same sequence as the call to ``p2 = 
  Pack( '>l', '<H', 'i6' )`` the call to ``p3 = Pack( '>l<H', 'i6' )`` is
  different!  It will create a sequence of 2 packers, the first being a
  sequence of 2 atomic integer packers and the second being just a single
  atomic integer packer:

  .. code:: lua

   p1, p2, p3 = Pack( '>l<Hi6' ), Pack( '>l', '<H', 'i6'  ), Pack( '>l<H', 'i6' )
   print( p1, p2, p3 )
   -- t.Pack.Sequence[3]: 0x55c95e8af958      t.Pack.Sequence[3]: 0x55c95e8aff38      t.Pack.Sequence[2]: 0x55c95e8b2e18
   print( p1[1], p2[1] )
   -- t.Pack.Field[0](Int8B): 0x5641245a24b8           t.Pack.Field[0](Int8B): 0x5641245a3848
   print( p3[1], p3[2] )        -- 1. Sequence 2. Atomic
   -- t.Pack.Field[0](Sequence[2]): 0x55c95e8b6748    t.Pack.Field[10](UInt2L): 0x55c95e8b6788
   print( p3[1][1], p3[1][2] )  -- within the Sequence are two Atomic
   -- t.Pack.Field[0](Int8sb): 0x55c95e8b6d88 t.Pack.Field[8](Int2ul): 0x55c95e8b6e08

array
  The constructor takes a format string which defines a packer (atomic OR
  combinator) and a number defining how often it gets repeated.
  eg. p = ``Pack( '>d<H', 4 )`` defines a sequence of 2 elements which is
  10 bytes long, it will get repeated 4 times, making the packer cover 40
  bytes::

   - p[1]:    is a packer sequence
   - p[2][1]: is an atomic packer of type (float) with an 10 bytes offset

struct
  The constructor takes a format string which defines a composition of
  multiple items. eg. ``p = Pack( '>l<H' )`` defines a sequence of 2
  elements and is 10 bytes long on a 64 bit system::

   - p[1]: is an atomic packer of type (Int8sb) with a  0 bytes offset
   - p[2]: is an atomic packer of type (Int2sl) with an 8 bytes offset

reuse of packers
  Any previously defined packer can be used in place of a format string to
  create a new packer.  Consider the following code:

  .. code:: lua

   p1 = Pack( 'f>I4' ) -- sequence of packers
   -- formulate as struct
   p2 = Pack(
      { floatie = p[ 1 ] },
      { Int32   = p[ 2 ] }
   )


API
===

Class Members
-------------

``int bytes, int bits = Pack.size( t.Pack p )``
  Returns the size of the ``Pack p`` in bytes and in bits.  For bit type
  Packers the byte value is truncated to the next full byte value.  The
  function can be used on any of the combinators ``Pack.Sequence``,
  ``Pack.Array`` and ``Pack.Struct`` and returns the accumulated size.

``int bytes, int bits = Pack.offset( t.Pack.Field pf )``
  Returns the offset of the ``Pack.Field pf`` in bytes and in bits.  For bit
  type Packers the byte value is truncated to the next full byte value.  The
  function can be used on the combinators ``Pack.Field`` instances only and
  does not apply to atomic ``Pack`` types.


Class Metamembers
-----------------

``Pack p = Pack( value [, value, ...] )       [__call]``
  Creates ``Pack.* p`` from a single or multiple arguments. ``Pack()``
  reflects on the arguments to determine the type of Packer to be created.
  More details on the behaviourn can be found in the documentation for
  ``Pack.Struct``, ``Pack.Array`` and ``Pack.Sequence``.

``Pack p = Pack( string fmt )       [__call]``
  Creates ``Pack.* p`` from a format string.  The following format strings
  are allowed::

    - ``<``   : sets little endian
    - ``>``   : sets big endian
    - ``b``   : a signed byte (char)
    - ``B``   : an unsigned byte (char)
    - ``h``   : a signed short (native size)
    - ``H``   : an unsigned short (native size)
    - ``l``   : a signed long (native size)
    - ``L``   : an unsigned long (native size)
    - ``j``   : a lua_Integer
    - ``J``   : a lua_Unsigned
    - ``T``   : a size_t (native size)
    - ``i[n]``: a signed int with n bytes (default is native size)
    - ``I[n]``: an unsigned int with n bytes (default is native size)
    - ``f``   : a float (native size)
    - ``d``   : a double (native size)
    - ``n``   : a lua_Number
    - ``cn``  : a fixed-sized string with n bytes
    - ``r[n]``: signed Integer, n bits wide
    - ``R[n]``: unsigned Integer, n bits wide
    - ``v``   : single bit, intepreted as Lua boolean -> 0=False, 1= True

Instance Members
----------------

Atomic Packer instances have no access to internal members.  Combinators,
however, do.  Refer to their seperate documentation for details:

 - `Pack.Array <Pack.Array.rst>`__
 - `Pack.Sequence <Pack.Sequence.rst>`__
 - `Pack.Struct <Pack.Struct.rst>`__

Instance Metamembers
--------------------

``string s = tostring( Pack p )  [__tostring]``
  Returns a string representing the ``Pack p`` instance.  The string
  contains type, length and memory address information such as
  *`t.Pack.UInt5B: 0xdac2e8`*, meaning it is an unsigned integer which is 5
  bytes long and has Big Endian byte order.
