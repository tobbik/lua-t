lua-t Http.Response - Control Http Response
+++++++++++++++++++++++++++++++++++++++++++


Overview
========

The ``Http.Response res`` is the second argument to the callback passed into
the ``Http.Server`` constructor.

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

``Http.Response res = Http.Response( Http.Stream stream, int id, string version, int created)  [__call] internal only``
  Creates an ``Http.Response res`` instance.  ``Http.Stream stream`` is a
  stream object created by the ``Http.Server`` instance.  The ``int id``
  identifies the response and is matched by the ``Http.Request req.id``
  which is issued by the ``Http.Stream stream`` instance.  ``string
  version`` identifies the HTTP version as it was determined by the
  corresponding ``Http.Request req.version``.  It was parsed from the
  incoming HTTP header.


Instance Members
----------------

None.

Instance Metamembers
--------------------

``integer n = #Http.Request req  [__len]``
  Returns the content length of the request as reported by the HTTP header
  if send by the HTTP client.
