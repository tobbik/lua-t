lua-t Set - The Set Object (Data structure)
+++++++++++++++++++++++++++++++++++++++++++++


Overview
========

Set is a wrapper around a Lua table, striving to keep many table styled
operations functional.  Elements in a Set are unordered and they can occur
only once.  It does allow for set operations; most of which are mapped to
metamethods and hence can be accessed with normal operators ( | , -, etc.)


Common Pitfalls:
----------------
``Set`` has no instance members because the access to the elements is
controlled via Lua table syntax.  As such elements are added and removed the
way the would be added and removed to tables: ``s.element = true`` adds
``element`` to the set.  Likewise, ``s.element = nil`` removes ``element``
from the set.  Designing ``Set`` in this fashion makes for very speedy Set
operations because it can be done via hash key lookups.


API
===

Class Members
-------------

``string str = Set.toString( Set s )``
  Returns a string showing all values within ``Set s`` like a table.  Useful
  for debugging only.

``table t = Set.values( Set s )``
  Returns an array table with all elements of ``Set s`` as values.  The
  returned table could be used to create a cloned Set. ``Set(
  Set.table( set ) )``


Class Metamembers
-----------------

``Set s = Set( [table t], boolean useKeys )   [__call]``
  Instantiate new ``Set s`` object.  The constructor will iterate ( like
  pairs( ), including hash and numeric indexes) over all elements in
  ``Table t`` and create ``Set s`` from it's values.  If values exist more
  than
  once only one is added to ``Set s``. If ``boolean useKeys`` is ``true``
  the set will be composed of the keys rather than the values.  Otherwise
  keys in ``table t`` have no effect on ``Set s``.  An empty table or no
  argument will result in an empty set ``Set s``.

``Set s1 = Set( [Set s] )   [__call]``
  Instantiate a ``Set s1`` object.  The constructor will create a copy of
  the ``Set s``.  An empty ``Set s`` or nothing as argument will result in
  an empty ``Set s1``.


Instance Members
----------------

A ``Set`` has no instance members because the access to the table valuess is
controlled via table syntax.  As such the following behaviour has the
following effects:

``Set set[ element ] = true -- or anything else but nil``
  Add element to the set.  If it already exists this operation has no
  effect.  The value at the key ``element`` will be set to ``true``
  regardless of what value has been passed to the set initially.

``Set set[ element ] = nil  -- only nil, not false``
  Remove element from the set.  If it doesn't exists this operation has
  no effect.

``if set[ element ] then ...``
  This is basically the ``contains`` operation.  ``set.element`` will always
  return ``true`` if the element is part of ``set``.  Otherwise it will
  return ``nil`` which Lua evaluates as ``false``.


Instance Metamembers
--------------------

``int i = #( Set s )``  [__len]
  Return an integer with how many elements are in ``Set s``.  Internally
  this an ``O^n`` scan operation.

``string str = tostring( Set s )  [__tostring]``
  Returns a string representing the ``Set s`` instance.  ``string str``
  contains type, length and memory address information such as
  ``t.Set[6]: 0x1193d18``, meaning ``Set s`` has 6 elements.

``boolean x = Set s1 == Set s2  [__eq]``
  Compares two sets for equality.  If ``Set s1`` contains all the same
  elements as ``Set s2`` the sets are considered equal.

``boolean x = Set s1 <= Set s2  [__le]``
  Compares two sets for ``Set s1`` being a subset of ``Set s2``.  Returns
  ``true`` if all elements of ``Set s1`` exist in ``Set s2``.  Else returns
  0.

``boolean x = Set s1 < Set s2  [__lt]``
  Compares two sets for ``Set s1`` being a proper subset of ``Set s2``.
  Returns ``true`` if all elements of ``Set s1`` exist in ``Set s2`` an ``s2
  ~= s1``.  Else returns ``false``.

``boolean b = Set s1 % Set s2  [__mod]``
  Returns true if ``Set s1`` and ``Set s2`` have no elements in commmon.
  Also known as being *disjoint*.  Testing for not being disjoint requires
  braces because the ``not`` operator has higher precidence than the ``%``
  modulo operator:

  .. code:: lua

    if not (s1 % s2) then print( "The sets are not disjoint" ) end

The standard ``Set`` operations are done via the proper binary operators
where applicable and by that resemble Pythons implementation.  Since binary
operators in Lua are new (as of 5.3) operators used in previous
implementations are supported for legacy reasons.

``Set s = Set s1 | Set s2  [__bor]  -- s1+s2 overloaded [__add]``
  Creates and returns new ``Set s`` which is the union of ``Set s1`` and
  ``Set s2``. Eg. ``[1,2,3,4,5] | [2,4,6,7] = [1,2,3,4,5,6,7]``

``Set s = Set s1 - Set s2  [__sub]``
  Creates and returns new ``Set s`` which is the complement of ``Set s1``
  and ``Set s2``, which is all elements of ``Set s1`` with all elements
  removed that occur also in ``Set s2``.  Eg. ``[1,2,3,4,5] - [2,4] =
  [1,3,5]``

``Set s = Set s1 & Set s2  [__band]  -- s1*s2 overloaded [__mul]``
  Creates and returns new ``Set s`` which is the intersection of ``Set s1``
  and ``Set s2``.  Eg. ``[1,2,3,4,5] & [2,4,6,7] = [2,4]``

``Set s = Set s1 ~ Set s2  [__bxor]  -- s1^s2 overloaded [__pow]``
  Creates and returns new ``Set s`` which is the symetric difference of
  ``Set s1`` and ``Set s2``.  Eg. ``[1,2,3,4,5] ~ [2,4,6,7] = [1,3,5,6,7]``

