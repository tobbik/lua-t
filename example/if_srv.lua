#!../out/bin/lua
Net=require('t.Net')
sip,sport=Net.Interface( 'default' ).address:get(),8888

tcpsock,ip = Net.Socket.listen( sip, sport, 5 )
-- tcpsock:listen( 5 )
print( tcpsock, ip )

conns  = { tcpsock }
while true do
	res = Net.Socket.select( conns, {} )
	io.write('LOOP['..#res..']:')
	for n,cli in ipairs( res ) do
		io.write( '  '..tostring( cli )..'' )
	end
	print( )

	for n,cli in ipairs( res ) do
		if cli == tcpsock then
			print( 'it\'s a client socket' )
			local s,a =  tcpsock:accept( )
			table.insert( conns, s )
			print( '\tCONNECT: ' ..tostring( s ).. " FROM:  "..tostring( a ) )
		else
			msg, len = cli:recv( )
			if len<1 then
				for i,v in ipairs( conns ) do
					if v == cli then
						conns[i]:close( )
						table.remove( conns, i )
						print( '\tCLOSED: '.. tostring( v ) )
					end
				end
			else
				print( '\tRECIEVED['..len..']: '..msg )
			end
		end
	end
end
tcpsock:close()
