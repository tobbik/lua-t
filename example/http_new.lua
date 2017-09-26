#!../out/bin/lua -i
Server,Loop =  require't.Http.Server', require't.Loop'
fmt       = string.format
l         =  Loop( 1200 )

cb = function( req, res )
	print( "Socket:",             req.stream.cli )
	print( "Address:",            req.stream.adr )
	print( "Method:",             req.method )
	print( "URL:",                req.url )
	print( "VERSION:",            req.version )
	print( "Content-Length:",     #req )
	if req.query then
		print( "QUERY:" )
		for k,v in pairs( req.query ) do print ('',k,v) end
	end
	print ("HEADERS:")
	for k,v in pairs( req.headers ) do print ('',k,'--------',v) end
	-- msg:sink('./theFile')
	print( 'REQ:', req )
	--msg:write( "This is my answer\r" )
	res:finish( "This is the End of the Answer\r" )
end

h     = Server( l, cb )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show( )

l:run( )
