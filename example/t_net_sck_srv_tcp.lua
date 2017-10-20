#!../out/bin/lua
Net,Buffer,fmt = require't.Net',require't.Buffer',string.format
ipAddr,port    = Net.Interface( 'default' ).AF_INET.address.ip, 8888

sck, adr      = Net.Socket.listen( ipAddr, port, 5 )
print( sck, adr )
clisck,cliadr = sck:accept( )
print( clisck, cliadr )
length = 0
len    = 2
msg    = ''
while msg do
	msg, len = clisck:recv( )
	length = length+len
	print( len, length, msg )
end
print( fmt( "DONE  From: %s Length: %d", cliadr, length ) )
clisck:close( )
sck:close( )
