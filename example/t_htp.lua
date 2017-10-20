#!../out/bin/lua -i
Server,Method,Loop =  require't.Http.Server', require't.Http.Method', require't.Loop'
fmt         = string.format
l           =  Loop( 1200 )

cb = function( req, res )
	print( "--------------------REQUEST", req  )
	for k,v in pairs(req) do print(k,v) end
	print( "--------------------RESPONSE", res  )
	for k,v in pairs(res) do print(k,v) end
	print( "--------------------OTHER THINGS EXPLICITELY", res  )
	print( "Socket:",         req.stream.cli )
	print( "Address:",        req.stream.adr )
	print( "Method:",         Method[ req.method ] )
	print( "URL:",            req.url )
	print( "VERSION:",        req.version )
	print( "Content-Length:", req.contentLength )
	if req.query then
		print( "--------------------------QUERY:" )
		for k,v in pairs( req.query ) do print ('',k,v) end
	end
	print ("----------------------HEADERS:")
	for k,v in pairs( req.headers ) do print ('',k,'--------',v) end
	res:write( "This is the written() answer" )
	res:finish( "This is the finish() of the Answer" )
	l:show( )
end

h     = Server( l, cb )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show( )

l:run( )
