lua-t T.Test.Case - The Test CaseLibrary
++++++++++++++++++++++++++++++++++++++++


Overview
========

T.Test.Case is an internal class providing functionality around functions
ran within T.Test suites.  A *test_** named function handes to a test case
will be converted to a T.Test.Case instance.


Conventions
-----------

API
===

Class Members
-------------

Class Metamembers
-----------------

There is no T.Test.Case constructor since the cases are created from within
T.Test itself.


Instance Members
----------------

function *f* = testCaseInstance.f
  The actual function passed to T.Test suite.

string *s* = testCaseInstance.name
  The name of the T.Test.Case.  It has the value of the function name when
  passed to T.Test suite.

string *t* = testCaseInstance.todo
  Contains the reason for being a TODO.  If it is `nil` the test case is
  expected to pass.

string *s* = testCaseInstance.skip
  Contains the reason for being SKIP.  If it is `nil` the test case will be
  executed by the runner.  If it has a value it will be skipped.

string *d* = testCaseInstance.desc
  Contains an optional description of the Test case.  If it is `nil` the
  case will describe itself by it's *name* instead.

string *m* = testCaseInstance.message
  If execution fails the message contains the system error message.

string *t* = testCaseInstance.traceback
  If execution fails the message contains the traceback gathered by the
  interpreter.

string *l* = testCaseInstance.location
  If execution fails the location contains `filename:linenumber`.

string *s* = testCaseInstance.src
  Contains the source code of the test case function *f*.

boolean *p* = testCaseInstance.pass
  True if the test case passed, false if it failed.  If *p* is *nil* the
  test was never executed.


Instance Metamembers
--------------------

boolean *x* = t.testCase( T.Test *suite* )  [__call]
  Executes the test case.  T.Test *suite* must be passed as an argument.
  Returns true or false depending on weather the execution of the test case
  was successful.

string *s* = tostring( *testCase* )  [__toString]
  Returns a string representing a TAP line for the test case.  Formats extra
  information as YAML.::

    Test Case description or test name
       ---
       name : test_Name
       message : ../lua-t/example/t_tst.lua:75: Assert Message for failure
       location : ../lua-t/example/t_tst.lua:75:
       traceback : stack traceback:
           [C]: in function 'assert'
           ../lua-t/example/t_tst.lua:75: in function <../lua-t/example/t_tst.lua:71>
           [C]: in ?
           [C]: in global 't'
           ../lua-t/example/t_tst.lua:116: in main chunk
           [C]: in ?
       src :
             71:   test_Name = function( self )
             72:           -- #DESC: Test Case Description
             73:           local h = 3
             74:           local k = 4
             75:           assert( h == k, "Assert Message for failure" )
             76:   end,
       ...

