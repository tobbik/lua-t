-- \file      lua/Http/Connection.lua
-- \brief     Http Connection implementation
-- \detail    References an Http Connection to a Single Client
--            A connection can have multiple request/response pairs.  HTTP1.x
--            handles that by always servicing the
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table, Buffer, Segment = require't.Loop', require't', require't.Table', require't.Buffer', require't.Buffer.Segment'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Http.Connection type and return the proxy table
local chkCon  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self
end

local getStream = function( self )
	-- if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a stream identifier ... TODO:
	local stream = self.streams[ #self.streams ]
	if not stream then
		stream = Http.Stream( self )
		t_insert( self.streams, stream )
	end
	return stream
end

local recv    = function( self )
	local rcvd = self.cli:recv( self.buf )
	t.print( "RCVD: %d bytes\n", rcvd );
	if not rcvd then
		--dispose of itself ... clear streams, buffer etc...
	else
		local stream = getStream( self )
		stream:recv( Buffer.Segment( self.buf, 1, rcvd ) )
	end
end

local resp
resp    = function( self )
	local stream = self.streams[ #self.streams ]
	local snt    = self.cli:send( stream )
	if stream.state == stream.State.Done then
		self.srv.ael:removeHandle( cli, 'write' )
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name     = "t.Http.Connection"
	, __index    = _mt
	, resp       = resp
	, recv       = recv
}

return setmetatable( {
}, {
	__call   = function( self, srv, cli, adr )
		assert( T.type( srv ) == 't.Http.Server',  "`t.Http.Server` is required" )
		assert( T.type( cli ) == 'T.Net.Socket',   "`T.Net.Socket` is required" )
		assert( T.type( adr ) == 'T.Net.Address',  "`T.Net.Address` is required" )

		local con  = {
			  srv       = srv     -- Server instance
			, cli       = cli     -- client socket
			, adr       = adr     -- client Net.Address
			, buf       = Buffer( Buffer.Size ) -- the read buffer
			, rcvd      = 0
			, requests  = { }
			, responses = { }
			, strategy  = 1  -- 1=HTTP1.1; 2=HTTP2
		}

		srv.ael:addHandle( cli, 'read',  recv, con )
		srv.ael:addHandle( cli, 'write', resp, con )
		return setmetatable( con, _mt )
	end
} )

