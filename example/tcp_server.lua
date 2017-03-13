#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port=t.Net.Interface( 'default' ).address:get(),8888
Buffer = t.Buffer

print( ipAddr, port)

tcpsock = t.Net.Socket( 'ip4', 'TCP' )
ip      = t.Net.IPv4( ipAddr, port )
tcpsock:bind( ip )
tcpsock:listen( 5 )
-- --------------- or
--tcpsock,ip = t.Net.TCP.bind( ipAddr, port )
-- --------------- or
--tcpsock, ip = t.Net.TCP.listen( ipAddr, port, 5 )
--for k,v in pairs(getmetatable(ip)) do print( k, v ) end
--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
print( tcpsock, ip )
consock,cip = tcpsock:accept( )
length = 0
len    = 2
msg    = ''
buff   = Buffer(1024)
while msg do
	msg, len = consock:recv( buff )
	length = length+len
	print( len, length )
end
print( fmt( "DONE  From: %s Length: %d", cip, length ) )
consock:close( )
tcpsock:close( )
