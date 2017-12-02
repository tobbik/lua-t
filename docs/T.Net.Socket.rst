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
objects and perform operations on it in one step. 


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
which is then the second parameter.  The first parameter is always the
message to be sent.  If the socket is an UDP socket the user is supposed to
specify an address to send the datagram to.  As a result the ``send()``
function inspects the second argument.  If it is a ``Net.Address`` it'll be
used to send the message to it.  Otherwise the second value will be
interpreted as how many bytes of the message should be send. So while it is
possible to skip arguments, the order is always the same:

1. message
2. t.Net.Address adr
3. max bytes of message to be sent

.. code:: lua

  -- valid invocations of send()
  sent_bytes_count = sck:send( message_string )
  sent_bytes_count = sck:send( message_string, length )
  sent_bytes_count = sck:send( message_string, address )
  sent_bytes_count = sck:send( message_string, address, length )

Like ``send()``, ``recv()`` will also accept various arguments which will
always have the order:

1. t.Net.Address
2. t.Buffer/t.Buffer.Segment
3. max bytes to receive

Argument scan be skipped, but the order must remain as outlined.


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

``Net.Socket sck, Net.Address a = Net.Socket.bind( Net.Address adr/[string host, int port] )``
  Creates TCP ``Net.Socket`` instance which is bound to the ``Net.Address
  adr`` or an address that is specified via ``string host:int port``.  The
  host is accepted as **aaa.bbb.ccc.ddd** IPv4 string or as **[::]** IPv6
  string.  If no ``Net.Address adr`` is given and no ``string host`` is
  specified the socket will be automaticaly bound to the default ``IP_ANY``
  interface.  If an ``Net.Address adr`` is specified, the return address
  ``a`` is a reference to ``adr`` not a new value.

  .. code:: lua

    -- meanings: adr    -> instances of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    sck,adr = Socket.bind(  )           -- Sck (TCP); Adr 0.0.0.0:(0)
    sck,adr = Socket.bind( host )       -- Sck (TCP); Adr host:(0)
    sck,adr = Socket.bind( port )       -- Sck (TCP); Adr 0.0.0.0:port
    sck,adr = Socket.bind( host, port ) -- Sck (TCP); Adr host:port
    sck,adr = Socket.bind( adr )        -- Sck (TCP)


:: _Net-Socket-listen:

``Net.Socket sck, Net.Address adr = Net.Socket.listen( [Net.Address adr]/[string host, int port, int backlog] )``
  Creates a TCP ``Net.Socket sck`` instance which is listening for
  connections on ``Net.Address adr`` or the address specified  via ``string
  host`` and ``int port`` number.  The host is accepted as **aaa.bbb.ccc.ddd**
  IPv4 string or as **[::]** IPv string.  If no ``Net.Address adr`` is given
  and no ``string host`` is specified the socket will be automaticaly bound
  to the default ``IP_ANY`` interface.  If an ``Net.Address adr`` is
  specified, the return address ``a`` is a reference to ``adr`` not a new
  value. ``int backlog`` is optional and defaults to SOMAXCONN.  Here are
  all permutations of using the listen() class function:

  .. code:: lua

    -- meanings:
    --           adr   -> instances of Net.Address
    --           sck   -> instance of Net.Socket
    --           xxxxx -> random port number choosen by the system
    --           bl    -> integer specifying the backlog
    --           port  -> integer specifying the port
    --           host  -> string specifying the IP address

    sck,adr = Socket.listen(  )               -- Sck (TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( bl )             -- Sck (TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( host )           -- Sck (TCP); Adr host:(0)
    sck,adr = Socket.listen( port, bl )       -- Sck (TCP); Adr 0.0.0.0:port
    sck,adr = Socket.listen( host, port )     -- Sck (TCP); Adr host:port
    sck,adr = Socket.listen( host, port, bl ) -- Sck (TCP); Adr host:port
    sck,adr = Socket.listen( adr )            -- Sck (TCP)
    sck,adr = Socket.listen( adr, bl )        -- Sck (TCP)

``Net.Socket sck, Net.Address adr = Net.Socket.connect( Net.Address adr/[string ip, int port] )``
  Creates a TCP ``Net.Socket sck`` instance which connected to a socket
  listening on ``Net.Address adr`` or the address specified  via ``string
  host`` and ``int port`` number.  The host is accepted as **aaa.bbb.ccc.ddd**
  IPv4 string or as **[::]** IPv string.  If no ``Net.Address adr`` is given
  and no ``string host`` is specified the socket will be automaticaly
  connected to the default ``localhost`` interface.  If an ``Net.Address
  adr`` is specified, the return address ``a`` is a reference to ``adr`` not
  a new value.

  .. code:: lua

    -- meanings: adr  -> instances of Net.Address
    --           sck  -> instance of Net.Socket
    --           port -> integer specifying the port
    --           host -> string specifying the IP address

    sck,adr = Socket.connect( adr )        -- Sck (TCP); ADR adr
    sck,adr = Socket.connect( port )       -- Sck (TCP); Adr 0.0.0.0:port
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

``Net.Address adr = Net.Socket sck:bind( [string host, int port ])``
  Creates and returns ``Net.Address adr`` instance defined by the
  ``string ip`` and ``int port`` number and binds the ``Net.Socket sck``
  instance to it.  ``string ip`` is accepted as **aaa.bbb.ccc.ddd**.  If
  ``ip string`` is omitted it will automatically bind to **0.0.0.0**, the
  IP_ANY interface.

``Net.Address adr = Net.Socket sck:bind( Net.Address adr/ [string host, int port] )``
  Binds ``Net.Socket sck`` instance to ``Net.Address adr`` or the address
  defined via ``string host`` and ``int port``.  The host is accepted as
  **aaa.bbb.ccc.ddd** IPv4 string or as **[::]** IPv string.  If no
  ``Net.Address adr`` is given and no ``string host`` is specified the
  socket will be automaticaly connected to the default ``IP_ANY`` interface.
  If an ``Net.Address adr`` is specified, the return address ``a`` is a
  reference to ``adr`` not a new value.

  .. code:: lua

    -- meanings:
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    adr  = sck.bind( )            -- bind to 0.0.0.0:0
    adr  = sck.bind( host )       -- Adr host:0
    adr  = sck.bind( host, port ) -- Adr host:port
    adr  = sck.bind( adr )        -- bind Adr

``Net.Address adr = Net.Socket sck:connect( Net.Address adr/[string ip, int port] )``
  Connects ``Net.Socket sck`` instance to socket listening on
  ``Net.Address adr`` or the address defined via ``string host`` and ``int
  port``.  The host is accepted as **aaa.bbb.ccc.ddd** IPv4 string or as
  **[::]** IPv6 string.  If no ``Net.Address adr`` is given and no ``string
  host`` is specified the socket will be automaticaly connected to the
  default ``localhost`` interface. If an ``Net.Address adr`` is specified,
  the return address ``a`` is a reference to ``adr`` not a new value.

  .. code:: lua

    -- meanings:
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    adr  = sck.connect( adr )        -- perform connect
    adr  = sck.connect( host )       -- Adr host:0
    adr  = sck.connect( host, port ) -- Adr host:port

``Net.Address adr = Net.Socket sck:listen( Net.Address adr/[string ip, int port], int backlog )``
  Makes ``Net.Socket sck`` listening for connections on ``Net.Address adr``
  or the address specified  via ``string host`` and ``int port`` number.
  The host is accepted as **aaa.bbb.ccc.ddd** IPv4 string or as **[::]**
  IPv6 string.  If no ``Net.Address adr`` is given and no ``string host``
  is specified the socket will be automaticaly bound to the default
  ``IP_ANY`` interface.  If an ``Net.Address adr`` is specified, the
  returned address ``a`` is a reference to ``adr`` not a new value.
  ``int backlog`` is optional and defaults to SOMAXCONN.  Here are
  all permutations of using the listen() instance method:

  .. code:: lua

    -- meanings:
    --           adr    -> instance of Net.Address
    --           sck    -> instance of Net.Socket
    --           xxxxx  -> random port number choosen by the system if
    --           bl     -> integer specifying the backlog
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address

    adr = sck:listen( )                -- just listen; assume bound socket
    adr = sck:listen( bl )             -- just listen; assume bound socket
    adr = sck:listen( adr )            -- perform bind and listen
    adr = sck:listen( adr, bl )        -- perform bind and listen
    adr = sck:listen( host )           -- Adr host:xxxxx
    adr = sck:listen( host, port )     -- Adr host:port
    adr = sck:listen( host, port, bl ) -- Adr host:port

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

  -- meanings:
  --           sck -> instance of Net.Socket
  --           adr -> instance of Net.Address
  --           buf -> instance of Buffer
  --           msg -> instance of Lua string, received payload
  --           len -> integer, len of received data in bytes
  --           max -> integer, max to read data in bytes

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

``boolean false, string errMsg = Net.Socket sck:recv( ... )``
  If ``recv()`` fails the first return value will evaluate to ``false``.  If
  a system err has occured the message will be in the secind return value.
  A return value of 0 bytes is returned as ``nil``, which also evaluates as
  ``false`` and that is usually indicative of the peer having the socket
  closed.  This is normal operation but can be detected via ``if sck:recv()
  then ...``.  In this case no error meassage is returned.


Overloaded send() method
........................

Like ``recv()``, the ``send()`` method can have three arguments:

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
  --           snt    -> integer, sent bytes
  --           max    -> integer, max to send data in bytes

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

``boolean false, string errMsg = Net.Socket sck:recv( ... )``
  If ``recv()`` fails the first return value will evaluate to ``false``.  If
  a system err has occured the message will be in the secind return value.
  A return value of 0 bytes is returned as ``nil``, which also evaluates as
  ``false`` and that is usually indicative of the peer having the socket
  closed.  This is normal operation but can be detected via ``if sck:recv()
  then ...``.  In this case no error meassage is returned.


Socket properties
.................

The availability of the following modes and/or their writablity is dependent
on the platforms implementation.  The majority of this documentation has
been taken from the Linux Manpages for the appropriate options.


Boolean Socket Options
''''''''''''''''''''''

``boolean b = sck.nonblock     [read/write] (O_NONBLOCK)``
  Socket blocking mode.

``boolean b = sck.broadcast    [read/write] (SO_BROADCAST)``
  Permits sending of broadcast messages, if this is supported by the
  protocol.

``boolean b = sck.debug        [read/write] (SO_DEBUG)``
  Turns on recording of debugging information.

``boolean b = sck.dontroute    [read/write] (SO_DONTROUTE)``
  Requests that outgoing messages bypass the standard routing facilities.

``boolean b = sck.keepalive    [read/write] (SO_KEEPALIVE)``
  Keeps connections active by enabling the periodic transmission of
  messages, if this is supported by the protocol.

``boolean b = sck.oobinline    [read/write] (SO_OOBINLINE)``
  Reports whether the socket leaves received out-of-band data (data marked
  urgent) in line.

``boolean b = sck.reuseaddr    [read/write] (SO_REUSEADDR)``
  Specifies that the rules used in validating addresses supplied to bind()
  should allow reuse of local addresses, if this is supported by the
  protocol.

``boolean b = sck.reuseport    [read/write] (SO_REUSEPORT)``
  Specifies that the rules used in validating addresses supplied to bind()
  should allow reuse of local addresses, if this is supported by the
  protocol.

``boolean b = sck.useloopback  [read/write] (SO_USELOOPBACK)``
  Directs the network layer (IP) of networking code to use the local
  loopback address when sending data from this socket. Use this option only
  when all data sent will also be received locally.

``boolean b = sck.nodelay      [read/write] (TCP_NODELAY)``
  This affects TCP sockets only!
  If set, disable the Nagle algorithm. This means that segments are always
  sent as soon as possible, even if there is only a small amount of data.
  When not set, data is buffered until there is a sufficient amount to send
  out, thereby avoiding the frequent sending of small packets, which results
  in poor utilization of the network. This option is overridden by TCP_CORK;
  however, setting this option forces an explicit flush of pending output,
  even if TCP_CORK is currently set.

``boolean b = sck.maxsegment   [read/write] (TCP_MAXSEG)``
  This affects TCP sockets only!
  The maximum segment size for outgoing TCP packets. In Linux 2.2 and
  earlier, and in Linux 2.6.28 and later, if this option is set before
  connection establishment, it also changes the MSS value announced to the
  other end in the initial packet. Values greater than the (eventual)
  interface MTU have no effect. TCP will also impose its minimum and
  maximum bounds over the value provided.


Integer Socket options
''''''''''''''''''''''

``int n = sck.descriptor       [readonly]``
  Returns the integer value of the system resource that was returned by the
  original socket() system call.  If the socket has been closed, returns
  ``nil``.

``int n = sck.error            [read/write] (SO_ERROR)``
  Reports information about error status and clears it.

``int n = sck.recvbuffer       [read/write] (SO_RCVBUF)``
  Receive buffer size information.

``int n = sck.recvlow          [read/write] (SO_RCVLOWAT)``
  Minimum number of bytes to process for socket input operations.

``int n = sck.sendbuffer       [read/write] (SO_SNDBUF)``
  Send buffer size information.

``int n = sck.sendlow          [read/write] (SO_SNDLOWAT)``
  Minimum number of bytes to process for socket output operations.


T.Time (struct timeval) Socket options
''''''''''''''''''''''''''''''''''''''

``t.Time t = sck.recvtimeout      [read/write] (SO_RCVTIMEO)``
  Timeout value that specifies the maximum amount of time an input function
  waits until it completes.

``t.Time t = sck.sendtimeout      [read/write] (SO_SNDTIMEO)``
  Timeout value specifying the amount of time that an output function blocks
  because flow control prevents data from being sent.


String Socket Options
''''''''''''''''''''''

``string str = sck.family"      [readonly ]``
  Sockets family type. (AF_INET, AF_INET6, ...).

``string str = sck.type"        [readonly ] (SO_TYPE)``
  Socket type. (STREAM, DGRAM, ...).

``string str = sck.protocol"    [readonly ] (SO_PROTOCOL)``
  Socket protocol. (TCP, UDP, ...).



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
