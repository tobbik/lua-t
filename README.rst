lua-t, A Lua Toolbox
====================

Overview
++++++++

What is it
----------

lua-t is a library which extends Lua by various functionality.  It aims to
be very object oriented and in itself coherent using the same pattern of OOP
in all places offering a usable API.  It follows the Lua guidelines for OOP.
lua-t is less of a system library than it is an extension which get's
compiled into a static or standalone Lua executable for a self contained
application.  It covers some areas which are already served by other
libraries such as lua-socket, however, it aims to bring functionality
together under one umbrella and provide a framework to implement more C
based functionality with less overhead.  This makes programming easier on
the Lua side of things as it makes for a very consistent way to code.


Lua vs C
--------

lua-t started out as a pure monolithic C-library.  This design allowed to
have C-based functions that can be reused within the framework itself.  It
since moved to a layer of Lua libraries on top of the C-Monolioth which
provide the C-functions that can't be implemented in Lua.  It makes the
monolith significantly smaller.

Eventually, the native module have been split into submodules and the code
got re-organized to re-use code at minimal duplication (binary duplication
that is).  This allows loading more specifically what is needed and keeps
the memory footprint low.  Going forward, modularization is becoming a more
important factor in designing functionality maybe to the point that the
desired group of functionality can be defined at build time.


Functionality
-------------

When faced with writing an increasingly complex simulator I wished I had the
library lua-t is today.  The particular requirements (Packer, Network,
Async) of that project drove the features in lua-t for a long time.  Writing
a lot of code over the years left me with lots of availabe but incoherent
functionality.  lua-t is the attempt to bring all of it into a
comprehensive, well tested, well documented and easy to use library.  As
time progresses and users pick it up the available functionality will be
made more rounded and more useful.


Contents (High level overview)
-----------------------------

 - Networking (t.Net.*)   --> TCP,UDP,Sockets,Addresses,Interfaces etc
 - Buffers (t.Buffer)     --> buffers of defined length with mutable values
 - Packers (t.Pack.*)     --> binary Packer/Parser for several data types
 - Encoding (t.Encoding.*)--> En/Decoding, En/Decryption, Hashes etc.
 - Unit Tests (t.Test)    --> comprehensive unit tests
 - HTTP Server (t.Http)   --> An asynchronous HTTP Server implementation
 - Csv Processor (t.Csv)  --> A fast and flexible CSV parser
 - Asynchronous (t.Loop)  --> event/select based asynchronous workings
 - OrderedHashTable       --> Hashtable that preserves insertion order
 - Sets (t.Set)           --> Feature rich Set implementation
 - Time (t.Time)          --> Very simple wrapper defining a duration


Compatability
-------------

lua-t was written with Lua-5.3 in mind. And recent development was performed
on Lua-5.4.0 Release Candidates.  It is known to work on these versions and
islikely to brakes with older versions.  There are no immidiate plans to
make it compatible with older versions of Lua.  Recently, the t.Loop library
makes use of multiple userdata per uservalue, which is a feature that
requires Lua 5.4 and greater.


Documentation
-------------

Please refer to the docs/\*.rst files for in detail documentation.


License
-------

lua-t is released under the same MIT license Lua 5.3 (and other versions) is
release under.  The detailed License text is at the end of src/t.h.
