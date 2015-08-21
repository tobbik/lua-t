lua-t, A Lua Toolbox
====================

Overview
++++++++

What is it
----------

lua-t is a library which extends Lua by various functionality.  It aims to be
very object oriented and in itself coherent using the same pattern of OOP in
all places offering a usable API.  It follows the Lua guidelines for OOP.
lua-t is less of a system library than it is an extension which get's compiled
into a static or standalone Lua executable for a self contained application.
It covers some areas which are already served by other libraries such as
lua-socket, however, it aims to bring functionality together under one umbrella
and provide a framework to imlpement more C based functionality with less
overhead.  This makes programming easier on the Lua side of things as it makes
for a very consistent way to code.


Contents (High level overview)
-----------------------------

 - Networking (t.Socket)  --> TCP,UDP etc
 - Buffers (t.Buffer)     --> a buffer of defined length with mutable values
 - Packers (t.Pack)       --> a binary Packer for several types of data
 - Encoding (t.Encoding)  --> En/Decoding, En/Decryption, Hashes etc.
 - Unit Tests (t.Test)    --> comprehensive tests with unified output
 - HTTP Server (t.Http)   --> An asynchronous HTTP Server implementation
 - Asynchronous (t.Loop)  --> event/select based asynchronous workings


Maturity
--------

The core functionality of lua-t was written as a by-product of implementing a
mocking framework which reads/write packed binary data from and to network
buffers.  lua-t originally was written in short periods of spare time, making
my daytime job easier/possible.  Trying to implement it fast using a rigidly
structured scheme to the code kept the code base organized.  However, it is not
very well tested overall.  Some parts that were heavily used have been tested
and optimized, others were implemented as one off convienience functions and are
literally untested.  lua-t has it's own unit test framework.  ONe of the first
steps is to put the current code under maximum test coverage.


Compatability
-------------

lua-t was written with Lua-5.3 in mind.  That is what it uses and it likely
brakes with older versions.  There are no immidiate plans to make it compatible
to Lua 5.2 or 5.1.


Documentation
-------------

Please refer to the docs/\*.rst files for in detail documentation.


License
-------

lua-t is released under the same MIT license Lua 5.3 (and other versions) is
release under.  The detailed License text is at the end of src/t.h.
