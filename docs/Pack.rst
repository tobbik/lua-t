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


Pack Identification
-------------------

``Pack`` objects can identify themselves via a string.  The string is
composed of different elements which vary slightly from type to type.  The
general composition follows a simple schema:

   Type Length Modifiers

Type can be any of the following:

Int
  Integer type; can be up to 64 bit wide, arbitrary bit size is allowed.
  Can be signed or unsigned.  Types that align with byte borders can have
  endianess assigned to them otherwise they default to network byte order
  .aka big endian.  Examples:

   - ``Int32ul``  Little endian unsigned integer; 4 byte wide
   - ``Int15sb``  Big endian signed integer; 15 bit wide

Float
  Float type; can be 4 bytes or 8 bytes wide. aka Float or Double.  Floats
  are always signed.  Endianess can be assigned.  Examples:

   - ``Float32sl``  Little endian; 4 byte wide
   - ``Float64sb``  Big endian; 8 byte wide

Boolean
  Boolean type; No variations. 1 bit wide.  The difference between this and
  a 1 bit wide integer is the return value type when parsing.

Raw
  Raw or String type.  The only variation is the length.

   - ``Raw14``   Blob or string of 14 bytes
   - ``Raw512``  Blob or string of 512 bytes

Array
  Array of ``n`` ``t.Pack`` instances of the same type.

Sequence
  Sequence of multiple ``t.Pack`` instances.

Struct
  Ordered sequence of Key/Value pairs where the values are ``t.Pack``
  instances.

What kind of a packer is created is controlled by the constructor.  The
``Pack( )`` constructor takes the following paramters and creates the
following datatypes:

atomic
  The constructor takes a format string which defines a single atomic item.
  eg. ``p = Pack( '<I3' )`` defines a little endian unsigned integer of 3
  bytes width (Int3ul).

  An interesting aspect about atomic packers is that they are perfectly
  immutable and therefore don't have to be recreated.  The library performs
  some **lazy** caching in order to prevent the creation of unnecessary
  copies:

  .. code:: lua

   Pack = require't.Pack'
   print( Pack[ 'Int4ub' ] )
   -- nil
   p1, p2, p3 = Pack'R4', Pack'v', Pack'R4'
   print(p1,p2,p3)
   -- Note that p1 and p3 have the same address
   -- t.Pack.Int4ub: 0x55814d1601a8  t.Pack.Bool: 0x55814d1afeb8     t.Pack.Int4ub: 0x55814d1601a8
   print( Pack[ 'Int4ub' ] )
   -- t.Pack.Int4ub: 0x55814d1601a8

sequence
  The constructor takes a format string which defines a composition of
  multiple items.  eg. ``p = Pack( '<l', '>H', 'i6' )`` defines a sequence
  of 3 elements and is 16 bytes long on a 64 bit system::

   - p[1]: atomic packer of type (Int8sl) 0  bytes offset (1st element)
   - p[2]: atomic packer of type (Int2ub) 8  bytes offset (length of p[1])
   - p[3]: atomic packer of type (Int6sl) 10 bytes offset (length of p[1]+p[2])

  More details are in the `Pack.Sequence <Pack.Sequence.rst>`__
  documentation.

array
  The constructor takes a format string which defines a packer (atomic OR
  combinator) and a number defining how often it gets repeated.
  eg. p = ``Pack( '>d<H', 4 )`` defines a sequence of 2 elements which is
  10 bytes long, it will get repeated 4 times, making the packer cover 40
  bytes::

   - p[1]:    is a packer sequence
   - p[2][1]: is an atomic packer of type (float) with a 10 bytes offset

  More details are in the `Pack.Array <Pack.Array.rst>`__ documentation.

struct
  The constructor takes a format string which defines a composition of
  multiple items. eg. ``p = Pack( '>l<H' )`` defines a sequence of 2
  elements and is 10 bytes long on a 64 bit system::

   - p[1]: is an atomic packer of type (Int8sb) with a  0 bytes offset
   - p[2]: is an atomic packer of type (Int2sl) with an 8 bytes offset

  More details are in the `Pack.Struct <Pack.Struct.rst>`__ documentation.

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

``string type, string subType= Pack.type( t.Pack p )``
  Returns ``string type`` such as ``Int, Float, Array, ...`` and the subType
  of a packer instance.  The ``string subType`` is composed of the type,
  length and modifiers as explained in _`Pack Identification`.


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
