#!../out/bin/lua
t,fmt       = require('t'),string.format
ipAddr,port = t.Net.Interface( 'default' ).address:get(),8888
Buffer      = t.Buffer

print( ipAddr, port)

--tcpsock = t.Net.Socket( 'TCP', 'ip4' )
--ip      = t.Net.IPv4( ipAddr, port )
tcpsock, ip = t.Net.Socket.listen( ipAddr, port, 5 )
--tcpsock:bind( ip )
--tcpsock,ip = t.Net.Socket.bind( ipAddr, port )
--tcpsock:listen( 5 )
-- --------------- or
for k,v in pairs(getmetatable(ip)) do print( k, v ) end
for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
print( tcpsock, ip )
clisock,cip = tcpsock:accept( )
length = 0
len    = 2
msg    = ''
buff   = Buffer(1024)
while msg do
	--msg, len = clisock:recv( buff )
	msg, len = clisock:recv( )
	length = length+len
	print( len, length, msg )
end
print( fmt( "DONE  From: %s Length: %d", cip, length ) )
--clisock:close( )
--tcpsock:close( )
