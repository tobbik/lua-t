lua-t Http.Stream - An Http Connection associated with a socket
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

When the ``Http.Server`` accepts a new client connection to receive an Http
message, the new client socket will be wrapped within a stream.  There are
hardly any consequenses for single request connections.  However, modern
Http attempts to keep sockets open and reuse them for multiple requests to
avoid the overhead of socket creation and destruction.  This principle is
known as ``keep-alive``.  Differnet generations of the Http protocoll
specify different keep-alive behaviours and implementations vary from server
to server and from client to client. ``Http.Stream`` is this servers
implementation anconcept to handle the keepalive request.  It is mainly used
for the internal workings of the Http server, however, several aspects of it
can be used from an aplplication perspective.


API
===

Class Members
-------------

None.


Class Metamembers
-----------------

``Http.Stream s = Http.Stream( Http.Server server, Net.Socket client_sock, Net.Address client_addr )       [__call]``
  Creates an ``Http.Stream s`` instance.  This is executed from the
  ``Http.Server`` accept() method.


Instance Members
----------------

``Net.Socket sck, Net.Address adr = Http.Server srv:listen( )``
  Takes the same arguments as `Net.Socket.Listen()
  <Net.Socket.rst#Net-Socket-listen>`__.


Instance Metamembers
--------------------


``string s = tostring( Http.Stream stream )  [__toString]``
  Returns a string representing ``Http.Stream stream`` instance.  The string
  contains type, length and memory address information, for example:
  *`t.Http.Stream[1]: 0x1193d18`* for a Stream with on request.
