-- \file      lua/Http/Connection.lua
-- \brief     Http Connection implementation
-- \detail    References an Http Connection to a Single Client
--            A connection can have multiple request/response pairs.  HTTP1.x
--            handles that by always servicing the
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Loop, T, Buffer = require't.Loop', require't', require't.Buffer'
local t_insert    , t_remove    , getmetatable, setmetatable, assert, type =
      table.insert, table.remove, getmetatable, setmetatable, assert, type

local Request = require't.Http.Request'

local _mt

-- ---------------------------- general helpers  --------------------
local getRequest = function( self )
	-- if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a request identifier ... TODO:
	local id      = #self.requests
	local request = self.requests[ id ]
	if not request then
		request = Request( self, id+1 )
		t_insert( self.requests, request )
		id      = id+1
	end
	return id, request
end

local removeRequest = function( self, request )
	t_remove( self.requests, request.id )
	if 0 == #self.requests then
		self.srv.ael:removeHandle( self.cli, 'read' )
	end
end

local recv = function( self )
	local succ,rcvd = self.cli:recv( self.buf )
	print( "RCVD BYTES:", rcvd );
	if not succ then
		-- dispose of itself ... clear requests, buffer etc...
	else
		local id, request = getRequest( self )
		if not request:receive( self.buf:Segment( 1, rcvd ) ) then
			removeRequest( self, request )
		end
	end
end

local resp
resp    = function( self )
	local request = self.requests[ #self.requests ]
	if request.response:send( self.cli ) then
		t_remove( self.requests, request.id )
	end
	-- Keep existing if keep-alive (put timer on loop for self destroy)
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name     = "t.Http.Stream"
	, __index    = _mt
	, resp       = resp
	, recv       = recv
}

return setmetatable( {
}, {
	__call   = function( self, srv, cli, adr )
		assert( T.type( srv ) == 't.Http.Server', "`t.Http.Server` is required" )
		assert( T.type( cli ) == 'T.Net.Socket',  "`T.Net.Socket` is required" )
		assert( T.type( adr ) == 'T.Net.Address', "`T.Net.Address` is required" )

		local stream  = {
			  srv       = srv     -- Server instance
			, cli       = cli     -- client socket
			, adr       = adr     -- client Net.Address
			, buf       = Buffer( Buffer.Size ) -- the read buffer
			, requests  = { }
			, responses = { }
			, strategy  = 1  -- 1=HTTP1.1; 2=HTTP2
		}

		srv.ael:addHandle( cli, 'read',  recv, stream )
		--srv.ael:addHandle( cli, 'write', resp, stream )
		return setmetatable( stream, _mt )
	end
} )

