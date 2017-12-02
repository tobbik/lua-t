-- \file      lua/Http/Stream.lua
-- \brief     Http Stream implementation
-- \detail    References an Http Connection to a Single Client
--            A stream can have multiple request/response pairs.  HTTP1.x
--            handles that by always servicing the last incoming first,
--            Http2.0 has Message-ID
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local Loop, T, Time = require't.Loop', require't', require't.Time'
local t_insert    , t_remove    ,  o_time, s_format      =
      table.insert, table.remove, os.time, string.format
local getmetatable, setmetatable, assert, type =
      getmetatable, setmetatable, assert, type

local Request, Response  = require't.Http.Request', require't.Http.Response'


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
	local dur = o_time() - self.created
	if dur > 2 then
		print( s_format( "LONG RUNNING STREAM: `%s`  %d seconds", self.sck, dur ) )
	end
	--print( "DESTROY:", self, self.sck )
	self.requests  = nil
	self.responses = nil
	self.srv.streams[ self.sck ] = nil
	self.sck:close( )
	self.sck = nil
end

local recv = function( self )
	local data,rcvd = self.sck:recv( )
	if not data then
		-- it means the other side hung up; No more responses
		print( s_format("----------------REMOVE read handler -> RECEIVE FAILURE on `%s` (%s) Time: %d",
			self.sck, rcvd, o_time( ) - self.lastAction ) )
		-- dispose of itself ... clear requests, buffer etc...
		self.srv.ael:removeHandle( self.sck, 'readwrite' )
		destroy( self )
	else
		self.lastAction = o_time( )
		local id, request = getRequest( self )
		if request:receive( data ) then
			--print("REQUEST DONE")
			t_remove( self.requests, request.id )
			self.keepAlive = request.keepAlive
			if 0 == #self.requests and not self.keepAlive then
				--print( "-----REMOVE read handler", self.sck)
				self.srv.ael:removeHandle( self.sck, 'read' )
			end
		end
		if request.state > Request.State.Headers then
			self.srv.callback( request, Response( self, request.id, request.version, request.created ) )
		end
	end
end

--  ************************************************************************
--- Called via t.Loop when socket is writeable again
--- grab a response and run send again
--  ************************************************************************
local send  --- forward declaration
local respond = function( self )
	local destroyStream, removeFromLoop, stopSending = true, true, false
	for k,v in pairs( self.responses ) do
		local rspDone, sckRemove, stopSend = send( self, v )
		if     stopSend   then stopSending    = true; break end -- sck is on the loop; wait for readyness
		if not rspDone    then destroyStream  = false end -- when any not done, don't destroy
		if not sckRemove  then removeFromLoop = false end -- when any not done, don't destroy
	end
	--[[
	print( destroyStream    and "destroyStream"  or "keepStream",
			 removeFromLoop   and "removeFromLoop" or "stayObserved",
			 stopSending      and "stopSending"    or "sendNext",
			 self.isOnOutLoop and "isInLoop"       or "notInLoop",
			 self.keepAlive   and "keepAlive"       or "close" )
	--]]
	if not stopSending then
		if not self.keepAlive and destroyStream then
			destroy( self )
		end
		if self.isOnOutLoop and removeFromLoop then
			self.srv.ael:removeHandle( self.sck, "write" )
			self.isOnOutLoop = true
		end
	end
end

-- *************************************************************
-- * Send data from a single response
-- return false if for any reason this response must remain active
-- * TODO: probably combine data from multiple responses (HTTP2 stuff)
--         the goal is to shove down as much data as possible the socket
-- *************************************************************
-- forward declared above
send = function( self, response )
	local responseDone, removeSocket, stopSending   = false, false, false
	local buf            = response:getBuffer( )
	local snt, eMsg, eNo = self.sck:send( buf )
	if snt then
		if #buf == snt then
			removeSocket = true
			if Response.State.Done == response.state then
				self.responses[ response.id ] = nil   -- release response
				responseDone = true
			else
				response:setBuffer( nil )
			end
		else
			response:setBuffer( buf:sub( snt+1 ) )
		end
	else
		stopSending = true
		if (11 == eNo) then --EAGAIN/EWOULDBLOCK -> put on loop
			if not self.isOnOutLoop then
				self.srv.ael:addHandle( self.sck, 'write', respond, self )
				self.isOnOutLoop = true
			end
		else
			print( "Failed to send:", eMsg, eNo )
			self.srv.ael:removeHandle( self.sck, 'readwrite' )
			destroy( self )
		end
	end
	return responseDone, removeSocket, stopSending
end

-- only run the response
local addResponse = function( self, response )
	if not self.responses[ response.id ] then
		self.responses[ response.id ] = response
	end
	if not self.isOnOutLoop then
		respond( self )
	end
end


-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	  __name      = "t.Http.Stream"
	, __index     = _mt
	, recv        = recv
	, addResponse = addResponse
}
_mt.__index     = _mt

return setmetatable( {
}, {
	__call   = function( self, srv, sck, adr )
		--assert( T.type( srv ) == 't.Http.Server', "`t.Http.Server` is required" )
		--assert( T.type( sck ) == 'T.Net.Socket',  "`T.Net.Socket` is required" )
		--assert( T.type( adr ) == 'T.Net.Address', "`T.Net.Address` is required" )

		local stream  = {
			  srv         = srv     -- Server instance
			, sck         = sck     -- client socket
			, adr         = adr     -- client Net.Address
			, requests    = { }
			, responses   = { }
			, strategy    = 1  -- 1=HTTP1.1; 2=HTTP2
			, keepAlive   = true
			, isOnOutLoop = false   -- we must wait for writing to resume again
			, lastAction  = o_time( )
			, created     = o_time( )
		}

		--print( "+++++ADDING read handler", sck)
		srv.ael:addHandle( sck, 'read', recv, stream )
		return setmetatable( stream, _mt )
	end
} )

