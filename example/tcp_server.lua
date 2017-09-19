#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).AF_INET.address.ip,8888
Buffer = require't.Buffer'

print( ipAddr, port)

--tcpsock = Net.Socket( 'TCP', 'ip4' )
--ip      = Net.IPv4( ipAddr, port )

--tcpsock:bind( ip )
--tcpsock:listen( ip, 5 )
-- --------------- or
tcpsock,ip = Net.Socket.bind( ipAddr, port )
-- --------------- or
--tcpsock, ip = Net.TCP.listen( ipAddr, port, 5 )
--for k,v in pairs(getmetatable(ip)) do print( k, v ) end
--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
print( tcpsock, ip )
tcpsock:listen(5)
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
