#!../out/bin/lua
t      = require't'
ipAddr = t.Net.Interface( 'default' ).address:get()
port   = 8888
s      = t.Net.TCP.bind( ipAddr, port )
l      = t.Loop( 10 )

read = function( c )
	local m,cnt = c:recv()
	print(cnt, m)
	if not m then
		l:removeHandle( c, true )
		c:close()
	end
end

accept = function( s )
	local c,ip = s:accept()
	print( c, ip )
	l:addHandle( c, true, read, c )
end

create_server = function()
	l:addHandle( s, true, accept, s )
	s:listen( 5 )
end

create_server()
l:run( )


