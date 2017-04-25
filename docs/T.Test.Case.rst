lua-t Test.Case - The Test CaseLibrary
++++++++++++++++++++++++++++++++++++++++


Overview
========

Test.Case is an internal class providing functionality around functions
executed within `Test` suites.  A **test_**  or **test_cb_** named function
handed to a `Test` suite instance will be converted to a `Test.Case`
instance.


Usage
=====

Some general information on how to write and invoke `Test.Case`.

Hooks
-----

``t.beforeEach = function( self )``
  The hook gets called before executing each test case in the suite.

``t.afterEach = function( self )``
  The hook gets called after executing each test case in the suite.

``t.beforeEach_cb = function( self, done )``
  The hook gets called before executing each callback based test case in the
  suite.  The `done()` callback must be called when the test finishes,
  otherwise test execution will be stalled.  The test runner does not fall
  back to use `t.beforeEach` if `t.beforeEach_cb` is not found.  If your
  `Test` suite mixes synchronous and callback based tests you can do the
  following to save code duplication.

.. code:: lua

    t.beforeEach_cb = function( self, done )
       self:beforeEach( )        -- call synchronous beforeEach
       -- do some asynch stuff here if needed ...
       done()
    end

``t.afterEach_cb = function( self, done )``
  The hook gets called after executing each callback based test case in the
  suite.  The `done()` callback must be called when the test finishes,
  otherwise test execution will be stalled.  Like `t.beforeEach_cb`` there
  is no fall back to `t.beforeEach`` for callback based test cases.


API
===

Class Members
-------------

``void Test.Case.describe( 'Rich description for this Test.Case' )``
  This is meant to be called from within a `Test.Case`.  By default the test
  is described by the function name that was used to assign it to the `Test`
  suite.  Calling this function will overwrite that default description.

.. code:: lua

  t.test_WhatEverToTest = function( self )
    Test.Case.describe( "Explain in nicer words what it does" )
    ... implementation ...
  end

``void Test.Case.todo( 'The reason why this Test.Case shall fail' )``
  This is meant to be called from within a `Test.Case`.  If a call to
  `Test.Case.todo()` happens the test runner will expect that the test
  fails.  If the test succeeds despite the call to `Test.Case.todo()`
  the test runner will mark the entire `Test` suite as failed.

``void Test.skip( 'The reason why this Test.Case shall be skipped' )``
  This is meant to be called from within a `Test.Case`.  It will skip the
  test at the point where it is called and it will set the skip reason so it
  can be displayed in the summary.  The function is implemented as a
  controlled call to `error()` which will invoke the traceback for the
  wrapping `xpcall()`.  The traceback will recognize the special invocation
  and act accordingly.  A side effect of implementing skip as a function
  call may be that a `Test.Case` can fail before `skip()` gets called.  So
  it is advisable to call `skip()` early in a test function.  However, it
  has the advantage to call `skip()` based on a condition:

.. code:: lua

  t.test_SkipWhenCalledTooEarly = function( self )
    if os.date('*t').hour < 10 then
      Test.Case.skip("Sorry, I' don't wake before coffee ...")
    end
    ... implementation ...
  end


Class Metamembers
-----------------

``Test.Case tc = Test.Case( [ string 'test_name', function tf, function join ] )   [__call]``
  Creates a new `Test.Case`.  `test_name` is mandatory and get's set as
  description.  If the test name starts with ``test_cb_`` the test will be
  constructed as callback based Test.Case and during execution time a
  callback will be passed in a second parameter (after ``self``).
  `function tf` is mandatory and get's set as the actual executable test
  case.  Finally, `function join` is called **after** the test case and
  `Test` hook afterEach.  The `Test` implementation uses it to iterate over
  all `Test.Case` instances in a `Test` and probes them if they had been
  executed.  This way all asynchronous tests get checked and the Test runner
  can determine whenthe execution of a `Test` suite has finished by testing
  that **each** `Test.Case` within a `Test` suite has been executed.


Instance Members
----------------

``function f = testCaseInstance.function``
  The actual function executed as `Test.Case`.

``string s = testCaseInstance.description``
  The name of the `Test.Case`.  It has the value of the function name when
  created from a `Test` suite.  It can be changed during the execution of
  the `Test.Case` by calling `Test.Case.describe()`.

``string t = testCaseInstance.todo``
  Contains the reason for being a TODO.  If it is `nil` the test case is
  expected to pass.  If it is set the `Test.Case` execution is expected to
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
  If execution fails the location contains `filepath:linenumber`.

``string s = testCaseInstance.source``
  Contains the source code of the test case function .

``boolean p = testCaseInstance.pass``
  True if the test case passed, false if it failed.  If `p` is `nil` the
  test was never executed.

``string t = testCaseInstance.testtype``
  Can be `standard` or `callback`.  If it is a `callback` the
  `testCaseInstance.function` must call the ``done()`` callback to continue
  execution.

``Time t = testCaseInstance.executionTime``
  A `Time` instance which measures the time to execute the actual
  `Test.Case` function.  It does **not** include the execution time for
  hooks and therefor can easily be used as a benchmark tool.  For callback
  based `Test.Cases`, the `executionTime` is set as the first thing in the
  `done()` function.


Instance Metamembers
--------------------

``boolean x = t.testCase( Test suite )  [__call]``
  Executes the test case.  `Test suite` must be passed as an argument.
  Returns true or false depending on weather the execution of the test case
  was successful unless it was a *callback* `testtype` which always returns
  `true`.

``string s = tostring( Test.Case test_case )  [__toString]``
  Returns a string representing a TAP line for the test case.  Formats extra
  information as YAML.  Extra information will be formatted as Yaml as per
  TAP v13 specifications:

.. code:: yaml

    description : Test Case description
    testtype: standard
    pass: false
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
      72:      Test.Case.describe( "Test Case Description" )
      73:      local h = 3
      74:      local k = 4
      75:      assert( h == k, "3 really shouldn't be 4, doh ..." )
      76:   end,

