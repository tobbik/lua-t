lua-t, Source tests
====================

Overview
++++++++

What is tested
--------------


The intergety if the C-Source code. This is not testing the Lua-side of things.
It is implemented as main() functions to a specific file and tests each function
individually.

How it works
------------

The Makefile combines the source code and the the test code into one file before
compiling it and executing it.  The test code contains a main which gets
executed and provides a test run result upon completion.
