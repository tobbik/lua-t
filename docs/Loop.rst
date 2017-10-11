lua-t Loop - An Asynchronous Event Loop
+++++++++++++++++++++++++++++++++++++++


Overview
========

This provides an asynchronous eventloop that is intended to be platform
independent.  It is based on redis ae_* code base and basically provides a
bridge to Lua making it a native member of Lua.  The eventloop is build to
handle Socket handlers and ``T.Time`` objects which are basically ``struct
timeval`` wrappers.


Important File Handle and platform Caveats
==========================================

Based on the underlying eventloop implementation, it may be possible to had
it also file descriptors that have been created with Lua's ``io.open()``
method.  Not all implementations can hadle regular files.  For example, the
following example fails under Linux which uses the ``epoll`` implementation
by default:

  .. code:: lua

   Loop = require't.Loop'
   l = Loop(123)
   f = io.open('/etc/passwd', 'r')
   p = function(fl) print( fl:read('*all') ) end
   l:addHandle( f, 'r', p, f )

The error message say that the operation is not permitted but that is just
the kernel telling that the operation is not imported.  That is dependent on
the file type.  The following code for example works just fine:

  .. code:: lua

   Loop = require't.Loop'
   l = Loop(123)
   f = io.open('/dev/random', 'r')
   p = function(fl) print( fl:read(45) ); l:stop() end
   l:addHandle( f, 'r', p, f )
   l:run( )

Similarily, ``epoll`` does work for ``io.popen( )``:

  .. code:: lua

   Loop = require't.Loop'
   l = Loop(123)
   f = io.popen('date')
   p = function(fl) print( fl:read('*all') ); l:stop() end
   l:addHandle( f, 'r', p, f )
   l:run( )

So everything under Linux is a file, but not all files are created equal!
If the underlying implementation is based ion ``select()`` the kernel has no
problem adding a regular file like ``/etc/passwd`` to the event loop, but if
the eventloop triggers a readability/writability event there is no guarantee
that non-blocking read/write will actually succeed.  Under Windows, IOCP can
handle that fine.  ``T.Loop`` tries to abstract many things away but it does
not go as far a libuv for example.  Therefore, it will be possible to
implement a lot of useful stuff in ``T.Loop`` but there are some limitations
which are platform specific.  For more information read
`Asynchronous I/O in Windows for Unix Programmers
<http://tinyclouds.org/iocp-links.html>`_


Singleton
=========

While the interface suggests that there can be multiple ``t.Loop`` instances
created, running multiple instances in parallel is not defined.


API
===

Class Members
-------------

None


Class Metamembers
-----------------

``Loop l = Loop( int n )       [__call]``
  Creates ``Loop l`` instamnce.  The parameter ``int n`` describes how many
  descriptors can be handled in the event loop.  After adding that number of
  descriptors to the event loop will return false.  There are no limits,
  beyond memory and CPU performance, to adding Timers.

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
  in the Caveats.  the direction can be ``'r'`` or ``'w'`` determining if
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
  previously added function and parameters.
  ``function f`` *can have* a single return value.  If it is an instance of
  ``T.Time`` it will automatically reschedule itself with the same
  parameters.  This allows to implement intervals.

``boolean b = loop:removeTimer( t.Time t )``
  Remove ``t.Time t`` from the event loop.


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
