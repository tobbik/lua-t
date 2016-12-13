lua-t T.OrderedHashTable - Hash with preserved insertion order
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

T.OrderedHashTable is a wrapper around a Lua table, striving to keep many
table styled operations functional.  Elements in an OrderedHashTable keep
the order in which they have been inserted.  The functions pairs( ) and
ipairs( ) will iterate over the instance in the guaranteed order and instead
of returning two values will return three::

   for k,v,i in pairs( instance ) do
      print( string.format( "Order: %d -- Key: '%s' -- Value: %s",
             i, k, tostring( v ) )
   end

   for i,v,k in ipairs( instance ) do  -- Note: k and i swapped positions
      print( string.format( "Order: %d -- Key: '%s' -- Value: %s",
             i, k, tostring( v ) )
   end

Common Pitfalls:
----------------

Why is there no `T.OrderedHashTable( { A="foo", B="bar" } )` constructor?
  The table in that example is already a Lua table by the time it is passed
  into the constructor.  At this point it the ordering information is
  already lost.

Why is there no `t.add( element )`?
  OrderedHashTable uses table syntax for access.  A user might wanna create
  an OrderedHashTable where 'add' is a value `t.add = true`.  This would
  lead to conflicting accessors.

Calling `T.OrderedHashTable.getindex( instance, index )` seems slow?
  Above operation is not a lookup but an O(n) scan operation.  Since it is
  not used all that often it seems to be a fair tradeoff.


API
===

Class Members
-------------

table *t* = T.OrderedHashTable.toTable( *oht* )
  Returns a Lua table with all values ordered and keys discarded.

table *t* = T.OrderedHashTable.toHash( *oht* )
  Returns a Lua table with keys and values and order discarded.

T.OrderedHashTable.insert( *oht*, *idx*, *key*, *value* )
  Insert *key*/*value* pair into *oht* at defined index *idx*.  Every
  element in *oht* which' index is greater than *idx* gets pushed down in
  *oht* order.

integer *i* = T.OrderedHashTable.getIndex( *oht*, *key* )
  Returns the index of the element in *oht* which has the given *key*.  If
  that key does not exist return nil.


Class Metamembers
-----------------

T.OrderedHashTable *t* = T.OrderedHashTable( [T.OrderedHashTable *oht*] )   [__call]
  Instantiate a new T.OrderedHashTable object.  If an instace of
  T.OrderedHashTable is passed as argument, the constructor will create a
  copy of that instance.  If no argument is passed the created
  OrderedHashTable is empty.


Instance Members
----------------

A T.OrderedHashTable has no instance members because access to the table
values is controlled via table syntax.  As such the following behaviour has
the following effects:

T.OrderedHashTable *oht[ key ]* = *value*
  If an element with *key* already exists, the value will be overwritten and
  the index of the *key* as well as the length of *oht* remains unchanged.
  If the element does not exist, it will be appended to the last position of
  *oht* and the length of *oht* will be incremented by one.
  
T.OrderedHashTable *oht[ index ]* = *value*
  If the index is < #*oht* the value will be overwritten.  If the index is >
  #*oht* an error is thrown.

T.OrderedHashTable *oht[ key/idx ]* = *nil*
  Remove element from the OrderedHashTable.  As a result, all key/value
  pairs with an index > idx or T.OrderedHashTable.getIndex( key ) will be
  moved up in the order of *oht*.  If key/idx doesn't exists this operation
  has no effect.


Instance Metamembers
--------------------

int *i* = #( T.OrderedHashTable *oht* )  [__len]
  Return an integer with how many elements *oht* contains.

string *s* = tostring( T.OrderedHashTable *oht* )  [__tostring]
  Returns a string representing the T.OrderedHashTable instance.  The String
  contains type, length and memory address information such as
  "T.OrderedHashTable[11]: 0xdac2e8", meaning it has 11 elements.

boolean *x* = T.OrderedHashTable *oht1* == T.OrderedHashTable *oht2*  [__eq]
  Compares two T.OrderedHashTable instances for equality.  If oht1 contains
  the same elements as oht2 in the same order the instances are considered
  equal.

