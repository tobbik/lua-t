lua-t T - Basic functions
+++++++++++++++++++++++++


Overview
========

`t` provides ouple of functions that are generally useful and they are used
internally throughout lua-t itself.


Usage
=====

These functions are convienience helpers and shall be used wherever
applicable.


API
===

Class Members
-------------

``boolean isEqual = t.equals( val a, val b )``
  Compares value a and b with the following precedence:

  - if they have the same reference
  - if they have the same scalar value
  - if they have an __eq metamethod and return that result
  - if they are tables and table length is equal, recursively compare them
    and their sub-tables

``table module = t.require( string modName )``
  This is an extension to the Lua standard `require()` with on eadded
  feature.  It tries to determine the location of the script that calls
  `t.require()` and appends temporarily it's path to `package.path` and
  `package.cpath`.  This allows to `t.require()` load modules that are
  located relatively to the calling script.

``string typeName = t.type( value obj )``
  If `obj` has a metatable attached and the metatable has a `__name` field
  `t.type()` will return that value, otherwise it will return the same as
  Lua built-in `type()`.

``table prx = t.proxyTableIndex``
  The proxytable index is a static value that is instatiated upon load time.
  It is in fact a reference to an empty table.  It is used throughout lua-t
  as the index for any classes (such as `t.OrderedHashTable` or `t.Set`)
  which utilize a proxytable to control get and set functionality via
  `__index` or `__newindex` metamethods.

Class Metamembers
-----------------

No meta members.


Instance Members
----------------

`t` can't be instanciated and hence there are no instance members.

Instance Metamembers
--------------------

`t` can't be instanciated and hence there are no instance members.
