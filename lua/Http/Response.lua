-- \file      lua/Http/Response.lua
-- \brief     Http Response implementation
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local t_insert    , t_concat    ,    time,    date,        format, setmetatable, pairs =
      table.insert, table.concat, os.time, os.date, string.format, setmetatable, pairs
local Status, Version = require't.Http.Status', require't.Http.Version'

local _mt

local State = {
	  Zero        = 0
	, Written     = 1
	, Done        = 2
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

local formHeader = function( self, msg )
	if self.contentLength then self.chunked = false end
	self.buf = {
		Version[ self.version ] .." ".. self.statusCode .." ".. self.statusMessage ..
		"\r\nDate: ".. now( ) ..
		"\r\nConnection: " .. (self.keepAlive and "keep-alive" or "close") ..
		(self.contentLength and "\r\nContent-Length: " .. self.contentLength or "\r\nTransfer-Encoding: chunked") ..
		"\r\n" .. (self.headers and '' or '\r\n') ..
		((not self.headers and msg) and (self.chunked and format( "%X\r\n%s\r\n", #msg, msg ) or msg) or '' )
	}
	if self.headers then
		for k,v in pairs( self.headers ) do
			t_insert( self.buf, k .. ": " ..v.. "\r\n" )
		end
		t_insert( self.buf, "\r\n" )
	end
end

-- this takes different parameters in different positions
local writeHead = function( self, cde, msg, hdr )
	if self.state > State.Zero    then error( "Can't set Head multiple times" ) end
	if not cde or not Status[cde] then error( "Must pass a valid status code" ) end
	self.statusCode    = cde
	self.statusMessage = 'string' == type( msg ) and msg or Status[ self.statusCode ]
	local headers      = 'table'  == type( msg ) and msg or hdr                 -- can be nil!
	if headers then
		if not self.headers then
			self.headers = headers
		else
			for k,v in headers do
				self.headers[ k ] = v
			end
		end
	end
	formHeader( self )
	self.state         = State.Written
	self.stream:addResponse( self )
end

local write = function( self, msg )
	if self.state < State.Written then
		formHeader( self, msg )
	else
		t_insert( self.buf, self.chunked and format( "%X\r\n%s\r\n", #msg, msg ) or msg )
	end
	self.state = State.Written
	self.stream:addResponse( self )
end

local finish = function( self, code, msg )
	if code and 'number' == type( code ) then
		self.statusCode    = code
		self.statusMessage = Status[ code ]
	else
		msg = code
	end
	if self.state < State.Written then
		self.contentLength = msg and #msg or 0
		formHeader( self, msg )
	else
		if msg and self.chunked then
			t_insert( self.buf, format( "%X\r\n%s\r\n0\r\n\r\n", #msg, msg ) )
		elseif msg then
			t_insert( self.buf, msg )
		elseif self.chunked then
			t_insert( self.buf, "0\r\n\r\n" )
		end
	end
	self.state = State.Done
	self.stream:addResponse( self )
end

-- return true if this response is done; else false
local send = function( self, sck )
	local buf = t_concat( self.buf )
	local snt = sck:send( buf )
	if snt == #buf and State.Done == self.state then
		return true
	elseif snt == #buf then
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
	__call   = function( self, stream, id, keepAlive, version, created )
		local response = {
			  stream        = stream
			, id            = id    -- StreamId
			, keepAlive     = keepAlive
			, state         = State.Zero
			, version       = version
			, chunked       = true
			, contentLength = nil
			, statusCode    = 200
			, statusMessage = Status[ 200 ]
			, created       = created
		}
		return setmetatable( response, _mt )
	end
} )

