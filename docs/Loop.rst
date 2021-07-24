lua-t Loop - An Asynchronous Event Loop
+++++++++++++++++++++++++++++++++++++++


Overview
========

This provides an asynchronous eventloop that is intended to be platform
independent.  It is based on redis ae_* code base and basically provides a
bridge to Lua making it a native member of Lua.  The eventloop is build to
handle Socket handlers and Task objects.  General invokation looks like:

  .. code:: lua

   Loop = require't.Loop'
   l = Loop( )

Technologies like luvit or NodeJs utilize similar ideas and underlying
technologies to handle io.  By including the loop in the interpreter they
can hide a lot of implementation detail from the user.  As a side effect of
tying the loop and the interpreter tightly together the software is fixed to
the version of the interpeter which creates the need for their own
eco-system.  Now, both luvit and node, have a great eco-system and npm is a
fantastic package manager, but in Lua land it remains desirable to just
choose and pick the lates official Lua distribution from PUC.  Therefore,
lua-t makes an effort to be 'just another library' that can be loaded and
used in conjunction with all the other software that is available either
via the distribution or lua-rocks.  Overall, the implementation of
``T.Loop`` is more comparable to Pythons asyncio rather than to luvit or
nodejs.


Implementation
==============

``t.Loop`` is very loosely based on the ae_* eventloop found in redis.
General architecture is similar as that it has a general part which handles
the common code and then implementation/platform specific code which handles
the abstraction regarding the use of the underlaying technology.  While not
fully implemented yet, the following platforms are planned to be supported:

  - ``select()`` platform independent with known select() limitations
  - ``epoll()``  default on Linux systems, and probably best tested
  - ``kqueue()`` default on OS X and all \*Bsd platforms
  - ``evport()`` Solaris
  - ``cpio``     Windows (inspiration from Microsoft redis port)

Only ``epoll()`` and ``select()`` are currently implemented.  As soon as I
get my hands on a Mac I will implement ``kqueue``.  Where ``t.Loop`` differs
significantly from redis is the handling of slots for descriptors to be
watched.  In redis, the user is responsible to not add a file/socket
descriptor to the loop which has a higher descriptor number then the slot
capacity of the loop and the user must resize the loop (which uses
realloc()) manually because the slots are managed as a fixed size array.
``t.Loop`` uses a Lua table instead which is garbage collected by Lua
itself.  There are some performance implications but in the interest of
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
   event-handlers, so it does not have to be manually resized


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

Static Class Members
--------------------

``void = t.Loop:sleep(int ms)``
  Makes process sleep for ``int ms`` milliseconds.  This is a busy wait that
  will also stall other coroutines.

``int ms = t.Loop:time()``
  Returns the milliseconds since epoch.  It has the same functionality as
  ``os.time`` but the resolution is in milliseconds instead of seconds.


Class Metamembers
-----------------

``Loop l = t.Loop( )       [__call]``
  Creates ``Loop l`` instance.  Create only one per application.  Using
  multiple loops is not defined as behaviour.


Instance Members
----------------

``string s = loop:show()    -- only available when compiled with DEBUGsupport``
  Print a list of elements in the loop in a pre-formatted way.

  .. code::

   T.Loop[1000]: 0x55fc3e7615f8 TIMER LIST:
     1  { 1000ms}  function 1000
     2  { 2000ms}  function 2000
     3  { 3000ms}  function 3000
     4  { 3500ms}  function 3500 `foo` `bar`
     5  { 4000ms}  function 4000
   T.Loop{3} 0x55fc3e7615f8 HANDLE LIST:
     4  [R]  function `a string` `a` `b` `c`
     5  [W]  function T.Net.Socket `Message to be sent`
     5  [R]  function T.Net.Socket

``void = loop:sleep(int ms)``
  Makes process sleep for ``int ms`` milliseconds.  This is a busy wait that
  will also stall other coroutines.

``int ms = loop:time()``
  Returns the milliseconds since epoch.  It has the same functionality as
  ``os.time`` but the resolution is in milliseconds instead of seconds.

``void = loop:run()``
  Starts the event loop.  It either runs until ``loop:stop()`` is called, or
  until no more tasks or event handlers are left on the loop.

``void = loop:stop()``
  Stops the event loop and returns to the normal flow of execution.

``void = loop:clean()``
  Removes all events and tasks from the loop which in turn also will make
  the loop stop itself.

``T.Loop.Node = loop:addHandle( handle h, string dir, function f, [...])``
  Add the ``handle h`` to the eventloop and define what should be executed
  when an event on the handle is observed.  The ``handle h`` can be a
  ``t.Net.Socket`` or a ``Lua File``.  Limitations apply as explained above
  in the caveats.  The direction can be ``'r'`` or ``'w'`` determining if
  the event would indicate readability or writablity.  For more clarity the
  following are also supported: ``rd``, ``read``, ``wr`` and ``write``.
  Upon the triggered event the ``function f`` will be executed with the
  parameters passed in ``...``.  ``addHandle()`` is idempotent and each call
  to it will **replace** the previously added function and parameters.  The
  returned ``T.Loop.Node n`` is a piece of userdata that can have three
  uservalues attached to it:

   - uservalue index 1:  A table with function and arguments for read
     operation
   - uservalue index 2:  A table with function and arguments for write
     operation
   - uservalue index 3:  Either the ``T.Net.Socket`` or ``Lua File`` object
     that gets observed

  These uservalues are used for the loops implementation but are exposed for
  convienience and debugging purposes.

``T.Loop.Node n = loop:removeHandle( handle h, string dir )``
  Remove observing events on the ``handle h`` for the direction ``string
  dir`` from the event loop.  For simplicity, ``removehandle`` also supports
  ``rw``, ``rdwr`` or ``readwrite`` as ``string dir`` which removes the
  handle for both directions.


``T.Loop.Task t = loop:addTask( integer ms, function f, ...)``
  Add the ``t.Time t`` to the eventloop and define what should be executed
  when then ``t.Time t`` value has passed  Upon the triggered event the
  ``function f`` will be executed with the parameters passed in ``...``.
  ``addTimer()`` is idempotent and each call to it will **replace** the
  previously added function and parameters.  ``function f`` *can have* a
  single return value.  If it is an integer ``int ms`` greater than 1, the
  task will automatically reschedule itself in ``int ms`` milliseconds with
  the same parameters.  This allows to flexibly implement intervals.
  ``T.Loop.Task t`` is a userdata representing the tasks internal
  implementation.  It has two uservalues assoiciated with it:

   - uservalue index 1:  The next ``T.Loop Task`` in line.  It's implemented
     as a linked list based on uservalues.
   - uservalue index 2:  A table with function and arguments executed when
     the task fires.

  These uservalues are used for the loops implementation but are exposed for
  convienience and debugging purposes.

``boolean b = loop:cancelTask( t.Loop.Task )``
  Remove ``t.Loop.Task t`` from the event loop.


Instance Metamembers
--------------------

``string s = tostring( Loop l )  [__tostring]``
  Returns a string representing the ``Loop l`` instance.  The string
  contains type, length and memory address information such as
  *`t.Loop{7}: 0xdac2e8`*, meaning it is currently observing 7 descriptors.

``t.Loop.Node n = Loop l[ idx ] [__index]``
  Returns a ``t.Loop.Node`` instance.  The index must be or a valid ``Lua
  File`` or ``t.Net.Socket`` handle.  The returned node is the same
  reference as the ``loo:addHandle()`` method would return.

  There is no ``__newindex()`` method since ``__index()`` has been
  implemented merely to provide som debugging and insight capabilities.
  Use ``loop:addtask()`` and ``loop:canceltask()`` instead.

``int n = #loop         [__len]``
  Returns the numbers of file or socket handles in the loop currently
  observed.
