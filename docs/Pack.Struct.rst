lua-t Pack.Struct - Named Packer Fields Structure
+++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

A ``Pack.Struct`` contains ``Pack.Field`` elements or other ``Pack.*``
combinators and allows to access them by a key.  It uses the very same
internal design as ``OrderedHashTable``.  This way it maintains the
necessary order of ``Pack.Field`` elements.  This allows to access the
fields by index as well.

API
===


Class Metamembers (related to Pack.Struct)
------------------------------------------

``Pack.Struct p = Pack({key1=pck1}, {key2=pck2})       [__call]``
  Creates ``Pack.Struct p`` from multiple arguments to ``Pack()`` where each
  argument is Lua table with eaxactly one key/value pair.  ``Pack()``
  reflects on the type of arguments and creates a ``Pack.Struct``` if the
  arguments passed are tables.

  .. code:: lua

    s = Pack(
      { length       = 'I2' },
      { ['type']     = 'I2' },
      { ['@status']  = 'B' },
      { ConsistCount = 'B' }
    )


Instance Members
----------------

A ``Pack.Struct`` has no instance members because access to the values is
controlled via table syntax.  As such the following behaviour has the
following effects:

``Pack.Field pf = Pack.Struct p[ value key ]``
  Returns ``Pack p`` wrapped in ``Pack.Field pf`` which contains the offset
  information of p within the ``Pack.Struct p``.

``Pack.Field pf = Pack.Struct p[ integer idx ]``
  Returns ``Pack p`` wrapped in ``Pack.Field pf`` which contains the offset
  information of p within the ``Pack.Struct p``.


Instance Metamembers
--------------------

``int i = #( Pack.Struct p )  [__len]``
  Return an integer with how many elements ``Pack.Struct p`` contains.

``string s = tostring( Pack.Struct p )  [__tostring]``
  Returns a string representing the ``Pack.Struct p`` instance.  The String
  contains type, length and memory address information such as
  *`t.Pack.Struct[11]: 0xdac2e8`*, meaning it has 11 elements.

``function f, table t, val key = pairs( Pack.Struct p)  [__pairs]``
  Iterator for ``Pack.Struct p``.  It will iterate over the table in proper
  order returning ``value key, Pack.Field pf`` for each iteration.

``function f, table t, val key = ipairs( Pack.Struct p)  [__ipairs]``
  Iterator for ``Pack.Struct p``.  It will iterate over the table in proper
  order returning ``integer idx, Pack.Field pf`` for each iteration.
