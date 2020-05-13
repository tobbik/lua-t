-- \file      lua/Http/Server.lua
-- \brief     Http Server implementation
-- \detail    As simple callback based HTTP server.  Requires to be controlled
--            via t.Loop asynchronously.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local getmetatable, setmetatable =
      getmetatable, setmetatable

local Stream, Socket, Time  = require't.Http.Stream', require't.Net.Socket', require't.Time'
local t_type  = require't'.type
local s_format,t_insert = string.format,os.time,table.insert

local _mt
local timeout = 5000

-- ---------------------------- general helpers  --------------------
local accept_cb = function( self )
	local ac_count = 0
	-- greedily accept() as much as we can to favour high concurrency
	repeat
		local cli, adr      = self.sck:accept( )
		if cli then
			ac_count = ac_count + 1
			cli.nonblock        = true
			self.streams[ cli ] = Stream( self, cli, adr )
			if self._event_handlers.connection then
				self._event_handlers.connection( cli, adr )
			end
		else
			if 0 == ac_count then -- actual error condition
				print( s_format( "Couldn't accept Client Socket: `%s`", adr ) )
			else
				--print( s_format( "Accepted simultaniously: %d", ac_count ) )
			end
		end
	until not cli
end

local listen = function( self, host, port, bl )
	self.sck    = Socket( 'tcp' )
	local eMsg  = nil
	self.sck.reuseaddr, self.sck.reuseport = true, true
	if 'number' == type( host ) then -- host is port, port is backlog, host defaults to 0.0.0.0 (all interfaces)
		self.adr, eMsg = self.sck:listen( host, port and port or nil )
	else
		self.adr, eMsg = self.sck:listen( host, port, bl and bl or nil )
	end
	if not self.adr then
		error( "Could not start HTTP Server because: " .. eMsg )
	end
	self.sck.nonblock = true
	self.ael:addHandle( self.sck, 'read', accept_cb, self )
	return self.sck, self.adr
end

local removeStream = function( self, stream )
	self.streams[ stream.cli ] = nil
end

local on = function( self, event_name, handler )
	self._event_handlers[ event_name ] = handler
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name     = "t.Http.Server"
	, listen     = listen
	, on         = on
}

_mt.__index     = _mt

dR = function()
	local d = debug.getregistry()
	for i=#d,3,-1 do
		if type(d[i]) =='table' then
			print( "TABLE", i, d[i] )
			for k,v in pairs(d[i]) do print('',k,v) end
		elseif type(d[i]) =='userdata' then
			print( "USERDATA", i, d[i] )
		end
	end
end

return setmetatable( {
	  toString = function( srv ) return _mt.__name end
}, {
	__call   = function( self, ael, cb )
		assert( t_type( ael ) == 'T.Loop',   "`T.Loop` is required" )
		assert( type( cb )    == 'function', s_format( "Callback function required but got `%s`", cb ) )
		local srv = {
			  ael              = ael
			, callback         = cb
			, streams          = { }
			, _event_handlers  = { }
		}
		-- crude keepAlive handling, rudely remove staleish sockets
		--[[
		local timer   = Time( timeout )
		local remover = (function( srv, tm )
			local str = srv.streams
			return function( )
				local t          = Time.get() - timeout
				print('rinse', srv.ael)
				local candidates = { }
				for k,v in pairs( str ) do
					if v.lastAction < t then
						print(" Overdue:",v.lastAction, t,  t - v.lastAction)
						t_insert( candidates, v.sck )
					end
				end
				for k,v in pairs( candidates ) do
					print("############# Timeout CLOSE Socket:", v, str[ v ] )
					ael:removeHandle( v, 'readwrite' )
					v:close( )
					str[ v ] = nil
				end
				--dR()
				--collectgarbage()
				tm:set( timeout )
				return tm
			end
		end)( srv, timer )
		ael:addTimer( timer, remover )
		--]]

		return setmetatable( srv, _mt )
	end
} )

