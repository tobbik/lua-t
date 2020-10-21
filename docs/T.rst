lua-t T - Basic functions
+++++++++++++++++++++++++


Overview
========

``t`` provides couple of functions that are generally useful and they are used
internally throughout lua-t itself.


Usage
=====

These functions are convenience helpers and shall be used wherever
applicable.


API
===

Class Members
-------------

``boolean isEqual = t.equals( val a, val b )``
  Compares value a and b with the following precedence:

  - if they have the same reference
  - if they have the same scalar value
  - if they have an __eq meta-method and return that result
  - if they are tables and table length is equal, recursively compare them
    and their sub-tables

``table module = t.require( string modName )``
  This is an extension to the Lua standard ``require()`` with the added
  ability to find modules relative from the file where ``require()`` is
  called.  It tries to determine the location of the script that calls
  ``t.require()`` and appends temporarily it's path to ``package.path`` and
  ``package.cpath``.  This allows to ``t.require()`` load modules that are
  located relatively to the calling script.

``string typeName = t.type( value obj )``
  If ``obj`` has a meta-table attached and the meta-table has a ``__name``
  field ``t.type()`` will return that value, otherwise it will return the
  same as Lua built-in ``type()``.

.. code:: lua

  T,Socket,Oht=require't',require't.Net.Socket',require't.OrderedHashTable'
  f,s,o = io.input(), Socket(), Oht()
  T.type(f)    -- 'FILE*'
  type(f)      -- 'userdata'
  T.type(s)    -- 'T.Net.Socket'
  type(s)      -- 'userdata'
  T.type(o)    -- 'T.OrderedHashTable'
  type(o)      -- 'userdata'

``table prx = t.proxyTableIndex``
  The proxy-table index is a static value that is instantiated upon load
  time.  It is in fact a reference to an empty table.  It is used throughout
  lua-t as the index for any classes (such as ``t.OrderedHashTable`` or
  ``t.Set``) which utilize a proxy-table to control get and set
  functionality via ``__index`` or ``__newindex`` meta-methods.

``void = t.assert( condition, string message[, ...] )``
  ``t.assert()`` expands the normal ``assert()`` functionality by the
  ability to use ``string.format()`` style parameters to extend the message
  string.  This allows for more informative ``assert()`` messages which can
  print out information about the error context more conveniently.


Class Metamembers
-----------------

No meta members.


Instance Members
----------------

``t`` can't be instantiated and hence there are no instance members.

Instance Metamembers
--------------------

``t`` can't be instantiated and hence there are no instance members.
