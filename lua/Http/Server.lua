-- \file      lua/Http/Server.lua
-- \brief     Http Server implementation
-- \detail    As simple callback based HTTP server.  Requires to be controlled
--            via t.Loop asynchronously.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local getmetatable, setmetatable =
      getmetatable, setmetatable

local Stream, Socket, Timer  = require't.Http.Stream', require't.Net.Socket', require't.Time'
local t_type  = require't'.type
local format,o_time,t_insert = string.format,os.time,table.insert

local _mt
local timeout = 5000

-- ---------------------------- general helpers  --------------------
local accept = function( self )
	local cli, adr      = self.sck:accept( )
	cli.nonblock        = true
	self.streams[ cli ] = Stream( self, cli, adr )
end

local listen = function( self, host, port, bl )
	self.sck    = Socket( 'tcp' )
	local eMsg  = nil
	self.sck.reuseaddr, self.sck.reuseport = true, true
	if 'number' == type( host ) then
		self.adr, eMsg = self.sck:listen( host, port and port or 5 )
	else
		self.adr, eMsg = self.sck:listen( host, port, bl and bl or 5 )
	end
	if not self.adr then
		error( "Could not start HTTP Server because: " .. eMsg )
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
}, {
	__call   = function( self, ael, cb )
		assert( t_type( ael ) == 'T.Loop',   "`T.Loop` is required" )
		assert( type( cb )    == 'function', format( "Callback function required but got `%s`", cb ) )
		local srv = {
			  ael      = ael
			, callback = cb
			, streams  = { }
		}
		-- keepAlive handling
		local timer   = Timer( timeout )
		local remover = (function( srv, tm )
			local str = srv.streams
			return function( )
				local t          = o_time() - 5
				print('rinse', srv.ael)
				local candidates = { }
				for k,v in pairs( str ) do
					print(v.lastAction)
					if v.lastAction < t then
						t_insert( candidates, v.cli )
					end
				end
				for k,v in pairs( candidates ) do
					print("Timeout CLOSE Socket:", v, str[ v ] )
					ael:removeHandle( v, 'read' )
					v:close( )
					str[ v ] = nil
				end
				--dR()
				--collectgarbage()
				srv.ael:resize( )
				tm:set( timeout )
				return tm
			end
		end)( srv, timer )
		ael:addTimer( timer, remover )
		print( srv, srv.ael)

		return setmetatable( srv, _mt )
	end
} )

