#!../out/bin/lua
local t=require('t')
local ipadd,port='10.128.3.145',8888

tcpsock = t.Net.TCP( )
sip     = t.Net.IPv4( ipadd, port )
cip     = t.Net.IPv4( ipadd, 54321 )
tcpsock:bind( cip )               -- control outgoing port for client
tcpsock:connect( sip )
-- --------------- or 
--tcpsock, sip = t.Net.TCP.connect( ipadd, port )

print( tcpsock, sip, cip )
msg = string.rep( '0123456789', 1000010 )
sent = 0
print( tcpsock, sip )
while sent<#msg do
	local snt = tcpsock:send( msg, sent )
	print ( "SNT:", snt )
	sent = sent + snt
	print( sent, #msg )
end
print( "DONE", '\n', sent, "\n" )
tcpsock:close( )
