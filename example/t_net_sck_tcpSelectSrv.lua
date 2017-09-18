#!../out/bin/lua
Net,fmt=require('t.Net'),string.format
ipAddr,port=Net.Interface( 'default' ).AF_INET.address.ip,8888

tcpsock = Net.Socket( 'TCP' ) --ip4 implied
print( tcpsock.reuseaddr, tcpsock.reuseport )
tcpsock.reuseaddr = true
tcpsock.reuseport = true
print( tcpsock.reuseaddr, tcpsock.reuseport )
adr   = tcpsock:listen( ipAddr, port, 5 )
print( tcpsock, adr )
rcvd = 0

conns = {master = tcpsock}
while true do
	rds = Net.Socket.select( conns, {} )
	--print( #rds, tcpsock )
	if rds.master then
		local s,a = tcpsock:accept( )
		print( "MASTER ACCEPT:",s ,a )
		table.insert( conns, s )
	end

	-- ipairs() iterates over numeric indices only -> excluding master
	for n,cli in ipairs( rds ) do
		print( n, cli )
		msg, len = cli:recv( )
		if msg then
			rcvd = rcvd + len
			print( "RCVD:", len, rcvd )
		else
			for i,v in ipairs( conns ) do
				if v == cli then
					rcvd = 0;
					conns[i]:close( )
					table.remove( conns, i )
					print( '\tCLOSED: '.. tostring( v ) )
					break
				end
			end
		end
	end
end
tcpsock:close( )
