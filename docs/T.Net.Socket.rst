lua-t T.Net.Socket - Sockets
++++++++++++++++++++++++++++


Overview
========

T.Net.Socket provides the ability to work with network connections.  It
allows most of the functionality known from other languages.  It focuses on
flexibility rather than on structure.  So, any socket created has an accept
method even if that doesn't make sense for all types of socket instances.


API
===

General API remarks
-------------------

The API strives to be both flexible and convienient.  For that reason some
methods are heavily overloaded.


Shortcut methods to create Sockets and Adresses
------------------------------------------------

The methods ``bind()``, ``connect()`` and ``listen()`` determine the
significance of an argument by type.  This allows to use them as methods on
an already instantiated object or as a classlevelfunction that can create
objects and perform operations on it in one step.  The reason for the
flexibility is the underlying functions for bind, listen and connect that
are called on a class level or on an instance level(method) are the same.


Overloaded send() and recv() methods
------------------------------------

Similarly send() and recv() act for all type of sockets.  In the socket is
unbound it requieres a ``T.Net.Address`` instance to be passed which is then
the first parameter.  Otherwise the first parameter is the message to be
sent.
of UDP the user is supposed to specify an address to send the datagram to.
As a result the `send()` function inspects the first argument.  If it is a
``T.Net.Address`` it'll be used to send the message to it. If it's ``nil``
it will be disregarded.  Any other value will be interpreted as a message to
be send.  All of the following arguments will move up so that the following
to functioncalls behave identically.

.. code:: lua

  sent_bytes_count = sck:send( nil, message_string, offset, length )
  sent_bytes_count = sck:send( message_string, offset, length )


Class Members
-------------

``table r_rdy, table w_rdy = T.Net.Socket.select( table rds, table t_wrs )``
  Wrapper for the select() system call.  The ``rds`` argument holds all
  observable descriptors for read readiness.  Likewise the ``wrs`` observes
  descriptors for write readiness.  Returns ``r_rdy`` table which contains
  all ``T.Net.Socket`` instances that reported read readiness via select()
  and ``w_rdy`` which reports back every ``T.Net.Socket`` ready to be
  written to.  If a ``T.Net.Socket`` in the input table has a numeric key
  it will just be appended to ``*_rdy`` table,  if it uses a hash index it
  will be written by the hash to that table.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.bind( [string ip,] int port )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is bound to the
  address requested via the ``ip`` string and ``port`` number. Ip string
  is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted
  the it will automatically bind to **0.0.0.0**, the IP_ANY interface.

``T.Net.Socket sck = T.Net.Socket.bind( T.Net.Address addr )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is bound to the
  T.Net.Address.

  .. code:: lua
  
    -- meanings: ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    sck,adr = Socket.bind(  )               -- Sck IPv4(TCP); Adr 0.0.0.0:(0)
    sck,adr = Socket.bind( host )           -- Sck IPv4(TCP); Adr host:(0)
    sck,adr = Socket.bind( host, port )     -- Sck IPv4(TCP); Adr host:port
    sck,_   = Socket.bind( ip )             -- Sck IPv4(TCP)

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.listen( [string ip, int port, int backlog] )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is listening
  connections on the address requested via the ``ip`` string and ``port``
  number. Ip string is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is
  omitted it will automatically bind to **0.0.0.0**, the INADDR_ANY interface.

``T.Net.Socket sck = T.Net.Socket.listen( [T.Net.Address addr, int backlog] )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is listening to the
  T.Net.Address.  ``backlog`` is optional and defaults to 5.  
  ``backlog`` is optional and defaults to SOMAXCONN.  Here are all
  permutations of using the listen() class function:

  .. code:: lua
  
    -- meanings: _, __  -> placeholders for nil
    --           ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           xxxxx  -> random port number choosen by the system if
    --                     T.Net.Address didn't have a port specified before
    --                     listen()
    --           bl     -> integer specifying the backlog
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    sck,adr = Socket.listen(  )               -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( bl )             -- Sck IPv4(TCP); Adr 0.0.0.0:xxxxx
    sck,adr = Socket.listen( host )           -- Sck IPv4(TCP); Adr host:(0)
    sck,adr = Socket.listen( host, port )     -- Sck IPv4(TCP); Adr host:port
    sck,adr = Socket.listen( host, port, bl ) -- Sck IPv4(TCP); Adr host:port
    sck,_   = Socket.listen( ip )             -- Sck IPv4(TCP)
    sck,_   = Socket.listen( ip, bl )         -- Sck IPv4(TCP)

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.connect( [string ip, int port] )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is connected to the
  address requested via the ``ip`` string and ``port`` number.  Ip string
  is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted
  the it will automatically connect to **127.0.0.1**, the `localhost`

``T.Net.Socket sck = T.Net.Socket.connect( T.Net.Address addr )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is connected to the
  T.Net.Address.

  .. code:: lua
  
    -- meanings: ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    sck,_   = Socket.connect( ip )         -- Sck IPv4(TCP)
    sck,adr = Socket.connect( host, port ) -- Sck IPv4(TCP); Adr host:port


Class Metamembers
-----------------

T.Net.Socket has no clone constructor because sockets are system resources
which can't be duplicated.

``T.Net.Socket sck = T.Net.Socket( [string protocol, string family, string type] )   [__call]``
  Instantiate a new T.Net.Socket object.  If no arguments are passed it will
  assume *TCP* and *ip4* as default values.  If only one argument is passed
  it will be interpreted as ``protocol`` and *ip4* is assumed as default
  family.  Type can be ``stream``, ``datagram`` or ``raw`` or any of the `C`
  based identifiers such as ``SOCK_DCCP`` and others.  If not passed it will
  be infered from the protocol.


Instance Members
----------------

``T.Net.Address addr = T.Net.Socket sck:bind( [string ip, int port ])``
  Creates and returns an ``T.Net.Address`` instance defined by the ``ip``
  string and ``port`` number and binds the ``sck`` instance to it.  Ip string
  is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted it will
  automatically bind to **0.0.0.0**, the IP_ANY interface.

``T.Net.Socket sck:bind( T.Net.Address addr )``
  Binds the ``T.Net.Socket`` instance to the ``T.Net.Address``.

  .. code:: lua
    
    -- meanings: _, __  -> placeholders for nil
    --           ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    adr,_   = sck.bind( )            -- bind to 0.0.0.0:0
    _,__    = sck.bind( ip )         -- bind Adr
    adr,__  = sck.bind( host )       -- Adr host:0
    adr,__  = sck.bind( host, port ) -- Adr host:port

``T.Net.Address addr = T.Net.Socket sck:connect( [string ip,] int port )``
  Creates and returns an ``T.Net.Address`` instance defined by the ``ip``
  string and ``port`` number and connects the ``sck`` instance to it.  Ip
  string is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted it
  will automatically connect to **127.0.0.1**, the `localhost` interface.
  In this case the port is the only argument which is mandatory.

``T.Net.Socket sck:connect( T.Net.Address addr )``
  Connects the ``T.Net.Socket`` instance to the ``T.Net.Address``.

  .. code:: lua
    
    -- meanings: _, __  -> placeholders for nil
    --           ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    _,__    = sck.connect( ip )         -- perform bind and listen
    adr,__  = sck.connect( host, port ) -- Adr host:port

``T.Net.Address addr = T.Net.Socket sck:listen( [string ip,] int port, int backlog )``
  Creates and returns an ``T.Net.Address`` instance defined by the ``ip``
  string and ``port`` number and make the ``sck`` instance listen on it.  Ip
  string is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted it
  will automatically listen on **0.0.0.0**, the IP_ANY interface.  In this
  case the port is the only argument which is mandatory.  Backlog defaults
  to 5.

``T.Net.Socket sck:listen( T.Net.Address addr, int backlog )``
  Makes the ``T.Net.Socket`` instance to the ``T.Net.Address``.  Backlog
  defaults to 5.

  .. code:: lua
    
    -- meanings: _, __  -> placeholders for nil
    --           ip,adr -> instances of T.Net.Address
    --           sck    -> instance of T.Net.Socket
    --           xxxxx  -> random port number choosen by the system if
    --                     T.Net.Address didn't have a port specified before
    --                     listen()
    --           bl     -> integer specifying the backlog
    --           port   -> integer specifying the port
    --           host   -> string specifying the IP address
    
    _,__    = Socket.listen( sck )                 -- just listen; assume bound socket
    _,__    = Socket.listen( sck, bl )             -- just listen; assume bound socket
    _,__    = Socket.listen( sck, ip )             -- perform bind and listen
    _,__    = Socket.listen( sck, ip, bl )         -- perform bind and listen
    adr,__  = Socket.listen( sck, host )           -- Adr host:xxxxx
    adr,__  = Socket.listen( sck, host, port )     -- Adr host:port
    adr,__  = Socket.listen( sck, host, port, bl ) -- Adr host:port

``T.Net.Socket client, T.NetAddress addr = T.Net.Socket sck:accept( )``
  Accepts a new connection the ``T.Net.Socket`` instance.  Returns the
  ``T.Net.Address`` client instance and the clients ``T.Net.Address``
  instance.


Overloaded recv() method
........................

The three possible arguments to ``recv()`` **must always** be in the order
of: ``T.Net.Address addr, T.Buffer/Segment buf/LuaString str, int max``.
Only the buf/str argument is mandatory. 

``string msg, int rcvd, T.NetAddress addr = T.Net.Socket sck:recv( int max )``
  Receives data from the ``T.Net.Socket`` instance.  Returns the
  ``T.Net.Address`` client instance and the clients ``T.Net.Address``
  instance.  ``msg`` contains the payload recieved.  ``max`` limits the
  amount of data received at once.  If no is passed the maximum of `BUF_LEN`
  is used.  Values bigger than `BUF_LEN` are an error.

``string msg, int rcvd = T.Net.Socket sck:recv( T.NetAddress addr )``
  Writes the ``T.Net.Address`` information of the peer into the instance
  passed into as parameter.

``bool rcvd, int cnt = T.Net.Socket sck:recv( T.Buffer/Segment buf )``
  Write the recieved payload into the ``T.Buffer/Segment`` instance instead
  of allocating a new string.  The call to ``rcvd()`` is automatically sized
  to ``#buf``.

``bool rcvd, int cnt = T.Net.Socket sck:recv( T.Net.Address addr, T.Buffer/Segment buf )``
  Writes the ``T.Net.Address`` information of the peer into the instance
  passed into as parameter.  Write the recieved payload into the
  ``T.Buffer/Segment`` instance instead of allocating a new string.


Overloaded send() method
........................

The three possible arguments to ``send()`` **must always** be given in the
order of: ``T.Net.Address addr, T.Buffer/Segment buf, int offset``.  The
`buf` argumnent is mandatory.  Each of the other arguments are optional.

``int sent = T.Net.Socket sck:recv( T.Net.Address addr, T.Buffer/Segment buf, int offset )``
  Send data via ``T.Net.Socket`` to `addr`.  `buf` can be a Lua string, a
  ``T.Buffer`` or a ``T.Buffer.Segment``.  If an `offset` is given the data
  send to the socket will start at `buf` index offset.  It will try to send
  as many data as possible, potentially until the end of buffer if possible.


Instance Metamembers
--------------------

``string s = tostring( T.Net.Scoket sck )  [__tostring]``
  Returns a string representing the T.Net.Socket instance.  The String
  contains type, Socket handle number and memory address information such as
  "T.Net.Socket[TCP,3]: 0xdac2e8", meaning it is a TCP Socket with socket
  handle number 3.

``T.Net.Socket sck = nil  [__gc]``
  Garbage collector makes sure the socket closes and gets properly disposed
  of on garbage collection.
