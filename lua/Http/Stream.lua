-- \file      lua/Http/Stream.lua
-- \brief     Http Stream implementation
-- \detail    References an Http Stream object.  It basically references a
--            single Request/Response interaction   Http2.0 introduced the
--            `Stream` terminology. This however implements it for lower
--            versions(1.1) as well.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table, Buffer, Segment =
      require't.Loop', require't', require't.Table', require't.Buffer', require't.Buffer.Segment'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkStr  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local State = {
	  Method   = 1
	, Url      = 2
	, Version  = 3
	, Headers  = 4
	, Body     = 5
	, Done     = 6
}


local recv    = function( self, seg )
	local buf
	if self.buf then
		buf = Buffer( self.buf:read() .. seg:read( ) )
		seg = Segment( buf )
	end
	local seg = self.segment
	if State.Method == self.state then
		self.method            = Http.parseMethod( seg )
		if self.method then
			self.state             = State.Url
		else
			self.buf = Buffer( seg )
		end
	end
	if State.Url == self.state then
		self.query, self.url   = Http.parseUrl( seg )
		if self.query then
			self.state             = State.Headers
		else
			self.buf = Buffer( seg )
		end
	end
	if State.Headers == self.state then
		self.headers           = Http.parseUrl( seg )
		if self.headers then
			self.rsp  = Http.Response( self.con, self.id )
			self.con.srv.cb( self, self.rsp )
		else
			self.buf = Buffer( seg )
		end
	end
end


-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Http.Stream"
}


return setmetatable( {
	  State  = State
	, Method = Method
}, {
	__call   = function( self, con )
		assert( T.type( con ) == 't.Http.Connection',  "`t.Http.Connection` is required" )
		local stream = {
			  rqCLen     = 0   -- request Length as reported by Header
			, rsCLen     = 0   -- response Length as calculated or set by application
			, rsBLen     = 0   -- size of Response Buffer
			, cb         = nil -- request handler function( callback )
			, state      = State.Method
			, method     = Http.Method.Illegal
			, version    = Http.Version.VER09
			, con        = con
		}
		return setmetatable( { [ T.prxTblIdx ] = con }, _mt )
	end
} )

