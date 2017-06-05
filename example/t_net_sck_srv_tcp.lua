#!../out/bin/lua
Net,Buffer,fmt = require't.Net',require't.Buffer',string.format
ipAddr,port    = Net.Interface( 'default' ).AF_INET.address:get(),8888

print( ipAddr, port)

--tcpsock = Net.Socket( 'TCP', 'ip4' )
--ip      = Net.Address( ipAddr, port )
tcpsock, ip = Net.Socket.listen( ipAddr, port, 5 )
--tcpsock:bind( ip )
--tcpsock,ip = Net.Socket.bind( ipAddr, port )
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
