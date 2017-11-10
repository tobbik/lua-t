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
   l = Loop( 123 )


Modes
=====

``T.Loop`` has 3 modes that determine the behaviour of the memory management
in the loop.  Internally, similar to the redis ae_* loop, it uses an array
to allow for access to the event handlers based on file descriptors.  That
has been implemented as a C-array rather than a Lua table for reasons of
speed and code simplicity.  Anytime a new file/socket descriptor gets added
to the Loop via ``ael:addHandle( ... )`` there must be enough space in that
array to keep track of the event handler attached via ``addhandle()``.  For
that, ``T.Loop`` has three modes that get determined via the invocation of
the constructor:

Fixed number of descriptors
---------------------------

If the constructor is invoked via ``ael = Loop( int n )`` the loop will have
a fixed number of slots for file descriptors.  Consequently, a call to
``ael:addhandle( sock, ...)`` with a socket where the value of
``sock.descriptor`` is bigger than the slot capacity of ``Loop ael`` will
return ``false``.  No error will be thrown.  The situation can be handled
manually by calling ``ael:resize( newSize )``.  For reasons of efficiency
and simplicity, ``int n`` must be at least 4.

Automatic increase of descriptor slots
--------------------------------------

If the constructor is invoked via ``ael = Loop( bool false )`` the loop will
automatically increase the nuber of slots as needed.  Calling ``ael =
Loop()`` will also invoke the same behaviour .Consequently, ``ael:addhandle(
sock, ... )`` will never return false.  However, if descriptors get removed
from the loop, the added slots will **NOT** be freed up and the space
remains allocated in the loop.  This situation can be handled with strategic
calls to ``ael:resize( )`` (without an argument) which automatically trims
the slots to the maximal number needed.

Automatic increase AND decrease of descriptor slots
---------------------------------------------------

If the constructor is invoked via ``ael = Loop( bool true )`` the loop will
automatically increase the nuber of slots as needed.  And after each round
of processing events the loop gets tested if the highest descriptor allows
for the loop to be shrunk.  This scenario does sound most convienient but it
**MAY** have undesirable side effects.  Since it is implemented using
``realloc( ... )``, depending on reallocs implementation on your platform
this can create memory thrashing.  It comes down to testing your code and
looking for memory behaviour.  In many cases it is just fine because the
memory used is really small and mostly ``realloc()`` is decent on reusing
memory.


Important File Handle and platform Caveats
==========================================

Based on the underlying eventloop implementation, it may be possible to had
it also file descriptors that have been created with Lua's ``io.open()``
method.  Not all implementations can hadle regular files.  For example, the
following example fails under Linux which uses the ``epoll`` implementation
by default:

  .. code:: lua

   l = Loop(123)
   f = io.open('/etc/passwd', 'r')
   p = function(fl) print( fl:read('*all') ) end
   l:addHandle( f, 'r', p, f )
   -- stdin:1: Error adding descriptor to set (Operation not permitted)

The error message says that the operation is not permitted but that is just
the kernel telling that the operation is really not supported.  That
behaviour is dependent on the file type and implementation.  The following
code for example works just fine with ``epoll`` becuase ``//dev/random`` is
more than just a file:

  .. code:: lua

   l = Loop(123)
   f = io.open('/dev/random', 'r')
   p = function(fl) print( fl:read(45) ); l:stop() end
   l:addHandle( f, 'r', p, f )
   l:run( )

Similarily, ``epoll`` does work for ``io.popen( )``:

  .. code:: lua

   l = Loop(123)
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
are some limitations which are platform specific.  For more information read
`Asynchronous I/O in Windows for Unix Programmers
<http://tinyclouds.org/iocp-links.html>`_


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

``Loop l = Loop( int n )       [__call]``
  Creates ``Loop l`` instance.  The parameter ``int n`` describes how many
  descriptors can be handled in the event loop.  Adding descriptors with a
  higher number than ``int n`` to the loop will return false.  the minimum
  value for ``int n`` is 4.  This is for reasons of effciency and
  simplicity.  There are no limits, beyond memory and CPU performance, to
  adding Timers.

``Loop l = Loop( bool x )       [__call]``
  Creates ``Loop l`` instance.  The parameter ``bool x`` determines for
  automatic or semi-automatic handling of descriptor slots.  If ``x==false``
  the loop willl add slots to it's capacity as needed but it will not
  automatically decrease it.  If ``x==true`` the loop wikk full
  automatically handle adding AND removing slots but that may have impatcs
  on memory thrashing and/or performance of the loop.


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
  *`t.Loop{123:5}: 0xdac2e8`*, meaning it has capacity for 123 descriptors
  and 5 is the highest file descriptor number.

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
