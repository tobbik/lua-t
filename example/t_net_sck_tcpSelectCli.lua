#!../out/bin/lua
Net,Buffer,fmt=require't.Net',require't.Buffer',string.format
ipAddr = arg[1] and arg[1] or Net.Interface( 'default' ).AF_INET.address:get()
port   = arg[2] and arg[2] or 8888

tcpsock, ip = Net.Socket.connect( ipAddr, port )
print( tcpsock, ip )

print( tcpsock.nonblock )
tcpsock.nonblock = true
print( tcpsock.nonblock )
conns = {client = tcpsock}
buf   = Buffer( string.rep( 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', 1234567 ) )
sent  = 0

while true do
	local rds, wds = Net.Socket.select( {}, conns )
	if wds.client then
		local snt = wds.client:send( Buffer.Segment( buf, sent+1 ) )
		if snt and snt>0 then
			sent = sent + snt
			print( "SENT:", snt, sent, #buf )
		else
			break
		end
	end
end
--tcpsock:close( )
