lua-t T.Pack - The Binary Packer
++++++++++++++++++++++++++++++++


Overview
========

A Bit and Byte packer using the same kind of formatting string as Lua 5.3
string.pack( ) and string.unpack( ) with two notable exceptions:

   1 it does not deal with alignment
   2 it can parse and serialize to Bit wide resolution

t.Pack preferably works on t.Buffer objects, because they are mutable.  This way
for each write the Lua interpreter does not have to create and internalize a new
string.

t.Pack format strings
=====================

For Bit Style packing lua-t introduces the following format strings

   - **v:** a boolean represented as a single bit.
   - **r:** a signed Integer up to native size.  It can span byte boundaries.
   - **R:** an unsigned Integer up to native size.  It can span byte boundaries.


t.Pack types
------------

t.Pack objects can come in multiple flavours.  There is a main separation
between atomic packers and packer collections.  The access to packer collections
follows the same syntax as Lua tables.  Items in packer collections can be
packer collections themselves (nesting).

atomic
  A single byteType or a single bitType packer which returns a scalar value
  such as a boolean, Int, float or string.

sequence
  Multiple values that are packed in order defined by the format string.

array
  A collection of same typed packers with a given length.

struct
  A collection of packers which have named fields.


t.Pack identification
---------------------

t.Pack objects can identify themselves via a string.  The String is composed of
different elements which vary slightly from type to type.  The general
composition follows a simple schema:

   Type Length Endianess

Type can be any of the following:

   - Int          (includes byte, short, long, LuaInteger)
   - UInt         (includes unsigned byte, short, long, LuaInteger)
   - Float        (includes double and LuaNumber)
   - Boolean      (includes double and LuaNumber)
   - BitSigned    Bitfield representing signed integer
   - BitUnsigned  Bitfield representing unsigned integer
   - Raw          string/utf8/binary
   - Array        Array Combinator
   - Sequence     Sequence Combinator
   - Struct       Struct Combinator

What kind of a packer is created is controlled by the constructor.  The t.Pack
constructor takes the following paramters and creates the following datatypes:

atomic
  The constructor takes a format string which defines a single atomic item.
  eg. p = t.Pack( '<I3' ) defines a little endian unsigned integer of 3 bytes
  width (UInt3L)

sequence
  The constructor takes a format strings which defines a composition of
  multiple items. eg. p = t.Pack( '>l<H' ) defines a sequence of 2 elements and
  is 10 bytes long on a 64 bit system.

   - p[1]: is an atomic packer of type (Int8B) with a  0 bytes offset
   - p[2]: is an atomic packer of type (int2L) with an 8 bytes offset

array
  The constructor takes a format strings which defines a packer (atomic or
  combinator) and a number defining how often it gets repeated. 
  eg. p = t.Pack( '>d<H', 4 ) defines a sequence of 2 elements which is
  10 bytes long, it will get repeated 4 times, making the packer cover 40 bytes.

   - p[1]:    is a packer sequence
   - p[2][1]: is an atomic packer of type (float) with an 10 bytes offset

struct
  The constructor takes a format strings which defines a composition of
  multiple items. eg. p = t.Pack( '>l<H' ) defines a sequence of 2 elements and
  is 10 bytes long on a 64 bit system.

   - p[1]: is an atomic packer of type (Int8B) with a  0 bytes offset
   - p[2]: is an atomic packer of type (int2L) with an 8 bytes offset

reuse of packers
  Any previously defined packer can be used in plcae of a format string to
  create a new packer.  Consider the following code::

   p1 = t.Pack( 'f>I4' ) -- sequence of packers
   -- formulate as struct
   p2 = t.Pack(
      { floatie = p[ 1 ] },
      { Int32   = p[ 2 ] }
   )

t.Pack.Struct
-------------

An ordered and named collection of t.Pack objects. ::

   s = t.Pack(
      { length       = 'I2' },
      { ['type']     = 'I2' },
      { ['@status']  = 'B' },
      { ConsistCount = 'B' }
   )

Available methods on t.Pack.Struct s are:

   - pairs( s ) => iterator,    returns  name, t.Pack.Reader
                   Unlike a normal pairs( table ) this function returns values
                   in order!
   - __index    => t.Pack.Reader, returns a type and position information
   - #struct    => length,      returns number of elements in struct
                   for i=1:#struct do allows numbered iteration (Lua 5.3 style)
   - tostring   => object name,
                   print(s) returns "t.Pack.Struct( len,sz }: address
   - t.Pack.size(s) => returns size of s in bytes


t.Pack.Array
------------

An ordered collection of "n" t.Pack objects. ::

   s = t.Pack( '<i2', 24 )


Available methods on t.Pack.Pack s are:

   - pairs( s )      => iterator
       returns  name, t.Pack.Reader values in order
   - #struct         => length
       returns number of elements in struct
       for i=1:#struct does allow numbered iteration (Lua 5.3 style)
   - tostring( )      => object name,
       print(s) returns "t.Pack.Struct(len,sz}: address
   - t.Pack.size( s ) => size
       returns size of s in bytes



t.Pack.Reader
-------------

A t.Pack or t.Pack.Struct or t.Pack.Array element returned by the packers __index
method.  Additionally to the type of the element it also contains information
about the offset in the returning context. ::

  a = t.Pack( 'c2' )     -- string 2 characters long
  s = t.Pack(
     { one       = a },
     { two       = a },
     { three     = a },
     { four      = a }
  )
  b = "ZZYYXXWW"
  for k,v in pairs( s ) do
     print( k, v, v( b ) )
  end
  one	T.Pack.Reader[0](Raw2): 0xfbc6e8	ZZ
  two	T.Pack.Reader[2](Raw2): 0xfbc6e8	YY
  three	T.Pack.Reader[4](Raw2): 0xfbc6e8	XX
  four	T.Pack.Reader[6](Raw2): 0xfbc6e8	WW


