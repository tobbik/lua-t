lua-t t.Net.Socket - Sockets
++++++++++++++++++++++++++++


Overview
========

Net.Socket provides the ability to work with network connections.  It
allows most of the functionality known from other languages.  It focuses on
flexibility rather than on structure.  For example, any socket created has
an accept method even if that doesn't make sense for all types of socket
instances.


API
===

General API remarks
-------------------

The API strives to be both flexible and convenient.  For that reason some
methods are heavily overloaded.


Shortcut methods to create Sockets and Adresses
------------------------------------------------

The methods ``bind()``, ``connect()`` and ``listen()`` determine the
significance of an argument by type.  This allows to use them as methods on
an already instantiated object or as a class-level function that can create
objects and perform operations on it in one step.  The reason for the
flexibility is the underlying functions for bind, listen and connect that
are called on a class level or on an instance level(method) are the same.


Default values for ``Net.Socket()``, ``bind()``, ``connect()`` and ``listen()``
-------------------------------------------------------------------------------

If these functions are used to create instances of ``Net.Socket`` or
``Net.Address`` these will be created as the default family ``IPv4`` and the
default protocol ``TCP`` resulting, for sockets, in a ``SOCK_STREAM`` type.
It is possible to reset the ``Net.Socket.defaultFamily`` and the
``Net.Socket.defaultProtocol`` if this is desired for an application.  Note
that if the default value is changed the new default value will be valid for
**ALL** lua-t instances running in this application for the entire runtime.
This document assumes the default values ``AF_INET`` (IPv4), ``TCP``, and
``SOCK_STREAM``.


Overloaded send() and recv() methods
------------------------------------

Similarly send() and recv() act for all type of sockets.  If the socket is
unbound/unconnected it requires a ``Net.Address`` instance to be passed
which is then the first parameter.  Otherwise the first parameter is the
message to be sent.  If the socket is an UDP socket the user is supposed to
specify an address to send the datagram to.  As a result the ``send()``
function inspects the first argument.  If it is a ``Net.Address`` it'll be
used to send the message to it.  Otherwise the first value will be
interpreted as a message to be send.  All of the following arguments will
move up so that the following to function calls behave identically.

.. code:: lua

  sent_bytes_count = sck:send( address, message_string, length )
  sent_bytes_count = sck:send( message_string, length )


Class Members
-------------

``table r_rdy, table w_rdy = Net.Socket.select( table rds, table wrs )``
  Wrapper for the select() system call.  The `rds` argument holds all
  observable descriptors for read readiness.  Likewise the `wrs`` observes
  descriptors for write readiness.  Returns ``r_rdy`` table which contains
  all ``Net.Socket`` instances that reported read readiness via select() and
  ``w_rdy`` which reports back every ``Net.Socket`` ready to be written to.
  If a ``Net.Socket`` in the input table has a numeric key it will just be
  appended to ``*_rdy`` table,  if it uses a hash index it will be written
  by the hash to that table.

``Net.Socket sck, Net.Address a = Net.Socket.bind( [string ip,] int port )``
  Creates TCP ``Net.Socket`` instance which is bound to the ``Net.Address``
  defined via the ``ip`` string and ``port`` number.  ``ip string`` is
  accepted as **aaa.bbb.ccc.ddd**.  If ``string ip`` is omitted the it will
  automatically bind to **0.0.0.0**, the IP_ANY interface.

``Net.Socket sck = Net.Socket.bind( Net.Address adr )``
  Creates a TCP ``Net.Socket`` instance which is bound to ``Net.Address``.

  .. code:: lua

    -- meanings: adr    -> instances of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    sck,adr = Socket.bind(  )               -- Sck (TCP); Adr 0.0.0.0:(0)
    sck,adr = Socket.bind( host )           -- Sck (TCP); Adr host:(0)
    sck,adr = Socket.bind( host, port )     -- Sck (TCP); Adr host:port
    sck,_   = Socket.bind( adr )            -- Sck (TCP)

``Net.Socket sck, Net.Address adr = Net.Socket.listen( [string ip, int port, int backlog] )``
  Creates a TCP ``Net.Socket sck`` instance which is listening connections
  on the address requested via ``string ip`` and `int port` number.
  ``string ip`` is accepted as **aaa.bbb.ccc.ddd**.  If ``string ip`` is
  omitted it will automatically bind to **0.0.0.0**, the INADDR_ANY
  interface.

``Net.Socket sck = Net.Socket.listen( [Net.Address addr, int backlog] )``
  Creates an IPv4 TCP ``Net.Socket sck`` instance which is listening to the
  ``Net.Address  adr``. ``int backlog`` is optional and defaults to SOMAXCONN.
  Here are all permutations of using the listen() class function:

  .. code:: lua

    -- meanings: _     -> placeholders for nil
    --           adr   -> instances of Net.Address
    --           sck   -> instance of Net.Socket
    --           xxxxx -> random port number choosen by the system
    --           bl    -> integer specifying the backlog
    --           port  -> integer specifying the port
    --           host  -> string specifying the IP address

    sck,adr = Socket.listen(  )               -- Sck (TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( bl )             -- Sck (TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( host )           -- Sck (TCP); Adr host:(0)
    sck,adr = Socket.listen( host, port )     -- Sck (TCP); Adr host:port
    sck,adr = Socket.listen( host, port, bl ) -- Sck (TCP); Adr host:port
    sck,_   = Socket.listen( adr )            -- Sck (TCP)
    sck,_   = Socket.listen( adr, bl )        -- Sck (TCP)

``Net.Socket sck, Net.Address adr = Net.Socket.connect( [string ip, int port] )``
  Creates an TCP ``Net.Socket`` instance which is connected to the address
  requested via the ``ip`` string and ``port`` number.  ``string ip`` is
  accepted as **aaa.bbb.ccc.ddd**.  If ``string ip`` is omitted the it will
  automatically connect to **127.0.0.1**, the ``localhost``

``Net.Socket sck = Net.Socket.connect( Net.Address adr )``
  Creates an TCP ``Net.Socket`` instance which is connected to the
  ``Net.Address``.

  .. code:: lua

    -- meanings: ip,adr -> instances of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    sck,_   = Socket.connect( ip )         -- Sck (TCP)
    sck,adr = Socket.connect( host, port ) -- Sck (TCP); Adr host:port


Class Metamembers
-----------------

Net.Socket has no clone constructor because sockets are system resources
which can't be duplicated.

``Net.Socket sck = Net.Socket( [string protocol, string family, string type] )   [__call]``
  Instantiate new ``Net.Socket sck`` object.  If no arguments are passed it
  will assume ``TCP`` and ``Ipv4`` as default values.  If only one argument
  is passed it will be interpreted as ``string protocol`` and `ip4` is
  assumed as default family.  Type can be ``stream``, ``datagram`` or
  ``raw`` or any of the ``C`` based identifiers such as ``SOCK_DCCP`` and
  others.  If not passed it will be inferred from the protocol.


Instance Members
----------------

``Net.Address adr = Net.Socket sck:bind( [string ip, int port ])``
  Creates and returns ``Net.Address adr`` instance defined by the
  ``string ip`` and ``int port`` number and binds the ``Net.Socket sck``
  instance to it.  ``string ip`` is accepted as **aaa.bbb.ccc.ddd**.  If
  ``ip string`` is omitted it will automatically bind to **0.0.0.0**, the
  IP_ANY interface.

``Net.Socket sck:bind( Net.Address addr )``
  Binds ``Net.Socket sck`` instance to ``Net.Address adr``.

  .. code:: lua

    -- meanings: _      -> placeholder for nil
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    adr,_   = sck.bind( )            -- bind to 0.0.0.0:0
    adr,_   = sck.bind( host )       -- Adr host:0
    adr,_   = sck.bind( host, port ) -- Adr host:port
    _,_     = sck.bind( adr )        -- bind Adr

``Net.Address addr = Net.Socket sck:connect( [string ip,] int port )``
  Creates and returns ``Net.Address adr`` instance defined by ``string ip``
  and ``int port`` number and connects the ``Net.Socket  sck`` instance to
  it.  ``string ip`` is accepted as **aaa.bbb.ccc.ddd**.  If ``string ip``
  is omitted it will automatically connect to **127.0.0.1**, the
  ``localhost`` interface. In this case the port is the only argument which
  is mandatory.

``Net.Socket sck:connect( Net.Address addr )``
  Connects the ``Net.Socket`` instance to the ``Net.Address``.

  .. code:: lua

    -- meanings: _      -> placeholder for nil
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    _,_    = sck.connect( adr )        -- perform bind and listen
    adr,_  = sck.connect( host, port ) -- Adr host:port

``Net.Address addr = Net.Socket sck:listen( [string ip,] int port, int backlog )``
  Creates and returns ``Net.Address adr`` instance defined by the ``string
  ip`` string and ``int port`` number and make the ``Net.Scoket sck``
  instance listen on it.  ``ip string`` is accepted as **aaa.bbb.ccc.ddd**.
  If ``string ip`` is omitted it will automatically listen on **0.0.0.0**,
  the IP_ANY interface.  In this case the port is the only argument which is
  mandatory.  Backlog defaults to SOMAXCONN.

``Net.Socket sck:listen( Net.Address addr, int backlog )``
  Makes the ``Net.Socket sck`` instance listen on ``Net.Address adr``.
  Backlog defaults to SOMAXCONN.

  .. code:: lua

    -- meanings: _      -> placeholder for nil
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           xxxxx  -> random port number choosen by the system if
    --           bl     -> integer specifying the backlog
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    _,_    = sck:listen( )                -- just listen; assume bound socket
    _,_    = sck:listen( bl )             -- just listen; assume bound socket
    _,_    = sck:listen( ip )             -- perform bind and listen
    _,_    = sck:listen( adr, bl )        -- perform bind and listen
    adr,_  = sck:listen( host )           -- Adr host:xxxxx
    adr,_  = sck:listen( host, port )     -- Adr host:port
    adr,_  = sck:listen( host, port, bl ) -- Adr host:port

``Net.Socket client, NetAddress addr = Net.Socket sck:accept( )``
  Accepts a new connection the ``Net.Socket`` instance.  Returns 
  ``Net.Address`` client instance and the clients ``Net.Address``
  instance.


Overloaded recv() method
........................

``recv()`` can have three arguments:

``Net.Address adr``
  ``recv( adr )`` will write the peers address into the ``Net.Address adr``
  instance.  This is useful for datagram(UDP) sockets.

``Buffer/Buffer.Segment buf``
  Instead of returning the payload as a Lua string it will get written into
  ``Buffer buf``.  The call to ``recv()`` will return a boolean instead of
  Lua string indicating weather or not the call was successful.

``int max``
  Limits the maximum number of received bytes for the call to ``recv()``.
  If no ``Buffer/Segment buf`` is passed it defaults to a maximum of
  ``BUFSIZ``.  A value greater than ``BUFSIZ`` will throw an error.  If a
  ``Buffer/Segment buf`` is passed, the length of ``buf`` determines the
  maximum number of bytes received by the call.  ``int max`` does not
  guarantee the number of received bytes, it only *allows* the OS to receive
  that many.  The actual number of received bytes is determined by the way
  the OS handles it.


The three possible arguments to ``recv()`` **must always** be in the order
of: ``recv( Net.Address adr, Buffer/Segment buf, int max )``.  Non of the
arguments is mandatory.  All of the following permutations for ``recv()``
are valid:

.. code:: lua

  -- meanings: _      -> placeholder for nil
  --           sck    -> instance of Net.Socket
  --           adr    -> instance of Net.Address
  --           buf    -> instance of Buffer
  --           msg    -> instance of Lua string, received payload
  --           len    -> Lua integer, len of received data in bytes
  --           max    -> Lua integer, max to read data in bytes

  msg, len  = sck:recv( adr, buf, max )
  msg, len  = sck:recv( adr, buf )
  msg, len  = sck:recv( adr, max )
  msg, len  = sck:recv( buf, max )
  msg, len  = sck:recv( adr )
  msg, len  = sck:recv( buf )
  msg, len  = sck:recv( max )
  msg, len  = sck:recv( )

The following explains what each argument means.

``string msg, int len = Net.Socket sck:recv( Net.Address adr )``
  Receives data from the ``Net.Socket`` instance.  Returns ``msg`` as the
  payload received or ``nil`` if nothing was received.  ``int len`` contains
  the length of ``string msg`` in bytes or 0 if ``msg`` is nil.  ``adr``
  will be used to determine where the message will be received from, which
  is important for datagram(UDP) sockets.  If the ``Net.Socket sck``
  instance is already bound the ``adr`` argument has no impact.


``boolean msg, int len = Net.Socket sck:recv( Buffer/Segment buf )``
  Receives data from the ``Net.Socket`` instance.  Returns ``boolean msg``
  if the ``recv()`` operation was successful.  The received payload will be
  written into the ``Buffer/Segment buf`` object.  The call to ``recv()`` it
  gets automatically limited to a maximum number of bytes equal to the
  length ``#buf`` instance.


``boolean msg, int len = Net.Socket sck:recv( int max )``
  Receives up to ``int max`` bytes from ``Net.Socket sck``.  If both ``int
  max`` and ``Buffer/Segment buf`` are omitted it will default to systems
  ``BUFSIZ``.  If ``int max`` passed as argument is either greater than the
  length of ``Buffer/Segment buf`` or the length or ``BUFSIZ`` ``recv()``
  will throw an error.


Overloaded send() method
........................


Like ``recv()`` the ``send()`` method can have three arguments:

``Buffer/Buffer.Segment/string msg``
  This is the only mandatory argument to ``send()``.  It holds the payload
  of data to be send through the ``Net.Socket``.  This can have three
  formats: a ``t.Buffer``, a ``t.Buffer.Segment`` or a standard Lua
  ``string``.

``Net.Address adr``
  ``send( msg, adr )`` will send the payload ``msg`` payload to the
  ``Net.Address adr``.  This is needed if ``Net.Socket sck`` had not been
  previously used ``sck:connect( Net.Address adr)`` to be in a connected
  state.  If the ``Net.Socket sck`` instance is not connected and no
  ``Net.Address adr`` argument is given ``send()`` will fail with a missing
  destinatuion error message.  The ``Net.Address adr`` argument is usually
  used on ``SOCK_DGRAM`` sockets aka. UDP.

``int max``
  Limits the maximum number of bytes sent out.  If ``int max`` is smaller
  that the length of the ``msg`` argument only ``int max`` bytes wuill be
  sent out.  If ``msg`` is actually shorter than ``int max`` the max
  argument is ignored.  Like in any network API really, passing ``int max``
  is no guarantee about the amount of bytes actually sent out.  It's the
  programmers duty to check the umber of sent bytes.


The three possible arguments to ``send()`` **must always** be in the order
of: ``send( Buffer/Buffer.Segment/string msg, Net.Address adr, int max )``.
Only the first argument ``msg`` is mandatory.  All of the following
permutations for ``recv()`` are valid:

.. code:: lua

  -- meanings: sck    -> instance of Net.Socket
  --           adr    -> instance of Net.Address
  --           msg    -> instance of Buffer or Buffer.Segment or Lua string
  --           snt    -> Lua integer, sent bytes
  --           max    -> Lua integer, max to send data in bytes

  snt  = sck:send( msg )           -- send msg on a connected socket
  snt  = sck:send( msg, adr )      -- send msg on unconnected socket to adr
  snt  = sck:send( msg, max )      -- send max bytes of msg on a connected socket
  snt  = sck:send( msg, adr, max ) -- send max bytes of msg on a unconnected socket to adr

The three possible arguments to ``send()`` **must always** be given in the
order of: ``Net.Address addr, Buffer/Segment buf/LuaString msg, int max``.
The ``buf/msg`` argument is mandatory.  Each of the other arguments are
optional.

The following explains what each argument means.

``int sent = Net.Socket sck:sent( Buffer/string msg[, Net.Address adr] )``
  Send data to the ``Net.Socket adr`` instance if the socket is not already
  connected.  Returns ``int sent`` determining how many bytes got send.
  Returns ``nil`` if nothing was sent.

``int sent = Net.Socket sck:sent( Buffer/string msg[, int max] )``
  Send ``int max`` bytes of ``msg`` through the socket.  If the length of
  ``msg`` is shorter than ``int max`` the parameter is ignored.

``int sent = Net.Socket sck:sent( Buffer/Segment/string msg )``
  ``msg`` defines the payload to be sent through the socket.  It can be an
  instace of ``Buffer``, ``Buffer.Segment`` or a Lua stirng.


Instance Metamembers
--------------------

``string s = tostring( Net.Socket sck )  [__tostring]``
  Returns a string representing the Net.Socket instance.  The String
  contains type, Socket handle number and memory address information such as
  ``*t.Net.Socket[TCP,3]: 0xdac2e8*``, meaning it is a TCP Socket with socket
  handle number 3.

``Net.Socket sck = nil  [__gc]``
  Garbage collector makes sure the socket closes and gets properly disposed
  of when garbage collection is performed.
