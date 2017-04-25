Origins of the lua-t Lua Toolbox
================================


Roots
-----

The code was originally a C program, designed to spit out pseudo-random bit
and byte sequences and send it to a listening (UDP) server.  It was my (day)
job to develop the server and the development of the hardware nodes supposed
to send the messages was too far behind to develop the server code.  A long
story short, to make my day job possible I took the little C program added
Lua as a configuration language and added the ability to send more
configurable messages to configurable servers.  Over much iteration, all
during my spare time, I turned the Lua configured C progam into a C-Library
for Lua which created the foundation for lua-t.


Evolution
---------

When I came back to the code after leaving it alone for a long time I
started to organize, document and unit test it.  Once happy with it it
became a release.  The feature set of lua-t in its inital release is
entirly driven by the original requirements for the original C program and
while it intersects with many other existing libraries it still offers some
very unique features such a bit resolution binary packging.


Move code from C only to Lua and C
----------------------------------

While it was a great exercise to write **everything** in C it made the size
of the t.so file increase steadily and hence load a lot od code even if just
one function was needed.  Eventually the code got broken up into modules, a
single .so file for each and .lua modules to wrap the functionality.  It
also allowed to re-implement whole modules in Lua such as `OrderedHashTable`
, `Set` or `Test`.  It improved the managability a lot but complicated the
build process slightly which was a worthy tradeoff.


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

Next up would be improving platform portability.  With the asynchronous
functionality that is no easy task though.


Roadmap
-------

 - make the asynchronous event loop handle coroutines in parallel with
   callbacks
 - get the HTTP Server and client working properly
 - create a WebSocket server and client
 - have a good look at axTLS to handle https/wss


