-- \file      lua/Http/Server.lua
-- \brief     Http Server implementation
-- \detail    As simple callback based HTTP server.  Requires to be controlled
--            via t.Loop asynchronously.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local getmetatable, setmetatable =
      getmetatable, setmetatable

local Stream, Socket = require't.Http.Stream', require't.Net.Socket'
local t_type  = require't'.type

local _mt

-- ---------------------------- general helpers  --------------------
local accept = function( self )
	local cli, adr = self.sck:accept( )
	local con      = Stream( self, cli, adr )
end

local listen = function( self, host, port, bl )
	if 'number' == type( host ) then
		self.sck, self.adr = Socket.listen( host, bl and bl or 5 )
	else
		self.sck, self.adr = Socket.listen( host, port, bl and bl or 5 )
	end
	self.sck.nonblock = true
	self.ael:addHandle( self.sck, 'read', accept, self )
	return self.sck, self.adr
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name     = "t.Http.Server"
	, listen     = listen
}

_mt.__index     = _mt

return setmetatable( {
	  toString = function( srv ) return _mt.__name end
	, listen   = listen
}, {
	__call   = function( self, ael, callback )
		assert( t_type( ael ) == 'T.Loop',   "`T.Loop` is required" )
		assert( type( cb )    == 'function', "Callback function required" )
		local srv = {
			  ael      = ael
			, callback = callback
		}
		return setmetatable( srv, _mt )
	end
} )

