-- \file      lua/Http/Stream.lua
-- \brief     Http Stream implementation
-- \detail    References an Http Connection to a Single Client
--            A stream can have multiple request/response pairs.  HTTP1.x
--            handles that by always servicing the last incoming first,
--            Http2.0 has Message-ID
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Loop, T, Buffer = require't.Loop', require't', require't.Buffer'
local t_insert    , t_remove    , getmetatable, setmetatable, assert, type =
      table.insert, table.remove, getmetatable, setmetatable, assert, type

local Request         = require't.Http.Request'

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
	if 0 == #self.requests and not self.keepAlive then
		self.srv.ael:removeHandle( self.cli, 'read' )
	end
end

local recv = function( self )
	local succ,rcvd = self.cli:recv( self.buf )
	if not succ then
		print( "RECEIVE FAIL", succ, rcvd )
		-- dispose of itself ... clear requests, buffer etc...
		self.srv.ael:removeHandle( self.cli, 'read' )
		self.srv.streams[ self.cli ] = nil
	else
		local seg = self.buf:Segment( 1, rcvd )
		print(seg, seg:toHex() )
		local id, request = getRequest( self )
		if request:receive( seg ) then
			removeRequest( self, request )
		end
	end
end

local resp = function( self )
	local runCount, id = 0, nil
	for k,v in pairs( self.responses ) do
		if 0==runCount then
			if v:send( self.cli ) then
				id = v.id
			end
			runCount = runCount+1
		else
			runCount = runCount+1
			-- runCount == 2 -> more responses
			break;
		end
	end
	print( "runCount:",runCount, "id", id )
	if id then self.responses[ id ] = nil end
	if 1==runCount then
		self.srv.ael:removeHandle( self.cli, "write" )
		self.responding = false
	end
end

local addResponse = function( self, response )
	if not self.responses[ response.id ] then
		self.responses[ response.id ] = response
	end
	if not self.responding then
		self.srv.ael:addHandle( self.cli, 'write', resp, self )
		self.responding = true
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name      = "t.Http.Stream"
	, __index     = _mt
	, resp        = resp
	, recv        = recv
	, addResponse = addResponse
}
_mt.__index     = _mt

return setmetatable( {
}, {
	__call   = function( self, srv, cli, adr )
		--assert( T.type( srv ) == 't.Http.Server', "`t.Http.Server` is required" )
		--assert( T.type( cli ) == 'T.Net.Socket',  "`T.Net.Socket` is required" )
		--assert( T.type( adr ) == 'T.Net.Address', "`T.Net.Address` is required" )

		local stream  = {
			  srv       = srv     -- Server instance
			, cli       = cli     -- client socket
			, adr       = adr     -- client Net.Address
			, buf       = Buffer( Buffer.Size ) -- the read buffer
			, requests  = { }
			, responses = { }
			, strategy  = 1  -- 1=HTTP1.1; 2=HTTP2
			, keepAlive = false
		}

		srv.ael:addHandle( cli, 'read',  recv, stream )
		--srv.ael:addHandle( cli, 'write', resp, stream )
		return setmetatable( stream, _mt )
	end
} )

