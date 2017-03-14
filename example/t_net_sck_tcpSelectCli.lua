#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888

tcpsock, ip = t.Net.Socket.connect( ipAddr, port )
print( tcpsock, ip )

print( tcpsock.nonblock )
tcpsock.nonblock = true
print( tcpsock.nonblock )
conns = {client = tcpsock}
buf   = t.Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 1234567 ) )
sent  = 0

while true do
	local rds, wds = t.Net.Socket.select( {}, conns )
	if wds.client then
		local snt = wds.client:send( nil, buf, sent )
		if snt>0 then
			sent = sent + snt
			print( "SENT:", snt, sent, #buf )
		else
			break
		end
	end
end
--tcpsock:close( )
