#!../out/bin/lua
t,fmt=require('t'),string.format
ipAddr,port=t.Net.Interface( 'default' ).address:get(),8888

tcpsock, ip = t.Net.Socket.listen( ipAddr, port, 5 )
print( tcpsock, ip )

conns = {master = tcpsock}
while true do
	res = t.Net.Socket.select( conns, {} )
	--print( #res, tcpsock )
	if res.master then
		local s,a =  tcpsock:accept( )
		print( "MASTER ACCEPT:",s ,a )
		table.insert( conns, s )
	end

	-- ipairs() iterates over numeric indices only -> excluding master
	for n,cli in ipairs( res ) do
		print( n, cli )
		msg, len = cli:recv( )
		if msg then
			print( msg, len, ip )
		else
			for i,v in ipairs( conns ) do
				if v == cli then
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
