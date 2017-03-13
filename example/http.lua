#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(1200)
s=t.Net.Socket( 'UDP', 'ip4' )
s:bind( 8888 )

x=function( msg )
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
	local msg, len, ip_cli = sck:recv()
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
sc:setOption( )

l:show()

l:run()
