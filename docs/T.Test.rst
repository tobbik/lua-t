lua-t T.Test - The Unit Test Library
++++++++++++++++++++++++++++++++++++


Overview
========

T.Test provides functionality to create and run unit test suites.  It also
has a deep table(object) comparing functions.  T.Test allows to write
synchronous and asynchronous test (callback based) and it allows them to be
mixed within the same T.Test suite.


Summary
=======

 - passing a table to the ``T.Test( )`` constructor will result in tests
   running in random order
 - an actual test case **must start with** ``test`` as function name.
 - the global test suite hooks ``beforeAll( self, done )`` and
   ``afterAll( self, done )`` must call the ``done( )`` callback even when
   executed synchronously
 - running a T.Test suite and checking for success is as easy as executing
   it: ``success = t()``


Usage
=====

Some general information on how to write and invoke T.Test suites.


Test runner
-----------

T.Test does not come with a console based runner executable.  This is by
design because the actual behaviour of a T.Test instance is defined by
metamethods and once a T.Test suite is instantiated it gets executed by just
calling it.

In order to create a test runner that can handle multiple test suites a
small Lua script is needed that wraps the test suites.  This way it can be
easily catered towards the application.  In a very simple scenario this
would look like the following.  Firstly, the several unit tests will get
defined as modules::

   test1.lua
   Test = require't'.Test
   return Test(
      { ... test definition ...}
   )

A test runner would require all the test suites as modules, potentially
assisted by some directory reader which ``requires()`` the contents of a
directory recursively.  Then the runner would iterate over all required
tests and execute them::

   test_runner.lua
   local t_names = { 'test1', 'test2', 'test3', 'test4', ... }
   for i=i,#t_names do
      local tst = require(t_names[i])
      tst( )
   end


Test Execution Order
--------------------

T.Test can execute test cases in a guaranteed order or, in true unit testing
spirit, in a random order.  The behaviour can be chosen via the way the
``T.Test()`` constructor gets called.  For guaranteed order, create an empty
T.Test instance first and then assign test case functions to it.  When
running the suite tests will get executed in the order they got assigned to
the T.est instance::

   t = T.Test( )
   t.test_one   = function( self ) ... end
   t.test_two   = function( self ) ... end
   t.test_three = function( self ) ... end
   t( ) -- this will run the tests in the order specified

To achieve randomly ordered test execution the ``T.Test()`` constructor will
accept a table that has all test functions defined as members.  Upon
constructing the T.Test instance all members of the table will be iterated
over like ``pairs()``.  That order will be used as execution order for the
T.Test instance.  That means the order gets randomized at construction time
and each consecutive execution of the entire suite will happen in the same
random order::

   t = T.Test( {
      test_one   = function( self ) ... end,
      test_two   = function( self ) ... end,
      test_three = function( self ) ... end
   } )
   t( ) -- this will run the tests in the order pairs() would iterate over
        -- the table


Constructing a T.Test Suite
---------------------------

To create a T.Test.Case in a T.Test suite a function must be assigned to the
suite which name **must begin** with ``test_*``.  When such a function with a
proper name is passed to T.Test it will invoke the T.Test.Case constructor
with the proper parameters.  If the function name starts with ``test_cb_*``
the constructed T.Test.Case will be able to execute asynchronously because
the test runner will pass in a ``done`` callback.  Any other value that gets
assigned to the table will be simply an instance variable that within the
test is available by ``self.variable_name``.  It is **not possible** to
create numerically indexed T.Test suite elements because the numeric part of
the table is reserved to define the execution order.


Hooks
-----

T.Test provides some hooks which will influence test execution.  Each of the
hooks is optional:

``t.beforeAll = function( self, done )``
  The hook gets called before executing any test case in the suite.  If this
  hook is present, note that the execution **requires** to be finished by
  calling the ``done( )`` callback.  The beforeAll hook is especially useful
  if a Test suite depends on the existence of a remote server or similar
  things when a connection needs to be setup before executing all tests.  If
  no elaborate logic is needed to be performed in the beforeAll hook it is
  simpler to just make the values part of the T.Test suite definition like
  this::

  tbl = {
     testValueGenerator = TestValueGenerator(),
     beforeEach = function( self )
        self.str = self.testValueGenerator:getString( 500 )
     end,
     test_StringForLength = function( self )
        assert( #self.str == 500, "String should be 500 long" )
     end
   }
   t = T.Test( tbl )
   t( )

``t.afterAll = function( self, done )``
  The hook gets called after all tests in the suite got executed.  If this
  hook is present, note that the execution **requires** to be finished by
  calling the ``done( )`` callback.

Any hooks for T.Test.Case are described in the T.Test.Case documentation.


API
===

Class Members
-------------

``boolean isEqual = T.Test.equal( val a, val b )``
  Compares value a and b with the following precedence:

  - if they have the same reference
  - if they have the same scalar value
  - if they have an __eq metamethod and return that result
  - if they are tables and table length is equal, recursively compare them
    and their sub-tables

``boolean hasPassed, int pass, int skip, int todo, int time = T.Test.hasPassed( T.Test t )``
  Allows to get metrics from an already ran T.Test suite.  

  - hasPassed   Was the Test run successful
  - pass        Number of Test.Case instances ran successfully
  - skip        Number of Test.Case instances that were skipped
  - todo        Number of Test.Case instances that were expected to fail
  - time        Accumulated runtime of the entire test suite, in millisecs

Class Metamembers
-----------------

``T.Test tc = T.Test( [ table t ] )   [__call]``
  Creates a new Unit Test Suite.  If a table is passed it will be converted
  into a unit test.  The table can not contain **ANY** numeric keys.


Instance Members
----------------

T.Test instances do not have any special instance members.  Any test that
gets passed to t as a "test_" named test function gets converted to a
T.Test.Case instance.


Instance Metamembers
--------------------

``boolean x = T.Test t( )  [__call]``
  Executes the test suite.  Returns true or false depending on weather the
  execution of the test suite was successful.  The boolean return only works
  for synchronous tests.  As soon as there is a single asynchronous test
  case in the T.Test instance the return value is always ``true``.

``string s = tostring( testInstance )  [__toString]``
  Returns a string which is a TAP report of the Test suite.

``int len = #testInstance  [__len]``
  Returns the number of T.Test.Case instances in this suite.

