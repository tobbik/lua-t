#!../out/bin/lua
t=require('t')
ipAddr,port='172.16.0.195',8888

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

msg = string.rep( '0123456789', 10010 )
sent = 0
print( tcpsock, sip, cip )
while sent<#msg do
	local snt = tcpsock:send( msg, sent )
	print ( "SNT:", snt )
	sent = sent + snt
	print( sent, #msg )
end
print( "DONE", '\n', sent, "\n" )
tcpsock:close( )
