#!/usr/bin/lua
socket = require( 'socket' )
host   = "*"
s_port = 8003
answer = 'This is my answer'

srv = assert( socket.bind( host, s_port ) )
ipAdd, port   = srv:getsockname( )
assert( ipAdd, port )
print( tostring(srv) .. " listening on '" ..ipAdd.. ":" ..port.. "' ..."  )
x=0
payload= 'HTTP/1.1 200 OK\r\r' ..
'Content-Length: ' .. #answer .. '\r\n' ..
'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..
answer


while true do
	local cli, a   = srv:accept( )
	local msg, len = cli:receive( )
	print('FROM ' ..tostring( cli ).. " RECEIVED:  "..msg )
	while msg~='' do
		--print( len, msg, string.byte(msg) )
		msg, len = cli:receive( )
	end
	len = cli:send( payload )
	print( '\tsend : ' ..len.. " BYTES VIA:  "..tostring( cli ) .. "["..cli:getfd().."]" )
	cli:close( )
	collectgarbage( )
	x = x+1
	if 0==x%10 then print( "CC" );collectgarbage() end
end
srv:close()
