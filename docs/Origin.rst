Origins of the Lua Toolbox
==========================

Roots
-----

The code was originally a C program, able to spit out pseudo-random bit and
byte sequences and send it to a listening (UDP) server.  Its purpose was to
simulate network nodes that were not yet developed and hence couldn't be
used to develop the server software.  The complexity, howerver ,grew at such
a fast pace that maintaining the tool became unfeasable.

Introducing Lua to the project
------------------------------

Originally it was envisioned to use Lua as a scripting language on the
network nodes it was soon to be used to configure the little C-Tool.  It
enabled to easily switch servers or reconfigure the package content.
However, using C-structs as the base for the package structure became too
rigid.  And with delays in the development of the network nodes a more
powerful and flexible solution was needed.  Unfortunately, the entire
project had no budget for "faking" the network nodes.  Thus keeping the
"simulator" simple was important.


Turning it it onto it's head
----------------------------

The requirements for the "simulator" soon became much higher.  It was
expected to simulate hundreds of nodes each spitting out a message a second.
Additionally, the messages couldn't be arbitrar but the meaning had to be
synchronized across the simulated nodes to make sense, thus creating a need
to do all this from a single thread.  After doing a little research and not
finding a binary packer/parser flexible enough for the project needs, I
decided to write a library that was written in C for Lua which would allow
to create messages flexibly and efficiently and some asynchronous magic to
allow for the multiple nodes to be simulated.  With no budget at hand, I
decided to write it as a library in my spare time which I could use at work
to make my job even possible.


The odd tools
-------------

The requirements at work drove the feature development of lua-t entirely.
This is how the odd combination of features was created.  The fast pace of
implementation created a code mess and progress wasn't possible anymore.  To
regain control, I rewrote the entire codebase over a couple of weekends,
implementing a rigidly structured scheme which organized layout and control
flow.  This makes it easier to read and understand the code and find bugs
faster.  However, it was not very well tested overall.  Some parts that were
heavily used have been tested and optimized, others were implemented as one
off convienience functions and are literally untested.  Though lua-t has
it's own unit test framework it didn't have any test coverage for it's own
functionality.


Why rolling my own
------------------

As mentioned, the main driver of creating this library was the bit and byte
packing capability.  At the time Lua did not have any of this build in (pre
5.3 era) and the available libraries did not have and still not have bit
resolution.  With the bit operators of Lua 5.3 it would have been possible
to write bit handling on to of byte packing but at that time lua-t was
already very developed.  Why not use lua-socket?  I simply need better
asynchronous capabilities.  When I studied Redis' asynchronous event loop I
saw how I could make a native Lua event loop that could be packaged.  I
acknowledge that there are libraries out there (Lua wrappers for libuv) that
could have been made work.  But writing my own allowed me to keep it within
the coding structure I had developed for lua-t.


Where to go
-----------

The first step is to solidify the unit test framework and put it to good
use.  The plan is to move over to a completely Test driven development.
Then, each library should get improved by doing the following steps:

 - write a comprehensive set of unit tests
 - improve the library and bring it up to standard (Coding.rst)

   - add clone constructors ( where applicable )
   - add comparator meta-methods ( where applicable )
   - fix file layouts, bring in line with coding guide
   - update comments
   - write or complete documentation
   - improve example files

Next up would be improving portability.  With the asynchronous functionality
that is no easy task though.


Roadmap
-------

 - make more classes table based instead of using structs (if possible)
   this will make it easier to convert whole things to Lua code instead of C
 - write Asynchronous T.Tests (by passing down and handling of a done()
   method)
 - make the asynchronous event loop handle coroutines in parallel with
   callbacks
 - get the HTTP Server and client working properly
 - create a WebSocket server and client
 - have a good look at axTLS to handle https/wss

