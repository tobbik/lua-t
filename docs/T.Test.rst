lua-t T.Test - The Unit Test Library
++++++++++++++++++++++++++++++++++++


Overview
========

``Test`` provides functionality to create and run unit test suites.
``Test`` allows to write synchronous as well as asynchronous test (callback
based) and it allows them to be mixed within the same ``Test`` suite.


Summary
=======

 - passing a table of ``test_*`` named functions to the ``Test( )``
   constructor will result in tests running in random order.
 - an actual test case **must start with** ``test_`` as function name.
   Other names like ``whatEver`` are just gonna be available within the
   `Test`` as ``self.whatEver``.
 - when persent, the global test suite hooks ``beforeAll( self, done )`` and
   ``afterAll( self, done )`` **must call** the ``done( )`` callback even
   when all tests in the test suite are of synchronous nature.
 - running a ``Test`` suite and checking for success is as easy as executing
   it: ``success = t()``.


Usage
=====

Some general information on how to write and invoke Test suites.


Test runner
-----------

``Test`` does not come with a console based runner executable.  This is by
design because the actual behaviour of a ``Test`` instance is defined by
metamethods and once a ``Test`` suite is instantiated it gets executed by
just calling it.

In order to create a test runner that can handle multiple test suites a
small Lua script is needed that wraps the test suites.  This way it can be
easily catered towards the application.  In a very simple scenario this
would look like the following.  Firstly, the several unit test suites will
get defined as modules:

.. code:: lua

  -- test1.lua
  Test = require't'.Test
  return Test(
     { ... test definition ...}
  )

A test runner would require all the test suites as modules, potentially
assisted by some directory reader which ``requires()`` the contents of a
directory recursively.  Then the runner would iterate over all required
tests and execute them:

.. code:: lua

  -- test_runner.lua
  local t_names = { 'test1', 'test2', 'test3', 'test4', ... }
  for i=i,#t_names do
     local tst = require(t_names[i])
     if not tst( ) then
        print( "Test Suite:" .. t_names[i] .. ' failed!' )
        print( tst )
  end


Test Execution Order
--------------------

``Test`` can execute test cases in a guaranteed order or, in true unit
testing fashion, in random order.  The behaviour can be selected via the way
the ``Test( )`` constructor gets called.  For guaranteed order, create an
empty ``Test`` instance first and then assign test case functions to it.
When running the suite tests will get executed in the order they got
assigned to the ``Test`` instance:

.. code:: lua

  t = Test( )
  t.test_one   = function( self ) ... end
  t.test_two   = function( self ) ... end
  t.test_three = function( self ) ... end
  t( ) -- this will run the tests in the order specified

To achieve randomly ordered test suite `Test( tbl )` constructor will accept
a table that has all test functions defined as members.  Upon constructing
the `Test` instance all members of the table will be iterated via `pairs()`.
That order will be used as execution order for the `Test` instance.  That
means the order gets randomized at construction time and each consecutive
execution of the entire suite will happen in the same random order:

.. code:: lua

  t = Test( {
     test_one   = function( self ) ... end,
     test_two   = function( self ) ... end,
     test_three = function( self ) ... end
  } )
  t( ) -- this will run the tests in the order pairs() would iterate over
       -- the table


Constructing a Test Suite
-------------------------

To create a ``Test.Case`` in a ``Test`` suite a function must be assigned to
the case which name **must begin** with ``test_*``.  When such a function
with a proper name is passed to ``Test`` it will invoke the ``Test.Case``
constructor with the proper parameters.  If the function name starts with
``test_cb_*`` the constructed ``Test.Case`` will be able to execute
asynchronously because the test runner will pass in a ``done`` callback.
Any other value that gets assigned to the table will be simply an instance
variable that within the test is available by ``self.variable_name``.  It is
**not possible** to create numerically indexed ``Test`` suite elements
because the numeric part of the table is reserved to define the execution
order.


Hooks
-----

`Test` provides some hooks which will influence test execution.  Each of the
hooks is optional:

``t.beforeAll = function( self, done )``
  The hook gets called before executing any test case in the suite.  If this
  hook is present, note that the execution **requires** to be finished by
  calling the ``done( )`` callback.  The beforeAll hook is especially useful
  if a Test suite depends on the existence of a remote server or similar
  things when a connection needs to be setup before executing all tests.  If
  no elaborate logic is needed to be performed in the beforeAll hook it is
  simpler to just make the values part of the Test suite definition like
  this:

.. code:: lua

  tbl = {
     testValueGenerator = TestValueGenerator(),
     beforeEach = function( self )
        self.str = self.testValueGenerator:getString( 500 )
     end,
     test_StringForLength = function( self )
        assert( #self.str == 500, "String should be 500 long" )
     end
   }
   t = Test( tbl )
   t( )

``t.afterAll = function( self, done )``
  The hook gets called after all tests in the suite got executed.  If this
  hook is present, note that the execution **requires** to be finished by
  calling the ``done( )`` callback.

For other hooks (``beforeEach/afterEach``) that are ``Test.Case`` specific
refer to the separate ``Test.Case`` documentation.


Test Execution Filter
---------------------

Executing the ``Test`` suite can be limited by names of the test functions.
This allows to group tests or run only single test while the suite will
still execute all the hooks.

.. code:: lua

  t = Test( {
     beforeAll  = function( self, done ) ...; done() end,
     afterAll   = function( self, done ) ...; done() end,
     test_odd_one   = function( self ) ... end,
     test_even_two  = function( self ) ... end,
     test_odd_three = function( self ) ... end
     test_even_four = function( self ) ... end
  } )
  t( 'odd' ) -- this will run the global hooks an all functions that have
             -- 'odd' in their name


API
===

Class Members
-------------

``boolean hasPassed, int pass, int skip, int todo, int time = Test.hasPassed( Test t )``
  Allows to get metrics from an already ran Test suite.

  - hasPassed   Was the Test run successful (nil if any test hasn't run)
  - pass        Number of Test.Case instances ran successfully
  - skip        Number of Test.Case instances that were skipped
  - todo        Number of Test.Case instances that were expected to fail
  - time        Accumulated runtime of the entire test suite, in millisecs

Class Metamembers
-----------------

``Test tc = Test( [ table t ] )   [__call]``
  Creates a new ``Test`` suite instance.  If a table is passed it will be
  converted into a unit test.  The table can not contain **ANY** numeric
  keys.


Instance Members
----------------

``Test`` instances do not have any special instance members.  Any test that
gets passed to a ``Test`` instance as a ``test_*`` named test function gets
converted to a ``Test.Case`` instance.  They have their own documentation.


Instance Metamembers
--------------------

``boolean x = Test t( [string pattern] )  [__call]``
  Executes the `Test t` suite.  Returns true or false depending on weather
  the execution of the test suite was successful.  The boolean return only
  works for synchronous tests.  As soon as there is a single asynchronous
  test case in the ``Test t`` the return value is always ``true``.  If a
  ``string pattern`` is passed as first parameter only ``Test.Case``
  instances in fields which contain ``string pattern`` will be executed.
  ``string pattern`` is evaluated by Luas own ``string.match()`` function,
  hence all Lua patterns apply.

``string s = tostring( Test t )  [__toString]``
  Returns a string which is a TAP report of the Test suite.

``int len = #testInstance  [__len]``
  Returns the number of ``Test.Case`` instances in this suite.

