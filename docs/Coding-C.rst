Coding Standards and Convention
+++++++++++++++++++++++++++++++

.. contents::

Code organization
=================

As mentioned in the Origin.rst, lua-t is organized by a rigid naming and
structure convention which allowed me to keep control over the code base
when it was under heavy development load.  Understanding and following these
conventions allows to read the code base faster and be more productive in
it.  This is true for bug fixing and for writing new functionality.  This
guide illustrates the conventions and makes them transparent and
understandable.  It is highly recommended to read the guide before studying
the code base itself.


Naming conventions
------------------

Code generally is all put into the 'src' directory and added to its
Makefile.  Each file should be prefixed with `t_`.  Each "Problem Space"
lua-t adressess is assigned a three character code.  Each subproblems is
assigned another three character code, allowing for a hierarchy which can be
serialized as a string.  For example, the Encoding class has 'enc' assigned
to it.  The code within Encoding to provide Base64 has 'b64' assigned to it.
From the Lua side the Base64 object is available via T.Encoding.Base64.
Within C the files are named after the tree character code like so:

  - t_enc.c
  - t_enc_b64.c
  - t_enc_crc.c

All functions belonging to a namespace are prefixed the same way.  Confining
it to 3 characters keeps it short, assigning exactly 3 character code to one
problem keeps it precise.


File organization
-----------------

Most classes are represented as Lua userdata in lua-t.  For most of the C
part of the code these are mainly structs.  As a result, for any userdata
definition regarding a namespace and it's subspaces all of the struct
definitions should go into the main header file for the namespace, in the
above example that would be t_enc.h.  This ensures that all data definition
within a namespace AND it's subspaces can interact with each other.


OOP interface and Meta-Methods
==============================

The majority of this guide will use the example namespace 'nry' which within
Lua then is known as 'Numarray'.  The Numarray example is explained in
detail by Roberto Ierusalimschy's book "Programming in Lua', also known as
PiL.  This guide assumes the Numarray module was loaded by::

  Numarray = require( "t.Numarray" )

Constructors
------------

All of lua-t uses the same OOP type according to `Lua-Users style guide`_
for classes with constructors.  A new instance of the Numarray class would
be instantiated like this(Python style)::

  myNry = Numarray( param )

In general cosntructors should offer 3 ways of creating objects:

#. Create an empty object: ``emptyArray = Numarray( )``
#. Create an object from paramters: ``newArray = Numarray( 1,2,3,4,5,6 )``
#. Create a cloned object: ``clonedArray = Numarray( newArray )``

Applicable metamethods
----------------------

In general, the use of metamethods is preferable over writing specific
functions.  Instead of ``myNry.getLength()`` the metamethod `__len` should be
used to allow for syntaxt like ``#myNry``.  Similar to that the toString
method and applicable operators such as `__eq` or `__add` should be
implemented if possible.


C File structure
================

Naming conventions
------------------

The prefixes for functions and data types signal the following conditions:

 - t_*         --> part of the `t\_` library
 - t_nry_*     --> part of the `t_nry\_` class
 - lt_nry_*    --> exposed to the Lua API (direct or metamethod)

t_nry_* functions would be usually exposed to t_nry.h and such available
from other code that can interact with t_nry type variables.  Most lt_nry_*
functions will remain static as they are accessed within the class only.


Exposure Level on Lua side
--------------------------

In general, a capitalized function name signals a function which is static
to the Lua class and a non capitalized name indicates a method on a class
instance.  A single underscore before the name indicates a direct call by
name, a double underscore signals that a metatable is involved.

lt_nry_Func
  static classmethod on T.Numarray (eg. T.Numarray.Something)

lt_nry__Func
  metatable function of T.Numarray such as T.Numarray( ) (__call method)

lt_nry_func
  method of t.Numarray instance such as myNry:func( )

lt_nry__func
  metatable method of T.Numarray instance (#myNry length)


Data type conventions
----------------------

``struct t_nry { ..., ..., ...  };``
   A struct defining the userdata generated by t.Numarray( ). Typically defined
   in a file called t_nry.h


Functional conventions
----------------------

There are some special functions each C file(class) *shall* povide.  They are
used for creating and testing Lua userdata.

``static int lt_nry__Call( L )``
  ``t.Numarray( p1, p2, [...] )`` styled constructor

``struct nry = t_nry_create_ud( L, sz )``
  Create a userdata of type t.Numarray with size sz and push onto stack,
  shall return a pointer to nry struct.  This function would be typically
  called from ``lt_nry_new( L )`` after it evaluated the parameters passed
  from Lua and used here to populate dthe struct nry.

``struct nry = t_nry_check_ud( L, pos, check )``
  Check element on stack pos for being a userdata of type nry (t.Numarray).
  If check==1 hard fail (lua_error) otherwise fail soft and return NULL
  pointer.  If successful return pointer to nry struct.

``static int lt_nry__gc( L )``
  If ``t_nry_create_ud( )`` would run some allocations or aquires a system
  require (such as a file or a socket) this method must exist for clean up
  resources when the Numarray instance gets garbage collected.

``LUA_API int luaopen_t_nry( L )``
  A function which is called from the src/t.c file which registers
  'Numarray' in the "t" namespace and makes "t.Numarray" and all it's
  functionality available.

There are some typical functions each C file(class) *should* provide.  They
are used for programmers convenience and the ability to use most of Lua's
capabilities:

``static int lt_nry__len( L )``
  return a meaningful #nry value

``static int lt_nry__tostring( L )``
  returns string "T.Numarray{length}: 0x123456".  It also returns the memory
  address as well

There are functions a C file(class) *can* provide.  They are usually used to
provide methods on class instances or static functions on the class itself:

``static int lt_nry_Something( L )``
  ``T.Numarray.Something( p1, p2 )`` a static classFunction

``lt_nry_reverse( L )``
  ``myNry:reverse( )`` instance method to reverse the array


Library conventions
-------------------

This convention describes how all the functionality is organized and hooked
up to Lua itself.  lua-t makes heavy use of Lua's internal way of doing it
but formalizes it somewhat.  It creates 3 struct luaL_Reg arrays which get
hooked up in the luaopen_t_nry( L ) function. ::

  // Numarray class metamethods library definition
  static const struct luaL_Reg t_nry_fm [] = {
  	{ "__call",        lt_nry__Call},
  	{ NULL,            NULL}
  };

  // Numarray class functions library definition
  static const struct luaL_Reg t_nry_cf [] = {
  	{ "new",       lt_nry_New },
  	{ NULL, NULL }
  };

  // Numarray object method library definition
  //Assigns Lua available names to C-functions on T.Numarray instances
  static const luaL_Reg t_nry_m [] = {
  	{ "__index",    lt_nry__index },
  	{ "__newindex", lt_nry__newindex },
  	{ "__len",      lt_nry__len },
  	{ "__tostring", lt_nry__tostring },
  	// normal methods -> __index has logic to figure out if an access to the
  	// array or the method library was desired
  	{ "reverse",    lt_nry_reverse },
  	{ NULL, NULL }
  };

  // creates Metatable with methods for objects
  // creates Metatable with functions for class and push on stack to put on "t."
  LUAMOD_API int luaopen_t_nry( lua_State *L )
  {
  	// T.Numarray stance metatable
  	luaL_newmetatable( L, "T.Numarray" );
  	luaL_setfuncs( L, t_nry_m, 0 );
  	lua_pop( L, 1 );        // remove metatable from stack

  	// T.Numarray class
  	luaL_newlib( L, t_nry_cf );
  	luaL_newlib( L, t_nry_fm );
  	lua_setmetatable( L, -2 );
  	return 1;
  }

.. _`Lua-Users style guide`: http://lua-users.org/wiki/LuaStyleGuide
