#!../out/bin/lua -i
t=require't'
l=t.Loop(50)
s=t.Socket.bind( 'UDP',  8888 )

x=function(req)
	print('REQ:', req)
end


cmd = function( sck )
	local msg, len, ip_cli = sck:recvFrom()
	local chunk = load( msg )
	if chunk then
		local done, msg = pcall( chunk )
		if not done then
			print( fmt('Execution failed: %s', msg ) )
		end
	else
		print( "Illegal code: syntax error" )
	end
end


l:addHandle( s, true, cmd, s )
h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip, s )

l:run()
