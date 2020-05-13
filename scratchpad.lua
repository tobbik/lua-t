local Loop     = require't.Loop'
local Server   = require't.Http.Server'
local fmt      = string.format

local httpServer = Server( Loop(), print )

local host,port = '0.0.0.0',8000

local srv, adr   = httpServer:listen( host, port, 1000 )
print( fmt( "Started `%s` at `%s` (%s)", srv, adr, srv.family ) )
httpServer.ael:run( )
