-- \file      lua/Http/Response.lua
-- \brief     Http Response implementation
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local t_insert    , t_concat    ,    time,    date,        format, setmetatable, pairs =
      table.insert, table.concat, os.time, os.date, string.format, setmetatable, pairs
local Status = require't.Http.Status'

local _mt

local State = {
	  Zero        = 0
	, HeadDone    = 1
	, Send        = 2
	, Done        = 3
}

local now = (function( )
	local now = time( )
	local str = date( "%a, %d %b %Y %H:%M:%S %Z", now )
	return function( )
		local n = time( )
		if n - now>0 then
			now = n
			str = date( "%a, %d %b %Y %H:%M:%S %Z", now )
		end
		return str
	end
end) ( )

local formHeader = function( httpVersion, statusCode, statusMsg, len, headers, keepAlive )
	statusCode = statusCode or 200
	local headBuffer = {
		httpVersion .." ".. statusCode .." ".. statusMsg ..
		"\r\nDate: ".. now( ) ..
		"\r\nConnection: " .. (keepAlive and "Keep-Alive" or "Close") ..
		(len and "\r\nContent-Length: " .. len or "\r\nTransfer-Encoding: chunked") ..
		"\r\n" .. (headers and '' or '\r\n')
	}
	if headers then
		for k,v in pairs( headers ) do
			t_insert( headBuffer, k .. ": " ..v.. "\r\n" )
		end
		t_insert( headBuffer, "\r\n" )
	end
	if len then
		return headBuffer, false
	else
		return headBuffer, true
	end
end

-- this takes different parameters in different positions
local writeHead = function( self, stsCde, msg, length, hdr )
	if self.state > State.Zero then error( "Can't set Head multiple times" ) end
	local stsMsg  = 'string' == type( msg ) and msg or Status[ stsCde ]
	local cntLen  = 'number' == type( msg ) and msg or length -- can be nil!
	local headers = 'table'  == type( msg ) and msg or length -- can be nil!
	headers       = 'table'  == type( headers ) and headers or hdr  -- can be nil
	self.buf,self.chunked = formHeader( self.version, stsCde, stsMsg, cntLen, headers, self.keepAlive )
	self.state    = State.HeadDone
end

local write = function( self, msg )
	if self.state < State.HeadDone then
		self.buf, self.chunked = formHeader( self.version, 200, Status[ 200 ], nil, nil, self.keepAlive )
	end
	t_insert( self.buf, self.chunked and format("%X\r\n%s\r\n", #msg, msg) or msg )
	self.state = State.Send
end

local finish = function( self, msg )
	if self.state < State.HeadDone then
		self.buf,self.chunked = formHeader( self.version, 200, Status[ 200 ], (msg and #msg or 0), nil, self.keepAlive )
	end
	if msg then
		t_insert( self.buf, self.chunked and format("%X\r\n%s\r\n", #msg, msg) or msg )
	end
	if self.chunked then t_insert( self.buf, "0\r\n\r\n" ) end
	self.state = State.Done
end

local send = function( self, sck )
	local buf = t_concat( self.buf )
	local snt = sck:send( buf )
	if snt == #buf and State.Done == self.state then
		return true
	elseif snt == #buf  then
		self.buf = { }
		return false
	else
		self.buf = { buf:sub( snt+1 ) }
		return false
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	  __name     = "t.Http.Response"
	, writeHead  = writeHead
	, write      = write
	, finish     = finish
	, send       = send
}
_mt.__index     = _mt

return setmetatable( {
	  State  = State
}, {
	__call   = function( self, id, keepAlive, version )
		local response = {
			  id         = id    -- StreamId
			, keepAlive  = keepAlive
			, cLen       = 0   -- Content-Length (Body)
			, bLen       = 0   -- length of Buffer to send (includes all Headers)
			, state      = State.Zero
			, version    = version
			, chunked    = true
		}
		return setmetatable( response, _mt )
	end
} )

