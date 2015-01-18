#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(50)
s=t.Socket.bind( 'UDP',  8888 )
o='./theFile'

x=function( msg )
	print ("Socket:",  msg.socket )
	print ("Address:", msg.ip )
	print ("URL:",     msg.url )
	print ("QUERY:")
	for k,v in pairs( msg.query ) do print ('',k,v) end
	print ("HEADERS:")
	for k,v in pairs( msg.header ) do print ('',k,'--------',v) end
	-- msg:sink('./theFile')
	print( 'REQ:', msg )
	msg:write( "This is my answer" )
	msg:finish( )
end


d=function()
	local t=debug.getregistry()
	for i,v in ipairs( t ) do print( i,v ) end
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

l:show()

l:run()
