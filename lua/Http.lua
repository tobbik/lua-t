local core       = require( "t.core" )
local connection = require( "t.Http.Connection" )
local server     = require( "t.Http.Server" )
local stream     = require( "t.Http.Stream" )
local websocket  = require( "t.Http.WebSocket" )

local Http   = core.htp

Http.Connection = connection
Http.Server     = server
Http.Stream     = stream
Http.WebSocket  = websocket

return Http
