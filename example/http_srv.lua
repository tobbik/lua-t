#!../out/bin/lua
Net      = require( 't.Net' )

s_port = 8003
answer = 'This is my answer'

srv,ip = Net.Socket.listen( s_port, 5 )
ipAdd, port   = ip:get( )
assert( ipAdd, port )
print( tostring(srv) .. " listening on '" ..ipAdd.. ":" ..port.. "' (" ..tostring( ip ).. ")..."  )
x=0
payload= 'HTTP/1.1 200 OK\r\r' ..
'Content-Length: ' .. #answer .. '\r\n' ..
'Date: Tue, 20 Jan 2015 20:56:55 GMT\r\n\r\n' ..
answer


while true do
	local cli, a   = srv:accept( )
	local msg, len = cli:recv( )
	print( 'FROM ' ..tostring( cli ).. " RECEIVED:  "..msg )
	while not msg:find( '\r\n\r\n' ) do
		--print( len, msg, string.byte(msg) )
		msg, len = cli:recv( )
	end
	len = cli:send( payload )
	print( '\tsend : ' ..len.. " BYTES VIA:  "..tostring( cli ) )
	cli:close( )
	collectgarbage( )
	x = x+1
	if 0==x%10 then print( "CC" ); collectgarbage() end
end
srv:close( )
