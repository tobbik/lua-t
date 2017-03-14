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

Quite a few methods and functions make heavy use of argument type
inspection.  Notably, bind(), connect() and listen() determine the
significance of an argument by type:

 - If the first argument is a ``T.Net.Address``, bind(), connect() or
   listen() at that address and just returns the socket.
 - If the first argument is a string, interpret as an IP address, and create
   an IP address with that value which is returned as second return value.
 - If the first argument is an integer assume it as port and assume the IP
   address to be either `0.0.0.0` or `localhost` depending on the function.
   Create the ``T.Net.Address`` instance and return as second value.

Similarily send() and recv() act for both UDP and TDP type sockets.  In case
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
  the it will automatically bind to **0.0.0.0**, the IP_ANY interface.  In
  this case the port is the only argument which is mandatory.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.bind( T.Net.Address addr )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is bound to the
  T.Net.Address.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.listen( [string ip,] int port, int backlog )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is listening
  connections on the address requested via the ``ip`` string and ``port``
  number. Ip string is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is
  omitted it will automatically bind to **0.0.0.0**, the IP_ANY interface.
  In this case the port is the only argument which is mandatory.
  ``backlog`` is optional and defaults to 5.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.listen( T.Net.Address addr, int backlog )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is listening to the
  T.Net.Address.  ``backlog`` is optional and defaults to 5.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.connect( [string ip,] int port )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is connected to the
  address requested via the ``ip`` string and ``port`` number. Ip string
  is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted
  the it will automatically connect to **127.0.0.1**, the `localhost`
  interface.  In this case the port is the only argument which is mandatory.

``T.Net.Socket sck, T.Net.Address a = T.Net.Socket.connect( T.Net.Address addr )``
  Creates an IPv4 TCP ``T.Net.Socket`` instance which is connected to the
  T.Net.Address.


Class Metamembers
-----------------

T.Net.Socket has no clone constructor because sockets are system resources
which can't be duplicated.

``T.Net.Socket sck = T.Net.Socket( [string protocol, string domain] )   [__call]``
  Instantiate a new T.Net.Socket object.  If no arguments are passed it will
  assume *TCP* and *ip4* as default values.  If only one argument is passed
  it will be interpreted as ``protocol`` and *ip4* is assumed as default
  domain.


Instance Members
----------------

``T.Net.Address addr = T.Net.Socket sck:bind( [string ip,] int port )``
  Creates and returns an ``T.Net.Address`` instance defined by the ``ip``
  string and ``port`` number and binds the ``sck`` instance to it.  Ip string
  is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted it will
  automatically bind to **0.0.0.0**, the IP_ANY interface.  In this case the
  port is the only argument which is mandatory.

``T.Net.Socket sck:bind( T.Net.Address addr )``
  Binds the ``T.Net.Socket`` instance to the ``T.Net.Address``.

``T.Net.Address addr = T.Net.Socket sck:connect( [string ip,] int port )``
  Creates and returns an ``T.Net.Address`` instance defined by the ``ip``
  string and ``port`` number and connects the ``sck`` instance to it.  Ip
  string is accepted as **aaa.bbb.ccc.ddd**.  If the ip string is omitted it
  will automatically connect to **127.0.0.1**, the `localhost` interface.
  In this case the port is the only argument which is mandatory.

``T.Net.Socket sck:connect( T.Net.Address addr )``
  Connects the ``T.Net.Socket`` instance to the ``T.Net.Address``.

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

``T.Net.Socket client, T.NetAddress addr = T.Net.Socket sck:accept( )``
  Accepts a new connection the ``T.Net.Socket`` instance.  Returns the
  ``T.Net.Address`` client instance and the clients ``T.Net.Address``
  instance.


Overloaded recv() method
........................

The three possible arguments to ``recv()`` **must always** be given in the
order of: ``T.Buffer/Segment buf, T.Net.Address addr, int max``.  Each of the
arguments is optional.

``string msg, int rcvd, T.NetAddress addr = T.Net.Socket sck:recv( int max )``
  Receives data from the ``T.Net.Socket`` instance.  Returns the
  ``T.Net.Address`` client instance and the clients ``T.Net.Address``
  instance.  ``msg`` contains the payload recieved.  ``max`` limits the
  amount of data received at once.  If no is passed the maximum of `BUF_LEN`
  is used.  Values bigger than `BUF_LEN` are an error.

``string msg, int rcvd = T.Net.Socket sck:recv( T.NetAddress addr )``
  Writes the ``T.Net.Address`` information of the peer into the instance
  passed into as parameter.

``int rcvd, T.NetAddress addr = T.Net.Socket sck:recv( T.Buffer/Segment buf )``
  Write the recieved payload into the ``T.Buffer/Segment`` instance instead
  of allocating a new string.

``int rcvd = T.Net.Socket sck:recv( T.Buffer/Segment buf, T.NetAddress addr )``
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
