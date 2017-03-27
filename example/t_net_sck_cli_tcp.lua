#!../out/bin/lua
t=require('t')
ipAddr = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888

--if arg[ 3 ] == c then
--	tcpsock = t.Net.Socket( ) -- implicit TCP and ip4
--	sip     = t.Net.IPv4( ipAddr, port )
--	cip     = t.Net.IPv4( ipAddr, 11111 )
--	--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
--	tcpsock:bind( cip )               -- control outgoing port for client
--	tcpsock:connect( sip )
--else
--	tcpsock, sip = t.Net.Socket.connect( ipAddr, port )
--end
tcpsock, sip = t.Net.Socket.connect( ipAddr, port )
print( tcpsock, sip, cip )

--buf  = t.Buffer( string.rep( '0123456789', 12345678 ) )
buf  = t.Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 1234567 ) )
sent = 0
while sent<#buf do
	local suc,snt = tcpsock:send( t.Buffer.Segment( buf, sent+1 ) )
	print ( "SUC SNT:", suc, snt )
	sent = sent + snt
	print( sent, #buf )
end
print( "DONE", '\n', sent, "\n" )
tcpsock:close( )
buf = nil
collectgarbage()
