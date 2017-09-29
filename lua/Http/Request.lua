-- \file      lua/Http/Request.lua
-- \brief     Http Request implementation
-- \detail    References an Http Request object.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type

local Loop, T, Table, Buffer =
      require't.Loop', require't', require't.Table', require't.Buffer'

-- require't.Http' loads the the .so file and puts T.Http.Request metatable into the registry
local           Http ,                Method ,                Version ,                Response =
      require't.Http', require't.Http.Method', require't.Http.Version', require't.Http.Response'

--local _mt
local _mt = debug.getregistry( )[ "T.Http.Request" ]

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkStr  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local State = {
	  Method  = 1
	, Url     = 2
	, Version = 3
	, Headers = 4
	, Body    = 5
	, Done    = 6
}

-- receive
-- @return
local receive = function( self, data )
	local tail = self:parse( self.tail and (self.tail .. data) or data, self.state )
	if self.state > State.Headers then
		self.stream.keepAlive = self.keepAlive
		self.stream.srv.callback( self, Response( self.stream, self.id, self.keepAlive, self.version ) )
	end
	if self.state == State.Done then
		self.buf = nil
		return true
	else
		--print("NEW TAIL BUFFER")
		self.tail = tail or nil
		return false
	end
end

_mt.receive = receive
_mt.__name  = "t.Http.Request"  --TODO: Fix naming globally; use lower case 't'
_mt.__len   = function( self )
	return self.contentLength
end

return setmetatable( {
	  State  = State
}, {
	__call   = function( self, stream, id )
		local request = {
			  stream        = stream
			, id            = id
			, state         = State.Method
			, method        = Method.ILLEGAL
			, version       = Version.ILLEGAL
			, contentLength = nil
			, keepAlive     = false
			, headers       = { }      -- pre-existent for re-entrant header parsing
		}
		return setmetatable( request, _mt )
	end
} )

