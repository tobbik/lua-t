#!../out/bin/lua -i

-- spawn three HTTP Servers on 3 different sockets all using the same event loop
-- Added UDP Socket that accepts messages which are supposed to be Lua code and
-- then get executed in the global context.
-- You can use `udp_cmd_cli.lua` to send Lua code to test things out such as:
--   print( s00 )

Server,Socket,Address,Loop = require't.Http.Server',require't.Net.Socket',require't.Net.Address',require't.Loop'
fmt = string.format
l   = Loop(10)
l1  = "This is the first part before we finish. A string te repeated a couple of times\n"
l2  = "This is the finish line of the response.\n"
rp  = 30
rc  = 10
s00 = string.rep( l1, rp )
s   = Socket( 'UDP', 'AF_INET' )

x00=function( req, rsp )
	-- set 200 OK and set Content-Length Header explicitely
	rsp:writeHead( 200, (rc * #s00) + #l2 )
	for i=1,rc do
		rsp:write( s00 )
	end
	rsp:finish( l2 )
end

s01  = string.rep( l1, rp*rc )
x01=function( req, rsp )
	-- calling finish without any previous writeHead() or write() inferes
	-- Content-Length for the length of the argument to finish
	rsp:finish( s01 .. l2 )
end

-- load file relative to this file
fileName = debug.getinfo( 1, "S" ).short_src:match( "^(.*)/" ) .. '/t_oht.lua';
fSz = 1368;
ln  = 0;
x02 = function( req, rsp )
	rsp:writeHead( 200, fSz )
	local f = io.open( fileName, "r" )
	print( fileName, f )
	l:addHandle( f, 'read', function( ) -- this doesn't work if ael uses epoll
		local d = f:read( 200 )
		if d then
			print( f, #d )
			rsp:write( d )
		else
			print( f, "end" )
			l:removeHandle( f, 'read' )
			f:close()
			rsp:finish()
		end
	end )
end

-- read Lua code from udp socket and execute
cmd = function( sck )
	local adr_cli  = Address()
	local msg, len = sck:recv( adr_cli )
	print( "Received Command from:", adr_cli )
	local chunk,e  = load( msg )
	if chunk then
		local done, msg = pcall( chunk )
		if not done then
			print( fmt( 'Execution failed: %s', msg ) )
		end
	else
		print( "Illegal code: syntax error:", e )
	end
end

-- observe incoming UDP messages
s:bind( 8888 )
l:addHandle( s, 'read', cmd, s )


h00=Server( l, x00 )
h01=Server( l, x01 )
h02=Server( l, x02 )
sc00,ip00 = h00:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
sc01,ip01 = h01:listen( 8001, 10 )  -- listen on 0.0.0.0 INADDR_ANY
sc02,ip02 = h02:listen( 8002, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( "Listening on:", sc00, ip00 )
print( "Listening on:", sc01, ip01 )
print( "Listening on:", sc02, ip02 )

l:show( )
l:run( )
