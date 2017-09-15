#!../out/bin/lua -i
Net,Http,Loop=require't.Net', require't.Http', require't.Loop'
fmt=string.format
l=Loop(1200)
s=Net.Socket( 'UDP', 'ip4' )
s:bind( 8888 )

cb=function( msg )
	print ("Socket:",  msg.socket )
	print ("Address:", msg.ip )
	print ("Method:",  msg.method )
	print ("URL:",     msg.url )
	print ("VERSION:", msg.version )
	print ("Content-Length:",     #msg )
	if msg.query then
		print ("QUERY:")
		for k,v in pairs( msg.query ) do print ('',k,v) end
	end
	print ("HEADERS:")
	for k,v in pairs( msg.header ) do print ('',k,'--------',v) end
	-- msg:sink('./theFile')
	print( 'REQ:', msg )
	msg:write( "This is my answer" )
	--l:show()
	--d()
	msg:finish( )
end


d=function()
	local t=debug.getregistry()
	for i,v in ipairs( t ) do print( i,v ) end
end


cmd = function( sck )
	local ip_cli = Net.Address()
	local msg, len = sck:recv( ip_cli )
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


l:addHandle( s, 'read', cmd, s )
h=Http.Server( l, cb )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip, s )
sc:setOption( )

l:show()

l:run()
