#!../out/bin/lua -i
Server,Loop =  require't.Http.Server', require't.Loop'
fmt       = string.format
l         =  Loop( 1200 )

cb = function( req, res )
	print( "--------------------REQUEST", req  )
	for k,v in pairs(req) do print(k,v) end
	print( "--------------------RESPONSE", res  )
	for k,v in pairs(res) do print(k,v) end
	print( "Socket:",          req.stream.cli )
	print( "Address:",         req.stream.adr )
	if req.query then
		print( "--------------------------QUERY:" )
		for k,v in pairs( req.query ) do print ('',k,v) end
	end
	print ("----------------------HEADERS:")
	for k,v in pairs( req.headers ) do print ('',k,'--------',v) end
	-- msg:sink('./theFile')
	res:write( "This is the written() answer" )
	res:finish( "This is the end() of the Answer" )
	print( "--------------------REQUEST", req  )
	for k,v in pairs(req) do print(k,v) end
	print( "--------------------RESPONSE", res  )
	for k,v in pairs(res) do print(k,v) end
end

h     = Server( l, cb )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show( )

l:run( )
