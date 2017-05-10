lua-t t.Test.Context - The Unit Test Execution Context
++++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

``Test.Context`` provides functionality to retain a state for the
execution time of a unit ``Test``.  It controlls 2 things mainly:

 - Filtering of unit ``Test.Case``; which to include /exclude
 - Hooks to execute before and after each ``Test.Case`` or before and after
   the entire unit ``Test`` suite.


API
===

For most of it a user can rely on the ``Test.Context`` which is created
automatically when a ``Test`` suite gets executed.


Class Members
-------------

None.

Class Metamembers
-----------------

``Test.Context ctx = Test.Context( include, exclude, beforeEach, afterEach, beforeAll, afterAll )   [__call]``
  Creates a new ``Test.Context`` instance.  If a ``Test.Context`` is passed
  as first parameter it will be cloned.  Otherwise, the six paramters relate
  to the instance variables with the respective names described further
  down.  All values are optional and if ``nil`` is passed instead the
  respective default values are used.  The default values include all tests
  and give a mild reporting and runtime output.  For completely silent
  execution use this:

  .. code:: lua
    local nilfunc = function() end
    local ctx = Test.Context( nil, nil, nilfunc, nilfunc, nilfunc, nilfunc )

    local tst = Test( { test cases here } )
    local success = tst( ctx )  -- completely silent execution


Instance Members
----------------

``string ctx.include    -- default ''``
  This string controls which test cases get executed upon the execution of a
  ``Test`` suite.  The **default INCLUDES EVERY** name in the execution.  It
  is ``''`` aka. the empty string.  The ``string include`` is evaluated by
  Luas own ``string.match()`` function, hence all Lua pattern work out of
  the box.

``string ctx.exclude    -- default '^$'``
  This string controls which test cases get not executed upon the execution
  of a ``Test`` suite.  ``string exclude`` is only evaluated after ``string
  include``.  The **default does NOT EXCLUDE ANY** name from execution.  It
  is ``'^$'`` aka. the empty line.  Like ``string include`` Luas own
  ``string.match()`` also evaluates ``string exclude``, hence all Lua
  pattern work out of the box.

``boolean willExecute = ctx.match( Test.Context ctx, string name )``
  Tests if the values for ``ctx.include`` and ``ctx.exclude`` would cover
  the execution of a ``Test.Case`` with the ``string name``.

``void function ctx.beforeEach( Test.Context ctx, Test.Case case )``
  This hook gets executed just before each ``Test.Case`` in the unit
  ``Test`` suite.  By default it prints the name of the currently active
  ``Test.Case``.

``void function ctx.afterEach( Test.Context ctx, Test.Case case )``
  This hook gets executed just after each ``Test.Case`` in the unit
  ``Test`` suite.  By default it prints the ``Test.Case.description`` of the
  currently active ``Test.Case``.

``void function ctx.beforeAll( Test.Context ctx, Test suite )``
  This hook gets executed just before the entire ``Test`` suite gets
  executed.  By default it does nothing.

``void function ctx.afterAll( Test.Context ctx, Test suite )``
  This hook gets executed just after the entire ``Test`` suite gets
  executed.  By default it prints a report.  The number of *Handled Tests*
  is affected by `ctx.include` and `ctx.exclude`.

  .. code:: lua

    Handled 13 tests in 0.160 seconds
    
    Executed         : 13
    Skipped          : 2
    Expected to fail : 1
    Failed           : 1
    status           : OK

``string current_name``
  The name of the curtrently executed test.

``int name_width``
  The length(in characters) of the longest name in the test suite to be
  executed.  The longest name is the longest is affected by
  ``ctx.incklude``` and ``ctx.exclude``.  This allows to print aligned
  columns like so::

    test_first          : The first test in the Test suite
    test_second         : The second test in the Test suite
    test_third          : The third test in the Test suite
    test_withALongName  : Anothe test in the suite

``table metrics =  function ctx.getMetrics( Test.Context ctx, Test suite )``
  The function evaluates the execution of a ``Test`` suite and returns a
  table with the following fields:
  values back:

    - ``boolean success``: was the execution of ``suite`` in the context of
      ``ctx`` successful?  Skipped ``Test.Case`` instances do not affect the
      success variable.  ``Test.Case`` instances that are expected to fail
      (via the ``Test.Case.todo()`` method) must fail.  Otherwise success
      will be ``false``.
    - ``int count``: how many ``Test.Case`` instances got handled in the
      context ``ctx``.
    - ``int pass``: how many ``Test.Case`` instances passed execution in the
      context ``ctx``.
    - ``int skip``: how many ``Test.Case`` instances got skipped in the
      context ``ctx``.
    - ``int todo``: how many ``Test.Case`` instances were expected to fail
      in the context ``ctx``.
    - ``Time time``: how long did the execution of all ``Test.Case``
      instances in the ``Test.Context ctx`` take.  This value only
      accumulates ``Test.Case.executionTime`` which covers the execution
      time of the ``Test.Case`` only. It disregards time taken by the
      execution of hooks.


Instance Metamembers
--------------------

None.
