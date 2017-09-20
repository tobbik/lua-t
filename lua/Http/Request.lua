-- \file      lua/Http/Request.lua
-- \brief     Http Request implementation
-- \detail    References an Http Request object.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table, Buffer =
      require't.Loop', require't', require't.Table', require't.Buffer'
local Method, Version = require't.Http.Method', require't.Http.Version'

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
	if State.Version == self.state then
		self.version           = Http.parseVersion( seg )
		if self.version then
			self.state             = State.Version
		else
			self.buf = Buffer( seg )
		end
	end
	if State.Headers == self.state then
		self.headers           = Http.parseUrl( seg )
		if self.headers then
			self.rsp  = Http.Response( self.id, self.version )
			self.callback( self, self.rsp )
		else
			self.buf = Buffer( seg )
		end
	end
end


-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Http.Request"
}


return setmetatable( {
	  State  = State
}, {
	__call   = function( self, callback )
		local request = {
			  rqCLen     = 0   -- request Length as reported by Header
			, rsCLen     = 0   -- response Length as calculated or set by application
			, rsBLen     = 0   -- size of Response Buffer
			, cb         = nil -- request handler function( callback )
			, state      = State.Method
			, method     = Method.Illegal
			, version    = Version.VER09
			, callback   = callback
		}
		return setmetatable( request, _mt )
	end
} )

