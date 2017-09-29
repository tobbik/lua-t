-- \file      lua/Http/Stream.lua
-- \brief     Http Stream implementation
-- \detail    References an Http Connection to a Single Client
--            A stream can have multiple request/response pairs.  HTTP1.x
--            handles that by always servicing the last incoming first,
--            Http2.0 has Message-ID
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Loop, T, Buffer, Timer = require't.Loop', require't', require't.Buffer'
local t_insert    , t_remove    , getmetatable, setmetatable, assert, type,  o_time =
      table.insert, table.remove, getmetatable, setmetatable, assert, type, os.time

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

local destroy = function( self )
	--print( "DESTROY:", self.buf, self )
	self.requests  = nil
	self.responses = nil
	self.buf = nil
	self.srv.streams[ self.cli ] = nil
	self.cli:close( )
	self.cli = nil
end

local recv = function( self )
	local success,rcvd = self.cli:recv( self.buffer )
	if not success then
		-- it means the other side hung up; No more responses
		--print( "REMOVE read handler -> RECEIVE FAILURE", self.cli )
		-- dispose of itself ... clear requests, buffer etc...
		self.srv.ael:removeHandle( self.cli, 'read' )
		destroy( self )
	else
		self.lastAction = o_time( )
		local id, request = getRequest( self )
		if request:receive( self.buffer, rcvd ) then
			-- print("REQUEST DONE")
			t_remove( self.requests, request.id )
			if 0 == #self.requests and not self.keepAlive then
				--print("REMOVE read handler", self.cli)
				self.srv.ael:removeHandle( self.cli, 'read' )
				self.reading = false
			end
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
	--print( "runCount:",runCount, "id", id )
	if id then self.responses[ id ] = nil end
	if 1==runCount then      -- no further response in the stream
		self.srv.ael:removeHandle( self.cli, "write" )
		if not self.keepAlive then
			destroy( self )
		else
			self.lastAction = o_time( )
		end
	end
end

local addResponse = function( self, response )
	if not self.responses[ response.id ] then
		self.responses[ response.id ] = response
	end
	if not self.responding then
		self.srv.ael:addHandle( self.cli, 'write', resp, self )
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
			  srv        = srv     -- Server instance
			, cli        = cli     -- client socket
			, adr        = adr     -- client Net.Address
			, buf        = Buffer( Buffer.Size ) -- the read buffer
			, requests   = { }
			, responses  = { }
			, buffer     = Buffer( Buffer.Size )
			, strategy   = 1  -- 1=HTTP1.1; 2=HTTP2
			, keepAlive  = false
			, reading    = true
			, lastAction = o_time()
		}

		--print( "ADDING HANDLE CLIENT:", cli )
		srv.ael:addHandle( cli, 'read', recv, stream )
		--print( "ADDED HANDLE CLIENT:", cli )
		--srv.ael:addHandle( cli, 'write', resp, stream )
		return setmetatable( stream, _mt )
	end
} )

