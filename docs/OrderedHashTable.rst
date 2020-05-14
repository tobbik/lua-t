lua-t OrderedHashTable - Hash with preserved insertion order
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

``OrderedHashTable`` is a wrapper around a Lua table, striving to keep many
table styled operations functional.  Elements in an ``OrderedHashTable``
keep the order in which they have been inserted.  The functions ``pairs( )``
and ``ipairs( )`` will iterate over the instance in the guaranteed order and
instead of returning two values will return three:

.. code:: lua

  for k,v,i in pairs( instance ) do
    print( string.format( "Order: %d -- Key: '%s' -- Value: %s",
           i, k, tostring( v ) )
  end

  -- Note: using `ipairs` make k and i swap positions
  for i,v,k in ipairs( instance ) do
    print( string.format( "Order: %d -- Key: '%s' -- Value: %s",
           i, k, tostring( v ) )
  end

Usage
=====

The order of values in the table is determined by order of insertion like
this:

.. code:: lua

  > oht = OrderedHashTable()
  > oht.foo = "I'm a value"
  > oht.bar = "I'm another value"
  > oht[1]
  I'm a value
  > oht[2]
  I'm another value

Common Pitfalls:
----------------

Why is there no ``OrderedHashTable( { A="foo", B="bar" } )`` constructor?
  The table in this case is already a Lua table by the time it is passed
  into the constructor.  At this point the ordering information is already
  lost.  There is, however, a constructor which is a little more verbose
  ``OrderedHashTable( { A="foo" }, { B="bar" } )`` which does preserve order.

Why is there no ``t.add( element )``?
  ``OrderedHashTable`` uses table syntax for access.  A user might wanna
  create an OrderedHashTable where 'add' is a value such as
  ``t.add = 'something'``.  This would lead to conflicting accessors.

Calling ``OrderedHashTable.getIndex( instance, key )`` can be slow for bigger tables?
  Above operation is not a hash table lookup but as a consequence of the
  implementation an O(n) scan operation.  Since it is not used all that often
  it seems to be a reasonable tradeoff.

Can I insert values at arbitrary an index of a ``OrderedHashTable`` instance?
  No, there cannot be "holes" in a an OrderedHashTable.  This is caused by
  Luas underlying table implementation where tables with missing indexes are
  actually not numerically indexed but implicitely converted to hash tables.


API
===

Class Members
-------------

``table t = OrderedHashTable.values( oht )``
  Returns a Lua table with all values in order. Keys are discarded.

``table t = OrderedHashTable.keys( oht )``
  Returns a Lua table with all keys in order. Values are discarded.

``table t = OrderedHashTable.table( oht )``
  Returns a Lua table with keys and values and order discarded.

``string s = OrderedHashTable.concat( oht, string sep )``
  Returns a string with all the values from the OrderedHashTable ``oht``
  separated by the string ``sep``.  Order in the string is preserved.

``void OrderedHashTable.insert( oht, idx, key, value )``
  Insert ``key``/``value`` pair into ``oht`` at defined index ``idx``.
  Every element in ``oht`` which' index is greater than ``idx`` gets pushed
  down in ``oht`` order.

``integer i = OrderedHashTable.index( oht, value key )``
  Returns the index of the element in ``oht`` which has the given ``key``.
  If that key does not exist return nil.  This operation is a bit more
  expensive then a lookup because it has to perform a table scan O(n)
  operation.

``value k = OrderedHashTable.key( oht, integer i )``
  Returns the key of the element in ``oht`` which has the given index ``i``.
  If that index does not exist return nil.  Unlike ``index()``, this
  operation is a lookup, not a scan.

``void OrderedHashTable.setElement( tbl, value key/int idx, value value)``
  For any ``Table t`` which adheres to the ``OrderedHashTable`` structure
  this function allows to set an element.  It performs all checks if the key
  is an integer and performs the expected operation in regards to
  overwriting an existing key or appending a new one.

``value val = OrderedHashTable.getElement( tbl, value key/int idx)``
  For any `Table t` which adheres to the ``OrderedHashTable`` structure this
  function returns an element.

``function f, table t, val x = OrderedHashTable.iters( tbl, boolean num )``
  For any ``Table t`` which adheres to the ``OrderedHashTable`` structure
  this function returns an iterable which can be used in a ``for k,v,i in
  iters( tbl )`` or ``for i,v,k in iters( tbl, true )`` loop.  If ``num`` is
  ``true`` the iterator will return the index on first and the key on third
  position. Otherwise these positions are reversed.


Class Metamembers
-----------------

``OrderedHashTable oht = OrderedHashTable( [OrderedHashTable oht] )   [__call]``
  Instantiate a new ``OrderedHashTable`` object.  If an instance of
  ``OrderedHashTable`` is passed as argument, the constructor will create a
  clone of that instance.  If no argument is passed the created
  ``OrderedHashTable`` is empty.

``OrderedHashTable t = OrderedHashTable( [{k1=v1}, {k2=v2}, …]  )   [__call]``
  Instantiate a new ``OrderedHashTable`` object.  If tables with single
  pairs of key/value are passed the will be inserted as key/value pairs into
  the new instance, preserving the order of the arguments.


Instance Members
----------------

A ``OrderedHashTable`` has no instance members because access to the table
values is controlled via table syntax.  As such the following behaviour has
the following effects:

``OrderedHashTable oht[ key ] = value``
  If an element with ``key`` already exists, the value will be overwritten
  and the index of the ``key`` as well as the length of ``oht`` remains
  unchanged.  If the element does not exist, it will be appended to the last
  position of ``oht`` and the length ``#oht`` will be incremented by one.

``OrderedHashTable oht[ index ] = value``
  If the index is `<#oht` the value will be overwritten.  If the index is
  `>#oht` an error is thrown.

``OrderedHashTable oht[ key/idx ] = nil``
  Remove element from the OrderedHashTable.  As a result, all key/value
  pairs with an index > idx or OrderedHashTable.getIndex( key ) will be
  moved up in the order of ``oht``.  If key/idx doesn't exists this
  operation has no effect.

``table t = OrderedHashTable oht[ t.proxyTableIndex ]``
  Returns the underlying Lua table with keys and values which holds values
  in the following fashion:

  .. code:: lua

    t = {
      1           = 'firstKey',
      2           = 'secondKey',
      'firstKey'  = 'first value',
      'secondKey' = 'second value'
    }

  Since the returned table is a reference, manipulating the table may have
  ill effects on the ``OrderedHashTable`` instance and result in erratic
  behaviour.  The function is provided for debugging purposes only.

Instance Metamembers
--------------------

``int i = #( OrderedHashTable oht )  [__len]``
  Return an integer with how many elements `oht` contains.

``string s = tostring( OrderedHashTable oht )  [__tostring]``
  Returns a string representing the ``OrderedHashTable`` instance.  The
  string contains type, length and memory address information such as
  *`t.OrderedHashTable[11]: 0xdac2e8`*, meaning it has 11 elements.

``boolean x = (OrderedHashTable oht1 == OrderedHashTable oht2)  [__eq]``
  Compares two OrderedHashTable instances for equality.  If ``oht1``
  contains the same elements as ``oht2`` in the same order the instances are
  considered equal.

``function f, table t, val key = pairs( OrderedHashTable oht)  [__pairs]``
  Iterator for ``OrderedHashTable oht``.  It will iterate over the table in
  proper order returning ``value key, value val, int index`` for each
  iteration.

``function f, table t, val key = ipairs( OrderedHashTable oht)  [__ipairs]``
  Iterator for ``OrderedHashTable oht``.  It will iterate over the table in
  proper order returning ``int index, value val, value key`` for each
  iteration.
