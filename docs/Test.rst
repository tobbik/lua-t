lua-t Test - The Test Case Library
++++++++++++++++++++++++++++++++++


Overview
========

Test is an internal class providing functionality around functions
executed as a test.  When running a function as a test the will be a result
be returned indicating the tests success and metrics.


Usage
=====

Some general information on how to write and invoke ``Test``.

API
===


Class Members
-------------

``void Test.describe( 'Rich description for this Test.Case' )``
  This is meant to be called from within a ``Test.Case``.  By default the
  test is called *"Unnamed test"*. Calling this function will overwrite that
  default description.

  .. code:: lua

    t.WhatEverToTest = function( self )
      Test.Case.describe( "Explain in nicer words what it does" )
      ... implementation ...
    end

``void Test.todo( 'The reason why this Test.Case shall fail' )``
  This is meant to be called from within a ``Test`` function.  If a call to
  ``Test.todo()`` happens the test runner will not care if that the test
  fails and instead returns a ``pass`` status.

``void Test.skip( 'The reason why this Test.Case shall be skipped' )``
  This is meant to be called from within a ``Test`` function.  It will skip
  the test at the point where it is called and it will set the skip reason
  so it can be displayed in the summary.  The function is implemented as a
  controlled call to ``error()`` which will invoke the traceback for the
  wrapping ``xpcall()``.  The traceback will recognize the special
  invocation and act accordingly.  A side effect of implementing skip as a
  function call may be that a ``Test`` can fail before ``skip()`` gets
  called.  So it is advisable to call ``skip()`` early in a test function.
  However, it has the advantage to call ``skip()`` based on a condition:

  .. code:: lua

    t.SkipWhenCalledTooDarnEarly = function( )
      if os.date('*t').hour < 10 then
        Test.skip("Sorry, I' don't wake before coffee ...")
      end
      ... code that fails without coffee ...
    end

``string src = getFunctionSource( function f )``
  Attempts to read the source of a function definition from the file itself.
  It depends on the source being Lua code that was read from a file and not
  from <stdin> or othe sources.

``string tapString = Test.tapOutput( tst )``
  Returns the ``Test`` result as a TAP complient YAML output.

  .. code:: yaml

    ---
    description: Cancel existing and running Task
    executionTime: 101
    status: FAIL
    message: This task function should not have been executed!
    location: /home/tobias/RaspberryPi/alarm/mygit/lua-t/test/t_ael.lua:13
    traceback: 
      [C]: in function 'assert'
      /home/tobias/RaspberryPi/alarm/mygit/lua-t/test/t_ael.lua:13: in function </home/tobias/RaspberryPi/alarm/mygit/lua-t/test/t_ael.lua:12>
      [C]: in method 'run'
      /home/tobias/RaspberryPi/alarm/mygit/lua-t/test/t_ael.lua:52: in function 't_ael.CancelTask'
      [C]: in function 'xpcall'
      ...spberryPi/alarm/mygit/lua-t/out/share/lua/5.4/t/Test.lua:115: in upvalue 'Test'
      ...yPi/alarm/mygit/lua-t/out/share/lua/5.4/t/Test/Suite.lua:95: in upvalue 'Suite'
      /home/tobias/RaspberryPi/alarm/mygit/lua-t/test/runner.lua:32: in local 'run'
      /home/tobias/RaspberryPi/alarm/mygit/lua-t/test/runner.lua:45: in main chunk
      [C]: in ?
    testSource: 
      44:   CancelTask = function( self )
      45:     Test.describe( "Cancel existing and running Task" )
      46:     local tsk1, tsk2 = nil, nil
      47:     local t1 = function( )
      48:       self.loop:cancelTask( tsk2 )
      49:     end
      50:     tsk1 = self.loop:addTask( 500 , t1 )
      51:     tsk2 = self.loop:addTask( 100, taskNotRun )
      52:     self.loop:run( )
      53:   end,
    failedSource: 
      12: local taskNotRun = function( )
      13:   assert( false, "This task function should not have been executed!" )
      14: end
    ...


Class Metamembers
-----------------

``bool ok, Test tst = Test( function tf, [... args] )   [__call]``
  Creates a new ``Test``.  ``function tf`` is the test function and will be
  called protected by ``xpcall( tf )`` with the arguments passed in
  ``args``. ``bool ok`` provides high level information if the test was
  successful.


Instance Members
----------------


``string s = testInstance.description``
  The name of the ``Test``.  It has the value of *"Unnamed test"* when
  created.  It can be changed during the execution of the ``Test``
  function by calling ``Test.describe()``.

``string m = testInstance.message``
  If execution fails the message contains the error message.  If a call to
  ``assert()`` fails it contains the assert message.  If the tes function
  called ``Test.skip(msg)`` or ``Test.todo(msg)`` the value of ``string
  msg`` will end up in ``testInstance.message``.

``string t = testInstance.traceback``
  If execution fails the message contains the traceback gathered by the
  virtual machine.

``string l = testInstance.location``
  If execution fails the location contains ``filepath:linenumber``.

``string s = testInstance.testSource``
  Contains the source code of the test case function .

``string s = testInstance.failedSource``
  If execution fails ``T.Test`` attempts to locate the execution failure.
  This should containm the source code of the function where execution
  failed.

``boolean p = testInstance.pass``
  True if the test case passed, false if it failed.  If execution failed but
  ``Test.todo()`` was called ``testInstance.pass`` is still ``true``.

``int milliseconds = testInstance.executionTime``
  Measures the time to execute the actual ``Test`` function.

``int milliseconds = testInstance.runTime``
  If executed withi a ``Test.Suite`` context it measures the time to execute
  the ``Test`` function and the ``Test.Suite`` hooks, such as
  ``beforeEach()`` and ``afterEach()``.

``string status = testInstance.status``
  Contains string describing the execution status.  There are four
  possibilities: ``PASS``, ``FAIL``, ``SKIP`` and ``TODO``.



Instance Metamembers
--------------------

``string desc = Test( function tf, [... args] )   [__tostring]``
  Creates a new ``Test``.  ``function tf`` is the test function and will be
  called protected by ``xpcall( tf )`` with the arguments passed in
  ``args``. ``bool ok`` provides high level information if the test was
  successful.

``string s = tostring( Test tst )  [__tostring]``
  Returns the description of the ``Test tst``.  If the test has a ``SKIP``
  or a ``TODO`` status the string will be composed TAP compliant by
  appending *# SKIP: ``test,message``* to the description.
