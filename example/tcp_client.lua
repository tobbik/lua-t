#!../out/bin/lua
Net,Buffer=require('t.Net'), require't.Buffer'
ipAddr = arg[1] and arg[1] or Net.Interface( 'default' ).AF_INET.address.ip
port   = arg[2] and arg[2] or 8888

if arg[ 1 ] then
	tcpsock = Net.Socket( 'TCP', 'ip4' )
	print( ipAddr, port );
	sip     = Net.IPv4( ipAddr, port )
	cip     = Net.IPv4( ipAddr, 11111 )
	--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
	tcpsock:bind( cip )               -- control outgoing port for client
	tcpsock:connect( sip )
else
	tcpsock, sip = Net.Socket.connect( ipAddr, port )
end
print( tcpsock, sip, cip, tcpsock.nonblock )

buf  = Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 123456 ) )
local snt = tcpsock:send( buf )
assert( snt == #buf, "Should have blockingly sent in one rush" )
print( "DONE", '\n', snt, "\n" )
tcpsock:close( )
buf = nil
collectgarbage()
