lua-t Table - Table helper and convienience fucntions
+++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

``Table`` is an extension to help with table handling, it does not extend
Luas builtin table library but instead has its own namesapce in ``t.Table``.
The functions are designed to help with some extended more complex table
operations in a convienient way.


API
===

Class Members
-------------

Being a helper library there are only static library functions available
since ``t.Table`` aims to no tinterfere with Luas native table namespace.


``table x = Table.map( table t, function f )``
  Returns a new table ``x``, composed of values from  Lua table ``t`` with
  all values processed via function ``f``.  The function ``f`` takes two
  arguments, the iteration value and the iteration key like below.  Because
  of Luas behaviour to remove items from a table when they get ``nil``
  assigned as value ``Table.map()`` can be used like a ``Table.filter()``
  function.

  .. code:: lua

    a = {1,2,3,4,5,6,7,8,9,10,11,12,13,14}

    -- create a table with squared even values
    b = Table.map( a, function(value, key)
      if 0 == value%2 then
        return value * value
      else
        print("removing key: `" .. key .. "`")
        return nil
      end
    end )
    print(a==b)  -- false

``table t1 = Table.foreach( table t, function f )``
  Returns a new reference ``t1`` to Lua table ``t`` with all values
  processed via function ``f``.  The function ``f`` takes two arguments,
  the iteration value and the iteration key like below.  Because of Luas
  behaviour to remove items from a table when they get ``nil`` assigned as
  value ``Table.map()`` can also be used like a ``Table.filter()`` function.

  .. code:: lua

    a = {1,2,3,4,5,6,7,8,9,10,11,12,13,14}

    -- create a table with squared even values
    a1 = Table.foreach( a, function(value, key)
      if 0 == value%2 then
        return value * value
      else
        print("removing key: `" .. key .. "`")
        return nil
      end
    end )
    print(a==a1)  -- true, same reference

``table res = Table.merge( table a, table b, bool union )``
  Returns a new Lua table ``res``  with keys merged from table ``a`` and
  table ``b``.  Merging happens exclusively based on the existence of keys.
  If values for the same key in table ``a`` and table ``b`` differ then the
  value from table ``b`` will be the one copied to the merge result
  ``res``.  This must be considered when passing the parameters ``a`` and
  ``b``. Otherwise, the order of the two input tables has no effect on he
  result.  The merge strategy indicator ``union`` will determine if the
  result ``res`` will be a union or an intersection of the parameters ``a``
  and ``b``.  If ``false`` or left out (aka. ``nil``, the result table
  ``res`` will be the intersection between table ``a`` and table ``b``.  If
  ``true`` it will be a union.

``table res = Table.complement( table a, table b, bool symmetric )``
  Returns a new Lua table ``res``  with keys representing the complement
  from table ``a`` and table ``b``.  Creating the complement happens
  exclusively based on the existence of keys.  If values for the same key in
  table ``a`` and table ``b`` differ then the value from table ``b`` will be
  the one copied to the complement result ``res``.  This must be considered
  when passing the parameters ``a`` and ``b``.  Otherwise, the order of the
  two input tables has no effect on he result.  The complement strategy
  indicator ``symmetric`` will determine if the result ``res`` will be a
  complement or a symmetric difference of the parameters ``a`` and ``b``.
  If ``false`` or left out (aka. ``nil``, the result table
  ``res`` will be the standard complement between table ``a`` and table
  ``b``.  If ``true`` it will be symmetric difference.

``bool res = Table.contains( table a, table b, bool disjunct )``
  Returns a boolean ``res`` indicating wether table ``b`` is contained in
  table ``a``.  This determination is based exclusively based on the
  existence of keys only in both tables.  The ``disjunct`` parameter will
  change the testing to indicate if both tables are completely disjunct
  instead.


Class Metamembers
-----------------

None.


Instance Members
----------------

None.

Instance Metamembers
--------------------

None.
