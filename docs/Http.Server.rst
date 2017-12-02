lua-t Http.Server - Asynchronous event driven Http server
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Overview
========

Http.Server provides an event driven Http server which allows to receive
HTTP requests and send out HTTP responses.  

  .. code:: lua

   Loop   = require't.Loop'
   Server = require't.Http.Server'
   l = Loop( )
   s = Server( l, function( req, res ) ... do stuff end )
   s:listen( '0.0.0.0', 8000 )
   l:run( )


API
===

Class Members
-------------

None.


Class Metamembers
-----------------

``Http.Server s = Http.Server( Loop l, function callback )       [__call]``
  Creates an ``Http.Server s`` instance.  ``Loop l`` is mandatory.  The
  ``function callback`` is executed anytime a request header is completely
  received.


Instance Members
----------------

``Net.Socket sck, Net.Address adr = Http.Server srv:listen( )``
  Takes the same arguments as `Net.Socket.Listen()
  <Net.Socket.rst#Net-Socket-listen>`__.


Instance Metamembers
--------------------

None.
