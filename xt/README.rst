
lua-xt => An extension of the Lua Language
======================================================

Overview
++++++++

What is it
----------

lua-xt is a library which extends Lua by various functionality.  I aims to be
very object oriented and in itself coherent using the same pattern of OOP in
all places offering a usable API.  It follows the Lua guidelines for OOP.
lua-xt is less of a system library than it is an extension which get's compiled
into a static Lua executable for a self contained application.  It covers areas
which are already served by other libraries such as lua-socket.



Contents (High level overview)
-----------------------------

 - Networking (xt.Socket)  --> TCP,UDP etc
 - Buffers (xt.Buffer)     --> a buffer of defined length with mutable values
 - Packers (xt.Pack)       --> a Packer of type and size
 - Struct (xt.Pack.Struct) --> a Combination of Packers or Structs
 - Array (xt.Pack.Struct)  --> a Sequence of Packers or Structs of same type
 - Encoding/Decoding/Encryption (xt.Encoding) --> CRC, Base64, ARC4, SHA1 ...
 - Unit Tests (xt.Test)    --> comprehensive tests with unified output
 - Asynchronous (xt.Loop)  --> event/select based asynchronous workings



Coding Standards
+++++++++++++++

OOP interface
-------------

luax-xt uses a unified OOP type according to Luas own recomendation for objects.
Class.new() or Class() create new objects.  lua-xt is so far exclusively written
in C.  This provides an overview of naming conventions and will help to navigate
the source code a lot faster.


C-CLass
-------

Prefixes:

 - xt               --> part of the xt library
 - xt_abc           --> part of the xt_xxx class
 - lxt_abc          --> exposed to the Lua API

xt_abc... functions would be usually exposed to xt_abc.h and such available
from other code that can interact with xt_abc type variables.  Most lxt_abc...
functions will remain static.


Exposure Level:

 - lxt_abc_Func     --> xt.Abc.Func()  static method on xt.Abc
 - lxt_abc__Func   --> metatable function of xt.Abc such as xt.Abc() (__call method)
 - lxt_abc_func     --> method of xt.Abc instance such as myAbc:func() 
 - lxt_abc__func   --> metatable method of xt.Abc instance (#myAbc length)




Example for a fictional abc class the way they are organized in lua-xt:

abc = xt_abc_create_ud( L, sz):
   create a userdata of type xt.Abc with size and push onto stack, shall return
   a pointer to abc
abc = xt_abc_check_ud( L, pos, check):
   check element on stack pos for to be a userdata of type abc (xt.Abc).  If
   check==1 hard fail otherwise fail soft and return NULL pointer.  If
   successful return pointer to abc.

lxt_abc__Call( luaVM )
   xt.Abc( p1, p2 ) styled constructor
lxt_abc_New( luaVM )
   xt.Abc.new( p1, p2 ) styled constructor
lxt_abc_Flav1( luaVM )
   xt.Abc.Flav1( p2 ) styled constructor
lxt_abc_DoIt( luaVM )
   xt.Abc.Doit( p2 ) static method

lxt_abc_read( luaVM )
   abc:read( p ) instance method

lxt_abc_write( luaVM )
   abc:read( p, 2 ) instance method


lxt_abc__len( luaVM )
   return a meaning ful #abc value

lxt_abc__tostring( luaVM )
   returns string "x.Abc{p1:p2}: 0x123456"

lxt_abc__gc( luaVM )
   clean up when abc gets collected




Documentation:
++++++++++++++

xt.Pack
---------

A Packer is a simple Data Format definition containing size and type. The following types are available:

 - xt.Packer.Bit(x,y)  = creates a packer of type Bit with big Endianess and length x and offset y
 - xt.Packer.Int(x)    = creates a packer of type Int with native Endianess and length x
 - xt.Packer.IntL(x)   = creates a packer of type Int with little Endianess and length x
 - xt.Packer.IntB(x)   = creates a packer of type Int with Big Endianess and length x
 - xt.Packer.Float(x)  = creates a packer of type Float with length x
 - xt.Packer.String(x) = creates a packer of type String with length x


xt.Pack.Struct
--------------

An ordered and optionally named collection of xt.Pack and/or xt.Pack.Struct. ::

   s = xt.Pack.Struct(
      { length       = xt.Pack.Int( 2 ) },
      { ['type']     = xt.Pack.Int( 2 ) },
      { ['@status']  = xt.Pack.Int( 1 ) },
      { ConsistCount = xt.Pack.Int( 1 ) },
      xt.Pack.String(17)
   )

Available methods on x.Pack.Struct s are:

   - pairs( s ) => iterator,    returns  name, xt.Pack.Reader
                   returns values in order!
   - __index    => xt.Pack.Reader, returns a type and position information
   - #struct    => length,      returns number of elements in struct
                   for i=1:#struct do allows numbered iteration (Lua 5.3 style)
   - tostring   => object name,
                   print(s) returns "xt.Pack.Struct(len,sz}: address
   - xt.Pack.size(s) => returns size of s in bytes


xt.Pack.Array
--------------

An ordered collection of a single xt.Pack or xt.Pack.Struct of n elements. ::

   s = xt.Pack.Array( xt.Pack.Int( 2 ), 24 )


Available methods on x.Pack.Struct s are:

   - pairs( s ) => iterator,    returns  name, xt.Pack.Reader
                   returns values in order!
   - #struct    => length,      returns number of elements in struct
                   for i=1:#struct do allows numbered iteration (Lua 5.3 style)
   - tostring   => object name,
                   print(s) returns "xt.Pack.Struct(len,sz}: address
   - xt.Pack.size(s) => returns size of s in bytes



xt.Pack.Reader
--------------

An xt.Pack or xt.Pack.Struct or xt.Pack.Array element returned by __index
method.  Additionally to the type of the element it also contains information
about the offset in the returning context. ::

   a = xt.Pack.String( 2 )
   s = xt.Pack.Struct (
      { one       = a},
      { two       = a},
      { three     = a},
      { four      = a}
   )
   b = "ZZYYXXWW"
   for k,v in pairs(s) do
      print( k, xt.Pack.read( v, b ) )
   end
   one        ZZ
   two        YY
   three      XX
   four       WW


