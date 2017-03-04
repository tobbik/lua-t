lua-t T.Set - The Set Object (Data structure)
+++++++++++++++++++++++++++++++++++++++++++++


Overview
========

T.Set is a wrapper around a Lua table, striving to keep many table styled
operations functional.  Elements in a Set are unordered and they can occur
only once.  It does allow for set operations; most of which are mapped to
metamethods and hence can be accessed with normal operators ( | , -, etc.)


Common Pitfalls:
----------------
T.Set has no instance members because the access to the elements is
controlled via Lua table syntax.  As such elements are added and removed the
way the would be added and removed to tables: ``s.element = true`` adds
``element`` to the set.  Likewise, ``s.element = nil`` removes ``element``
from the set.  Designing T.Set in this fashion makes for very speedy Set
operations because it can be done via hash key lookups.


API
===

Class Members
-------------

``table t = T.Set.getReference( s )``
  Returns the underlying Lua table used for the set implementation which
  holds values in the following fashion::

    table = {
      'firstSetElement'   = true,
      'secondSetElement'  = true,
      'thirdSetElement'   = true
    }

  Since the returned table is a reference, manipulating the table may have
  ill effects on the T.Set instance and result in erratic behaviour.  The
  function shall be used for debugging purposes only.

``table t = T.Set.getTable( s )``
  Returns an array table with all elements of ``s`` as values.  The
  returned table could be used to create a cloned T.Set. ``T.Set(
  T.Set.getTable( set ) )``


Class Metamembers
-----------------

``T.Set s = T.Set( [table t] )   [__call]``
  Instantiate a new T.Set object.  The constructor will iterate ( like
  pairs( ), including hash and numeric indexes) over all elements in ``t``
  and create ``s`` from it's values.  If values exist more than once only
  one is added to ``s``.  Keys have no effect on ``s``.  An empty table or
  no argument will result in an empty set ``s``.

``T.Set s1 = T.Set( [T.Set s] )   [__call]``
  Instantiate a new T.Set object.  The constructor will create a copy of
  the T.Set ``s``.  An empty T.Set or nothing as argument will result in an
  empty T.Set.


Instance Members
----------------

A T.Set has no instance members because the access to the table valuess is
controlled via table syntax.  As such the following behaviour has the
following effects:

``T.Set set[ element ] = true -- or anything else but nil``
  Add element to the set.  If it already exists this operation has no
  effect.  The value at the key ``element`` will be set to ``true``
  regardless of what value has been passed to the set initially.

``T.Set set[ element ] = nil  -- only nil, not false``
  Remove element from the set.  If it doesn't exists this operation has
  no effect.

``if set[ element ] then ...``
  This is basically the ``contains`` operation.  ``set.element`` will always
  return ``true`` if the element is part of ``set``.  Otherwise it will
  return ``nil`` which Lua evaluates as ``false``.


Instance Metamembers
--------------------

``int i = #( T.Set s )``  [__len]
  Return an integer with how many elements are in ``s``.

``string str = tostring( T.Set s )  [__tostring]``
  Returns a string representing the ``s`` instance.  The String contains
  type, length and memory address information such as
  ``T.Set[6]: 0x1193d18``, meaning ``s`` has 6 elements.

``boolean x = T.Set s1 == T.Set s2  [__eq]``
  Compares two sets for equality.  If set1 contains all the same elements as
  ``s2`` the sets are considered equal.

``boolean x = T.Set s1 <= T.Set s2  [__le]``
  Compares two sets for ``s1`` being a subset of ``s2``.  Returns 1 if all
  elements of ``s1`` exist in ``s2``.  Else returns 0.

``boolean x = T.Set s1 < T.Set s2  [__lt]``
  Compares two sets for ``set1`` being a proper subset of ``set2``.  Returns
  1 if all elements of ``s1`` exist in ``s2`` and ``#s2 ~= #s1``.  Else
  returns 0.

``boolean b = T.Set s1 % T.Set s2  [__mod]``
  Returns true if ``s1`` and ``s2`` have no elements in commmon.  Also known
  as being disjoint.  Testing for not being disjoint requires braces because
  the ``not`` operator has higher precidence than the ``%`` modulo
  operator::

    if not (s1 % s2) then print( "The sets are not disjoint" ) end

``T.Set s = T.Set s1 | T.Set s2  [__bor]``
  Creates and returns a new T.Set which is the union of ``s1`` nd ``s2``.
  [1,2,3,4,5] | [2,4,6,7] = [1,2,3,4,5,6,7]

``T.Set s = T.Set s1 - T.Set s2  [__sub]``
  Creates and returns a new T.Set which is the complement of ``s1`` and
  ``s2``, which is all elements of ``s1`` with all elements removed that
  occur also in ``s2``.  ``[1,2,3,4,5] - [2,4] = [1,3,5]``

``T.Set s = T.Set s1 & T.Set s2  [__band]``
  Creates and returns a new T.Set which is the intersection of ``s1`` and
  ``s2``.  ``[1,2,3,4,5] & [2,4,6,7] = [2,4]``

``T.Set s = T.Set s1 ~ T.Set s2  [__bxor]``
  Creates and returns a new T.Set which is the symetric difference of ``s1``
  and ``s2``.  ``[1,2,3,4,5] ~ [2,4,6,7] = [1,3,5,6,7]``

