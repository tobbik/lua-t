#!../out/bin/lua -i
t=require't'
l=t.Loop(50)
s=t.Socket.bind( 'UDP',  8888 )
o='./theFile'

x=function( msg )
	local t = msg:print()
	print( t[1] )
	for k,v in pairs( t[2]) do print (k,v) end
	print( t[3] )
	print( t[4] )
	-- msg:sink('./theFile')
	print( 'REQ:', msg )
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
