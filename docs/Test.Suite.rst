lua-t T.Test - The Unit Test Library
++++++++++++++++++++++++++++++++++++


Overview
========

``Test.Suite`` provides functionality to create and run unit test suites.


Summary
=======

 - passing a table of functions to the ``Test.Suite( )`` constructor will
   result in tests running in random order.
 - an actual test case within a suite can accept the Suite as a ``self``
   argument.
 - the table members should all be functions, each of them representing a
   test case.  If static extra elements would be needed, they must be setup
   inside the ``beforeAll()`` function.


Usage
=====

For an example of how to run test suits look at ``test/runner.lua``

Test runner
-----------

``Test.Suite`` does not come with a console based runner executable.  This
is by design because the actual behaviour of a ``Test.Suite`` is to simply
execute a table of test methods and then return a collection of results.

In order to create a test runner that can handle multiple test suites a
small Lua script is needed that wraps the test suites.  This way it can be
easily catered towards the application.  In a very simple scenario this
would look like the following.  Firstly, the several unit test suites will
get defined as modules like this one:

.. code:: lua

  -- suite1.lua
  Test = require't'.Test
  return {
    test1 = function(self)
      Test.describe("first test")
      ...
    end,
    test2 = function(self)
      Test.describe("second test")
      ...
    end,
    ...
  }

A test runner would require all the test suites as modules, potentially
assisted by some directory reader which ``requires()`` the contents of a
directory recursively.  Then the runner would iterate over all required
tests and execute them:

.. code:: lua

  -- test_runner.lua
  local suites = { 'suite1', 'suite2', 'suite3', 'suite4', ... }
  for _,suite in ipairs( suites) do
     local result, time, failed = Suite( require( suite ) )
  end


Test Execution Order
--------------------

``Test.Suite`` can execute test cases in a guaranteed order or, in true unit
testing fashion, in random order.  By default, the excution will be ordered
by running ``pairs(tst)`` over the test cases, which is random everytime a
new table is build.

The ``Test.Suite`` result itself is a collection of ``Test`` results and
since it is implemented as a ``t.OrderedHashTable`` the results are stored
in order of execution.


Hooks
-----

``Test.Suite`` provides some hooks which will influence test execution.
Each of the hooks is optional:

``suite.beforeAll = function( self, done )``
  The hook gets called before executing any test case in the suite.  The
  ``beforeAll`` hook is especially useful if a Test suite depends on the
  existence of a remote server or similar things when a connection needs to
  be setup before executing all tests.  Any member of the table passed to
  ``Test.Suite`` needed inside the tests must be created inside the
  ``beforeAll`` function like this:

.. code:: lua

  tbl = {
     beforeAll = function( self )
        self.testValueGenerator = require('TestValueGenerator')
     end,
     StringForLength = function( self )
        Test.describe("Test string for proper length")
        local str = self.testValueGenerator:getString(500)
        assert( #str == 500, ("String should be 500 characters long but was %d"):format( #str ) )
     end
   }
   s = Test.Suite(tbl)

``suite.afterAll = function( self )``
  The hook gets called after all tests in the suite got executed.

``suite.beforeEach = function( self )``
  The hook gets called before each tests case in the suite got executed.

``suite.afterEach = function( self )``
  The hook gets called after each tests case in the suite got executed.

API
===

Class Members
-------------

None.

Class Metamembers
-----------------

``Test.Suite suite, int milliseconds, table failed = Test.Suite( table tests[, boolean sort, boolean quiet] )   [__call]``
  Creates a new ``Test.Suite`` suite instance.  It returns a collection of
  ``Test`` results in order of execution. ``int milliseconds`` is the
  runtime of the entire ``Test.Suite`` including all hooks.  Each test case
  in the suite which failed execution will be collected in the ``table
  failed``.  If all tests succeded that value will be ``nil``.
  If ``boolean sort`` is passed as true,  the tests in the ``table tests``
  will be sorted by their name. The default value is ``false``.
  If ``boolean quiet`` is passed, the execution will not print status and
  progress to the terminal. The default value is ``false``.


Instance Members
----------------

``Test.Suite`` instances do not have any special instance members. Instead
one can access each ``Test`` case result under its name as it was defined in
the original table of test functions or by the numeric index of its
execution order.


Instance Metamembers
--------------------

``string s = tostring( Test t )  [__toString]``
  Returns a string which is a TAP report of the ``Test.Suite`` instance.

``int len = #testInstance  [__len]``
  Returns the number of ``Test`` result instances in this suite.

