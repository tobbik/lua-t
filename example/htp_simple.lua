#!../out/bin/lua -i
t    = require't'
fmt  = string.format
l    = t.Loop( 1200 )
s    = t.Net.Socket( 'ip4', 'UDP' )
s:bind( 8888 )
a    = "This is my Answer"

x=function( stream )
	--print( "STREAM",          stream )
	--print( "Socket:",         stream.connection.socket )
	--print( "Address:",        stream.connection.ip )
	--print( "Method:",         stream.method )
	--print( "URL:",            stream.url )
	--print( "VERSION:",        stream.version )
	--print( "Content-Length:", #stream )
	--if stream.query then
	--	print ("QUERY:")
	--	for k,v in pairs( stream.query ) do print( '', k, v ) end
	--end
	--print ("HEADERS:")
	--for k,v in pairs( stream.header ) do print( '', k, '--------', v ) end
	-- stream:sink('./theFile')
	--stream:writeHead( 200, #a )
	--stream:write( a )
	stream:finish( a )
	--stream:finish( )
	--l:show()
	--d()
end


d=function( )
	local t = debug.getregistry( )
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
h     = t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip, s )
--for k,v in pairs( sc ) do print( k,v ) end
sc:setOption( )

l:show( )

l:run( )
