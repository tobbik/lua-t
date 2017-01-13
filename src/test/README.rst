lua-t, Source tests
===================

Overview
++++++++

Testing of the C parts of lua-t.  This allows to test portions of the code
independently from Lua and thus become more true to the idea of unit
testing.  See "t_unittest" for a more detailed explaination of the
mechanics.


What is tested
--------------

A good part of lua-t is composed of pure C-functions providing the
functionality while lua-t becomes merely a binding to these functions.  This
code allows to test these functions in isolation so the Lua biniding is
taken out of the equation.



t_unittest
++++++++++

Basic functionality
-------------------

t_unittest works as a code generator.  Source files get concatenated and
then equipped with a main function.  This limits the functionality to
testing libraries which do not come with their own main function.  To make
it work the t_unittest utility provides a tool called ``cat`` which is used
to concatenate files and inject some helper and and ``main()`` function at
the end of the concatenated file.  The result of the concatenation is piped
into the compiler.  Executing the compiled result will actually run the unit
test.

How to run the ``cat`` utility
------------------------------

t_unittest has no way of resolving dependencies.  All dependencies must be
passed to ``cat``.  There are three types of files, and the shall be passed
to ``cat`` in exactly that order.  It is important that the *testcase* is
the last file passed in:

 - *dependent* a file needed for the tested file
 - *tested* - the file which will, be tested in the unittest
 - *testcase* - the file with all the tests

Here is an example on how to run cat and pipe it to the compiler::

   ./cat ../dep1.c ../dep2.c ../tested.c test_for_tested.c | gcc -x c -I../ \
       -Wall -Wextra -O0 -std=c99 -fpic -g - -o test_for_tested -lm

Now test_for_tested can be executed ``./test_for_tested`` to run the tests.

How to write tests
------------------

A test function, or test case as you can call it, must take no arguments and
must return 0 upon successful completion.  Otherwise the test counts as
failed.  The ``_assert( boolean, string )`` macro provides a way to test a
condition and report an error if the condition failed to be true.::

   static int
   test_Are_the_numbers_ok( )
   {
      _assert( 1==1  ,  "Numbers shall be equal" );
      _assert( 5-5==0,  "Substraction shall be working" );
      _assert( 5+5==10, "Substraction shall be working" );
      _assert( 1==0,    "This shall fail" );
      return 0;
   }


Setup and teardown
------------------

Each test file must provide a setup and a teardown function which have the
same signature as test cases.  These functions are used to provide
preparation and for and cleanup after a unit test.  They are run before and
after each test.  The functions **must be present** and their names are
mandated as ``int t_utst_case_setup()`` and ``int t_utst_case_teardown()``.

The Test registry
-----------------

At the end of each test suite a registry of test must be provided which
follows the following rules:

 - Must be of type ``static const struct t_utst_case []``
 - Must be named   ``t_utst_all_tests``
 - Must have a sentinel ``{NULL,NULL}`` to mark the end of the test suite

All Tests in the suite will be executed in the oprder provided in the
registry.
