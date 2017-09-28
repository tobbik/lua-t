-- \file      lua/Http/Server.lua
-- \brief     Http Server implementation
-- \detail    As simple callback based HTTP server.  Requires to be controlled
--            via t.Loop asynchronously.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local getmetatable, setmetatable =
      getmetatable, setmetatable

local Stream, Socket, Timer = require't.Http.Stream', require't.Net.Socket', require't.Time'
local t_type  = require't'.type
local format,o_time,t_insert  = string.format,os.time,table.insert

local _mt

-- ---------------------------- general helpers  --------------------
local accept = function( self )
	local cli, adr      = self.sck:accept( )
	--print("CLIENT:", cli)
	self.streams[ cli ] = Stream( self, cli, adr )
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

local removeStream = function( self, stream )
	self.streams[ stream.cli ] = nil
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
	__call   = function( self, ael, cb )
		assert( t_type( ael ) == 'T.Loop',   "`T.Loop` is required" )
		assert( type( cb )    == 'function', format( "Callback function required but got `%s`", cb ) )
		local srv = {
			  ael      = ael
			, callback = cb
			, streams  = { }
		}
		local remover = (function(s)
			return function( )
				local t          = o_time() - 5
				print(t, s)
				local candidates = { }
				for k,v in pairs( s.streams ) do
					print(v.lastAction, t)
					if v.lastAction < t then
						t_insert( candidates, v.cli )
					end
				end
				for k,v in pairs( candidates ) do
					print("CLOSE:", v)
					v:close()
					s.streams[ v ] = nil
				end
				s.timer:set( 5000 )
				return s.timer
			end
		end)( srv )
		srv.timer    = Timer( 5000 )
		ael:addTimer( srv.timer, remover, srv )
		return setmetatable( srv, _mt )
	end
} )

