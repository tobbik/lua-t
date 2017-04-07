#!../out/bin/lua -i
Net,Loop,Http    = require't.Net',require't.Loop',require't.Http'
fmt  = string.format
l    = Loop( 1200 )
s    = Net.Socket( 'UDP' )
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
h     = Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip, s )
--for k,v in pairs( sc ) do print( k,v ) end
sc:setOption( )

l:show( )

l:run( )
