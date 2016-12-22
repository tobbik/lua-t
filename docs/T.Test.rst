lua-t T.Test - The Test Library
+++++++++++++++++++++++++++++++


Overview
========

T.Test provides a class to create unit test suites.  It also has a deep
table comparing functions.  If T.Test gets provided with a `setUp` and a
`tearDown` function these will be run before and after each test case.
Depending on how T.Test gets invoked tests will be executed in guaranteed
or random order::

   -- invoking T.Test with an empty construtor and adding tests later will
   -- make them get executed in guaranteed order
   t = T.Test( )
   t.setUp      = function( self ) ... end
   t.tearDown   = function( self ) ... end
   t.test_one   = function( self ) ... end
   t.test_two   = function( self ) ... end
   t.test_three = function( self ) ... end

   -- invoking T.Test with a table as a parameter to the construtor will
   -- make them get executed in the order pairs iterates over the Test suite
   -- table (randomized at the time of instantiation)
   t = T.Test({
      t.test_one   = function( self ) ... end,
      t.test_two   = function( self ) ... end,
      t.test_three = function( self ) ... end
   })

The test suite cannot be passed any numerically keyed elements.

Conventions
-----------

 - If a `setUp` function is provided it will be executed **BEFORE EACH**
   test case
 - If a `tearDown` function is provided it will be executed **AFTER EACH**
   test case
 - A test case function MUST start with the word `test`, otherwise it will
   become just a member of the test case which will be still accessible via
   `self.[variable_name]`
 - To provide a more detailed description than the test functions name a
   special comment `-- #DESC:` can be provided within the function source
   code.  NOTE: the comment MUST be within the functions source code, not
   above or below::

     t.test_one = function( self )
        -- #DESC: Test for equality of two numbers
        assert( self.a==self.b, "The two numbers should be the same" )
     end

 - To skip a test a special comment `-- #SKIP:` can be provided within the
   function source code.  NOTE: the comment MUST be within the functions
   source code, not above or below::

     t.test_two = function( self )
        -- #SKIP: Currently the numbers are not equal somehow...
        assert( self.a==self.b, "The two numbers should be the same" )
     end

 - To have a test expected to fail a special comment `-- #TODO:` can be
   provided within the function source code.  A test marked with a TODO will
   be run but is expected to fail.  As such it will not affect the overall
   outcome of the test suite if failed.  NOTE: the comment MUST be within
   the functions source code, not above or below::

     t.test_three = function( self )
        -- #TODO: This is such a next week issue ..
        assert( self.a==self.b, "The two numbers should be the same" )
     end


API
===

Class Members
-------------

boolean *t* = T.Test.equal( val *a*, val *b* )
  Compares value a and b with the following precedence:

  - if they have the same reference
  - if they have the same scalar value
  - if they have an __eq metamethod and return that result
  - if thay are tables and table length is equal, recursively compare them
    and their subtables


Class Metamembers
-----------------

T.Test *tx* = T.Test( [ table *t* ] )   [__call]
  Creates a new Unit Test Suite.  If a table is passed the table will be
  converted into a unittest.  The table can not contain **ANY** numeric
  keys.


Instance Members
----------------

T.Test instances do not have any specila instance members.  Any test that
gets passed to t as a "test__" named test function gets converted to a
T.Test.Case instance.


Instance Metamembers
--------------------

boolean *x* = t.test suite  [__call]
  Executes the test suite.  Returns true or false depending on weather the
  execution of the test suite was successful.

string *s* = tostring( testInstance )  [__toString]
  Returns a string which is a TAP report of the Test suite.


