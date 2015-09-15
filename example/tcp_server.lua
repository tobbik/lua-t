#!../out/bin/lua
local t,fmt=require('t'),string.format
local ipadd,port='10.128.3.145',8888

--tcpsock = t.Net.TCP( )
--ip      = t.Net.IPv4( ipadd, port )
--tcpsock:bind( ip )
-- --------------- or 
--tcpsock,ip = t.Net.TCP.bind( ipadd, port )
-- --------------- or 
tcpsock, ip = t.Net.TCP.listen( ipadd, port, 5 )
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
