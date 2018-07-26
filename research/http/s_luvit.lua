-- \file    research/http/http_perf.lua
-- \detail  Modified version of a nodeJS sample app translated to lua-t
--          -- NEVER USE THAT IN PRODUCTION. ROT47 IS NOT CRYPTO --

local http = require('http')
local url  = require('url')

local fmt,t_insert,t_concat = string.format, table.insert, table.concat

-- curl -i -X GET "http://localhost:8002/newUser?username=username&password=password"
-- curl -i "http://localhost:8002/auth?username=username&password=password"
-- ab -k -c 20 -n 250 "http://localhost:8002/auth?username=username&password=password"

local users    = { }  -- Users lookup  table
local r_cnt    = 0    -- Counts all auth operations

local rot47 = function( pw )
	local ret = { }
	for i=1, #pw do
		var k = pw.charCodeAt( i );
		--t_insert( ret, s_char( '!' + (k - '!' + 47) % 94 ) )
		t_insert( ret, s_char( 33 + (k + 14)%94 ) )
	end
	return t_concat( ret, '' )
end

local callback = function( req, res )
	local uri = url.parse( req.url, true )
	if uri.pathname == "/auth" then
		local username = uri.query.username or ''
		local password = uri.query.password or ''

		username = username:gsub( '[!@#$%%^&*]', '' )

		if not username or not password or not users[ username ] then
			res:writeHead( 400 );
			res:finish( );
		end

		if users[ username ] == rot47( password ) then
			r_cnt = r_cnt + 1
			res:finish( fmt( "%6d This user was authorized", r_cnt ) )
		else
			res:writeHead( 401 );
			res:finish( "Authorization failed" )
		end
	elseif uri.pathname == "/newUser" then
		local username = uri.query.username or ''
		local password = uri.query.password or ''

		username = username:gsub( '[!@#$%%^&*]', '' )

		if not username or not password or users[ username ] then
			res:writeHead( 400 );
			res:finish( "Creating new user failed\n" )
		else
			users[ username ] = rot47( password ) ;
			print( fmt( "Created User -> %s:%s", username, users[ username ] ) );
			res:finish( "Created new user `" .. username.. "`\n" );
		end
	else
		res:writeHead( 404 );
		res:finish( "There is nothing to do here\n" );
	end
end

local httpServer = http.createServer( callback )

local host,port = '0.0.0.0',8000
if args[ 2 ] then
	port = tonumber( args[ 2 ] )
end
if args[ 3 ] then
	host = args[ 3 ]
end

local srv, adr   = httpServer:listen( port, host)
print( fmt( "Started Server on `%s:%d`", host, port ) )
