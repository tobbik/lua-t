Internal Data Structure explaination
++++++++++++++++++++++++++++++++++++


Data within ``T.Pack`` is structured to keep as much information as possible
in a small data structure.  The criteria is similar to a relational DB:

 - cannot be redundent( must be normalized )
 - must be space efficient


Atomic types are really straight forward
========================================

The data structure here is a single struct of type ``t_pck`` which consist
of:

 - Type: one of Bool, Int, Float or Raw
 - Size: how many bits in this packer
 - Modifier: special flag (Endianess, signed/unsinged,...) encoded on the
   bit level

Atomic types have no references to other fields and none of their properties
are computed from adjacent information, hence the name *atomic*.


Dynamic types have a very specific signiture
============================================

Dynamic types provide the ability to specify any of their properties
dynamically.  For that reason they are implemented as Lua functions.  Their
``t_pck`` struct is defined by:

 - Type: (Function aka. tpck_t T_PCK_FNC)
 - Size: must be set to 0
 - Modifier: is the LUA_REGISTRY reference for the function itself

The following function shows how to model a Pascal string using a dynamic
packer.  A Pascal string is defined by the first byt defining the length of
the string.  The first argument for a dynamic packer function takes is the
buffer itself.

.. code:: lua

 function( buf )
   local lPack = Pack( 'B' )
   local len   = lPack( buf )
   local rPack = Pack( 'Bc'..len )
   local str   = rPack[2]( buf )
   return str, len  -- always return size as second return value
 end


Complex types reference other Packer definitions
================================================

Complex types come in three flavours, each of them using different fields
within the ``t_pck`` struct.


Array type Packers
------------------

The data structure for an Array consists of the following elements:

 - Type: (Array aka. t_pck_t T_PCK_ARR)
 - Size: How many elements in the Array
 - Modifier: LUA_REGISTRYINDEX for T.Pack userdata. This can be a straight
   ``t_pck`` or a ``t_pck_idx`` struct.

The Pack.__index metamethod will create a new t_pck_idx userdata on the fly
when an element is accessed.


Sequence type Packers
---------------------

The data structure for a Sequence consists of the following elements:

 - Type: (Sequence aka. t_pck_t T_PCK_SEQ)
 - Size: How many elements in the Sequence
 - Modifier: LUA_REGISTRYINDEX for a Lua table.  The elements of the table
   are the actual T.Pack or T.Pack.Index instances

The Pack.__index metamethod will create a new t_pck_idx userdata on the fly
when an element is accessed.
