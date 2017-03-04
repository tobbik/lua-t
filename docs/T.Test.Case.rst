lua-t T.Test.Case - The Test CaseLibrary
++++++++++++++++++++++++++++++++++++++++


Overview
========

T.Test.Case is an internal class providing functionality around functions
executed within T.Test suites.  A **test**  or **test_cb** named function
handed to a T.Test suite instance will be converted to a T.Test.Case
instance.


Conventions
-----------

API
===

Class Members
-------------

``void T.Test.Case.describe( 'Richer description for this T.Test.Case' )``
  This is meant to be called from within a T.Test.Case.  By default the test
  is described by the function name that was used to assign it to the T.Test
  suite.  Calling this function will overwrite that default description.

``void T.Test.Case.todo( 'The reason why this T.Test.Case shall fail' )``
  This is meant to be called from within a T.Test.Case.  If a call to
  ``T.Test.Case.todo()`` happens the test runner will expect that the test
  fails.  If the test succeeds despite the call to ``T.Test.Case.todo()``
  the test runner will mark the entire T.Test suite as failed.

``void T.Test.skip( 'The reason why this T.Test.Case shall be skipped' )``
  This is meant to be called from within a T.Test.Case.  It will skip the
  test at the point where it is called and it will set the skip reason so it
  can be displayed in the summary.  The function is implemented as a
  controlled call to luaL_error which will invoke the tracback for the
  wrapping ``lua_pcall()``.  The traceback will recognize the special
  invication and act accordingly.  Implementing skip as a function has the
  following effects:

    # it can be used in a condition:
      ``if not server then Test.skip('Server Unavailable')``
    # if a test fails before skip gets called it is still a failed test
      because the call to T.Test.Case.skip() will never execute


Class Metamembers
-----------------

``T.Test.Case tc = T.Test.Case( [ 'test_name', function ] )   [__call]``
  Creates a new T.Test.Case.  Takes ``test_name`` as preliminary description
  and the function to be executed.


Instance Members
----------------

``function f = testCaseInstance.function``
  The actual function passed to T.Test suite.

``string s = testCaseInstance.description``
  The name of the T.Test.Case.  It has the value of the function name when
  passed to T.Test suite.  It can later be changed by calling
  T.Test.Case.describe( "New description" )

``string t = testCaseInstance.todo``
  Contains the reason for being a TODO.  If it is `nil` the test case is
  expected to pass.  If it is set the T.Test.Case execution is expected to
  fail.

``string s = testCaseInstance.skip``
  Contains the reason for being skipped.  If it is `nil` the test case will
  be executed by the runner.  If it has a value it will be skipped.

``string m = testCaseInstance.message``
  If execution fails the message contains the error message.  If a call to
  ``assert()`` fails it contains the assert message.

``string t = testCaseInstance.traceback``
  If execution fails the message contains the traceback gathered by the
  virtual machine.

``string l = testCaseInstance.location``
  If execution fails the location contains `filename:linenumber`.

``string s = testCaseInstance.source``
  Contains the source code of the test case function .

``boolean p = testCaseInstance.pass``
  True if the test case passed, false if it failed.  If `p` is `nil` the
  test was never executed.

``string t = testCaseInstance.testtype``
  Can be `standard` or `callback`.  If it is a `callback` the
  `testCaseInstance.function` must call the ``done()`` calback to continue
  execution.
  test was never executed.


Instance Metamembers
--------------------

``boolean x = t.testCase( T.Test suite )  [__call]``
  Executes the test case.  T.Test `suite` must be passed as an argument.
  Returns true or false depending on weather the execution of the test case
  was successful unlessit was a *callback* `testtype` which always returns
  `true`.

``string s = tostring( T.Test.Case test_case )  [__toString]``
  Returns a string representing a TAP line for the test case.  Formats extra
  information as YAML.::

    Test Case description or test_functionName
       ---
       description : Test Case description
       testtype: standard
       pass: true
       message: Assert Message for failure
       location: ../lua-t/example/t_tst.lua:75:
       traceback: stack traceback:
           [C]: in function 'assert'
           ../lua-t/example/t_tst.lua:75: in function <../lua-t/example/t_tst.lua:71>
           [C]: in ?
           [C]: in global 't'
           ../lua-t/example/t_tst.lua:116: in main chunk
           [C]: in ?
       source:
             71:   test_Name = function( self )
             72:           -- #DESC: Test Case Description
             73:           local h = 3
             74:           local k = 4
             75:           assert( h == k, "Assert Message for failure" )
             76:   end,
       ...

