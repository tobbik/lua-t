#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port='172.16.0.195',8888

tcpsock = t.Net.TCP( )
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
while len>1 do
	msg, len = consock:recv( )
	print( len, length )
	length = length+len
end
print( fmt( "DONE  From: %s Length: %d", cip, length ) )
consock:close( )
tcpsock:close( )
