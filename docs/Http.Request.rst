lua-t Http.Request - Control Http Requests
++++++++++++++++++++++++++++++++++++++++++


Overview
========

The ``Http.Request req`` is the first argument to the callback passed into
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

``Http.Request req = Http.Request( Http.Stream stream, int id )       [__call] internal only``
  Creates an ``Http.Request req`` instance.  ``Http.Stream stream`` is a
  stream object created by the ``Http.Server`` instance.  The ``int id``
  identifies the response and is matched by the ``Http.Request req.id``
  which is issued by the ``Http.Stream stream`` instance.


Instance Members
----------------

``Http.Stream stream        = req.stream``
  Reference to the stream object.

``integer id                = req.id``
  Integer identification issued by the Stream.

``Http.Request.State state  = req.state``
  Tracking the state of the request.

``Http.Method method        = req.method``
  Parsed HTTP method from the HTTP header. (eg. GET, POST, OPTION etc.)

``Http.Version version      = req.version``
  Parsed HTTP version from the HTTP header. (eg. HTTP/1.0, HTTP/1.1)

``integer contentLength     = req.contentLength``
  If the HTTP header contained a Content-Length this is the parsed number.
  This is the same as #req.

``booelan keepAlive         = req.keepAlive``
  Parsed from the HTTP header, true if ``Connection: keep-alive`` was
  parsed. Else false.

``int created               = req.created``
  Integer of timestamp since epoch when the Request was issued.

``table headers             = req.headers``
  Parsed HTTP Header key mapped to value.

``string url                = req.url``
  Complete url as parsed from the HTTP header.

``string path               = req.path``
  Path from the url. Does not contain the search string.

``string search              = req.search``
  Search string from the url.

``table query               = req.query``
  Parsed search string.

Instance Metamembers
--------------------

``integer n = #Http.Request req  [__len]``
  Returns the content length of the request as reported by the HTTP header
  if send by the HTTP client.
