lua-t Loop - An Asynchronous Event Loop
+++++++++++++++++++++++++++++++++++++++


Overview
========

This provides an asynchronous eventloop that is intended to be platform
independent.  It is based on redis ae_* code base and basically provides a
bridge to Lua making it a native member of Lua.  The eventloop is build to
handle Socket handlers and ``T.Time`` objects which are basically ``struct
timeval`` wrappers.  General invokation looks like:

  .. code:: lua

   Loop = require't.Loop'
   l = Loop( )

Technologies like luvit or NodeJs utilize similar ideas and underlying
technologies to handle io.  By including the loop in the interpreter they
can hide a lot of implementation detail from the user.  As a side effect of
tying the loop and the interpreter tightly together the software is fixed to
the version of the interpeter which creates the need for their own
eco-system.  Now, both luvit and node, have a great eco-system and npm is a
fantastic package manager but in Lua land it remains desirable to just
choose and pick the lates official Lua distribution from PUC.  Therefore,
lua-t makes an effort to be 'just another library' that can be loaded and
used in conjunction with all the other software that is available either
via the distribution or lua-rocks.

Only ``epoll()`` and ``select()`` are currently implemented.


Implementation
==============

``t.Loop`` is very loosely based on the ae_* eventloop found in redis.
General architecture is similar as that it have a general part which handles
the common code and then implementation/platform specific code which handles
the abstraction regarding the use of the underlaying technology.  While not
fully implemented yet the following platforms are planned to be supported:

  - ``select()`` platform independent with known select() limitations
  - ``epoll()``  default on Linux systems, and probably best tested
  - ``kqueue()`` default on OS X and all \*Bsd platforms
  - ``evport()`` Solaris
  - ``cpio``     Windows (inspiration from Microsoft redis port)

Where ``t.Loop`` differs significantly from redis is the handling of slots
for descriptors to be watched.  In redis the user is responsible to not add
a file/socket descriptor to the loop which has a higher descriptor number
then the slot capacity of the loop and the user must resize the loop (which
uses realloc()) manually because the slots are managed as a fixed size
array.  ``t.Loop`` uses a Lua table instead which is garbage collected by
Lua itself.  There are some performance implications but in the interest of
easier handling this implementation is much preferable.  It keeps ``t.Loop``
in the spirit of Lua as a scripting language where the user shall not be
concerned with those kind of aspects.  On a technical level, ``t.Loop`` sets
a slot via ``slotTable[ fdNumber ]``.  Since there is no concept of filling
the table consecutively, Lua handles that as an associative table.  It is
still plenty fast.


Major differences from redis design
-----------------------------------

 - this is not a binding to the redis event-loop, but a reimplementation
   which uses lua_State all the way through
 - instead of C functions it directly executes Lua functions with Lua based
   parameters when events are fired
 - using a Lua table to hold the descriptors and their associated
   event-handlers, so it does not have to be manually resized.


Important File Handle and Platform Caveats
==========================================

Depending on the underlying eventloop implementation, it may be possible to
had it also file descriptors that have been created with Lua's ``io.open()``
method.  Not all implementations can handle regular files.  For example, the
following example fails under Linux which uses the ``epoll`` implementation
by default:

  .. code:: lua

   l = Loop( )
   f = io.open('/etc/passwd', 'r')
   p = function(fl) print( fl:read('*all') ) end
   l:addHandle( f, 'r', p, f )
   -- stdin:1: Error adding descriptor to set (Operation not permitted)

The error message says that the operation is not permitted but that is just
the kernel telling that the operation is really not supported.  That
behaviour is dependent on the file type and implementation.  The following
code for example works just fine with ``epoll`` because ``/dev/random`` is
not just a file but actually a device:

  .. code:: lua

   l = Loop( )
   f = io.open('/dev/random', 'r')
   p = function(fl) print( fl:read(45) ); l:stop() end
   l:addHandle( f, 'r', p, f )
   l:run( )

Similarily, ``epoll`` does work for ``io.popen( )``:

  .. code:: lua

   l = Loop( )
   f = io.popen('date')
   p = function(fl) print( fl:read('*all') ); l:stop() end
   l:addHandle( f, 'r', p, f )
   l:run( )

So everything under Linux is a file, but not all files are created equal!
If the underlying implementation is based ion ``select()`` the kernel has no
problem adding a regular file like ``/etc/passwd`` to the event loop, but
when the eventloop triggers a readability/writability event there is no
guarantee that a non-blocking read/write operation will actually succeed.
Under Windows, IOCP can handle that fine.  ``T.Loop`` tries to abstract many
things away but it does not go as far a libuv for example.  Therefore, it
will be possible to implement a lot of useful stuff in ``T.Loop`` but there
are some limitations which are platform specific.  For more general
information on that topic read `Asynchronous I/O in Windows for Unix
Programmers <http://tinyclouds.org/iocp-links.html>`_


Singleton
=========

While the interface suggests that there can be multiple ``t.Loop`` instances
created, **running multiple ``t.Loop`` instances is not defined**.


API
===

Class Members
-------------

None


Class Metamembers
-----------------

``Loop l = Loop( )       [__call]``
  Creates ``Loop l`` instance.  Create only one per application.  Using
  multiple loops is not defined as behaviour.


Instance Members
----------------

``string s = loop:show()``
  Print a list of elements in the loop in a preformatted way.

``void = loop:run()``
  Starts the event loop.

``void = loop:stop()``
  Stops the event loop and returns to the normal flow of execution.

``boolean b = loop:addHandle( handle h, string dir, function f, ...)``
  Add the ``handle h`` to the eventloop and define what should be executed
  when an event on the handle is observed.  The ``handle h`` can be a
  ``t.Net.Socket`` or a ``Lua File``.  Limitations apply as explained above
  in the Caveats.  The direction can be ``'r'`` or ``'w'`` determining if
  the event would indicate readability or writablity.  Upon the triggered
  event the ``function f`` will be executed with the parameters passed in
  ``...``.  ``addHandle()`` is idempotent and each call to it will
  **replace** the previously added function and parameters.

``boolean b = loop:removeHandle( handle h, string dir )``
  Remove observing events on the ``handle h`` for the direction ``string
  dir`` from the event loop.

``boolean b = loop:addTimer( t.Time t, function f, ...)``
  Add the ``t.Time t`` to the eventloop and define what should be executed
  when then ``t.Time t`` value has passed  Upon the triggered event the
  ``function f`` will be executed with the parameters passed in ``...``.
  ``addTimer()`` is idempotent and each call to it will **replace** the
  previously added function and parameters.  ``function f`` *can have* a
  single return value.  If it is an instance of ``T.Time`` it will
  automatically reschedule itself with the same parameters.  This allows
  to flexibly implement intervals.

``boolean b = loop:removeTimer( t.Time t )``
  Remove ``t.Time t`` from the event loop.

``boolean x loop:resize( [int n] )``
  Resizes slot capacity of the loop.  If the parameter ``int n is given``
  the loop will allocate the next higher power of 2 number of slots in the
  loop.  For example calling ``looo:resize( 25 )`` will allocate 32 slots in
  the loop.  When called without an argument ``loop:resize( )`` will remove
  as many slots as it can so it can still accommodate the highest descriptor
  and shrink itself to the next higest power of 2.  Therefore, a call to
  ``loop:resize( )`` is not guranteed to shrink the number of slots if the
  currently highest descriptor forbids that if the currently highest
  descriptor forbids that.


Instance Metamembers
--------------------

``string s = tostring( Loop l )  [__tostring]``
  Returns a string representing the ``Loop l`` instance.  The string
  contains type, length and memory address information such as
  *`t.Loop{7}: 0xdac2e8`*, meaning it is currently observing 7 descriptors.

``table t = Loop l[ idx ] [__index]``
  Returns a ``table t`` which is different for Timers or Handles.  The index
  must be a ``t.Time t`` or a valid File or Socket handle. For a time index
  the table contains ``[ func, arg1, arg2, ... ]``.  For handles it the
  table looks like:
  
  .. code:: lua
  
   {
     read  = { func, arg1,arg2, arg3, ... },
     write = { func, arg1,arg2, arg3, ... }
   }
  
  There is no ``__newindex()`` method since ``__index()`` has been
  implemented merely to provide som debugging and insight capabilities.  To
  replace values it is much better to call ``addTimer()/addHandle()`` again.
  It is important to point out, the tables returned by the ``index()``
  metamethod are just references and changing the values will infact change
  the executed function or parameter.

``int n = #loop         [__len]``
  Returns the numbers of slots in the loop currently provided as capacity.
