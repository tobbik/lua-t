#!../out/bin/lua
Net,Buffer=require('t.Net'),require't.Buffer'
ipAddr = arg[2] and arg[2] or Net.Interface.default( ).AF_INET.address.ip
port   = arg[2] and arg[2] or 8888

if arg[ 1 ] == 'c' then
	sck,adr1 = Net.Socket.bind( ipAddr, 11111 )  -- control outgoing port for client
	adr2     = sck:connect( ipAddr, port )
else
	sck, adr2 = Net.Socket.connect( ipAddr, port )
end
print( sck, adr1, adr2 )

--buf  = Buffer( string.rep( '0123456789', 12345678 ) )
buf  = Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 123456 ) )
local snt = sck:send( buf )
assert( snt == #buf, "Send in one rush expected" )
print( "DONE", '\n', snt, "\n" )
sck:close( )
