#!../out/bin/lua
t      = require't'
ipAddr = t.Net.Interface( 'default' ).address:get()
port   = 8888
s      = t.Net.TCP( )
l      = t.Loop( 10 )
buf    = t.Buffer( string.rep( '0123456789', 12345678 ) )
ofs    = 1

write  = function( c )
	local snt = c:send( buf, ofs )
	ofs = ofs + snt
	print(snt, ofs)
	if 0==snt then
		l:removeHandle( c, true )
		c:close()
	end
end

connect = function( s )
	local c,ip = s:connect( ipAddr, port )
	print( c, ip )
	l:addHandle( c, true, write, c )
end

create_client = function()
	l:addHandle( s, true, connect, s )
end

create_client()
l:run( )


