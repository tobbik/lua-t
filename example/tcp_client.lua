#!../out/bin/lua
t=require('t')
ipAddr = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888

if arg[ 1 ] == "c" then
	tcpsock = t.Net.TCP( )
	sip     = t.Net.IPv4( ipAddr, port )
	cip     = t.Net.IPv4( ipAddr, 11111 )
	--for k,v in pairs(getmetatable(tcpsock)) do print( k, v ) end
	tcpsock:bind( cip )               -- control outgoing port for client
	tcpsock:connect( sip )
else
	tcpsock, sip = t.Net.TCP.connect( ipAddr, port )
end
print( tcpsock, sip, cip )

buf  = t.Buffer( string.rep( '0123456789', 12345678 ) )
sent = 0
while sent<#buf do
	local snt = tcpsock:send( buf, sent )
	print ( "SNT:", snt )
	sent = sent + snt
	print( sent, #buf )
end
print( "DONE", '\n', sent, "\n" )
tcpsock:close( )
buf = nil
collectgarbage()
