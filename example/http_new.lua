#!../out/bin/lua -i
Http,Loop =  require't.Http', require't.Loop'
fmt       = string.format
l         =  Loop(1200)

cb = function( stream )
	print( "Socket:",  msg.con.socket )
	print( "Address:", msg.con.ip )
	print( "Method:",  msg.method )
	print( "URL:",     msg.url )
	print( "VERSION:", msg.version )
	print( "Content-Length:",     #msg )
	if msg.query then
		print( "QUERY:" )
		for k,v in pairs( msg.query ) do print ('',k,v) end
	end
	print ("HEADERS:")
	for k,v in pairs( msg.header ) do print ('',k,'--------',v) end
	-- msg:sink('./theFile')
	print( 'REQ:', msg )
	--msg:write( "This is my answer" )
	--l:show()
	--d()
	msg:finish( )
end

h     = Http.Server( l, cb )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show( )

l:run( )
