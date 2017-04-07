#!../out/bin/lua
Net,Buffer=require('t.Net'),require't.Buffer'
ipAddr = arg[1] and arg[1] or Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888

--if arg[ 3 ] == c then
--	tcpsock = Net.Socket( ) -- implicit TCP and ip4
--	sip     = Net.IPv4( ipAddr, port )
--	cip     = Net.IPv4( ipAddr, 11111 )
--	--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
--	tcpsock:bind( cip )               -- control outgoing port for client
--	tcpsock:connect( sip )
--else
--	tcpsock, sip = Net.Socket.connect( ipAddr, port )
--end
tcpsock, sip = Net.Socket.connect( ipAddr, port )
print( tcpsock, sip, cip )

--buf  = Buffer( string.rep( '0123456789', 12345678 ) )
buf  = Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 123456 ) )
local snt = tcpsock:send( buf )
assert( snt == #buf, "Send in one rush expected" )
print( "DONE", '\n', snt, "\n" )
tcpsock:close( )
buf = nil
collectgarbage()
