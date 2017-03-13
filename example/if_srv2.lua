#!../out/bin/lua
local t=require('t')
sip,sport=t.Net.Interface( 'default' ).address:get(),8888

tcpsock,ip = t.Net.Socket.listen( sip, sport, 5 )
--tcpsock:listen(5)
print( tcpsock, ip )
a=0

conns  = { master=tcpsock }
while a<100 do
	res = t.Net.selectK( conns, {} )
	io.write( 'LOOP['..#res..']:' )
	for n,cli in pairs( res ) do
		io.write( '  '..tostring( cli )..'' )
	end
	print( )

	for n,cli in pairs( res ) do
		if cli == tcpsock then
			local s,a =  tcpsock:accept( )
			conns [ tostring( a ) ] = s
			--table.insert(conns, s)
			print( '\tCONNECT: ' ..tostring( s ) .. " FROM:  "..tostring( a ) )
		else
			msg, len = cli:recv( )
			if len<1 then
				for i,v in pairs( conns ) do
					if v == cli then
						conns[i]=nil
						--table.remove(conns, i)
						print( '\tCLOSED: '.. tostring(v) )
					end
				end
			else
				print( '\tRECIEVED['..len..']: '..msg )
			end
		end
	end
	a=a+1
end
tcpsock:close( )
