-- \file    research/http/http_perf.lua
-- \detail  Modified version of a nodeJS sample app translated to lua-t
--          -- NEVER USE THAT IN PRODUCTION. ROT47 IS NOT CRYPTO --

local Server,Loop           = require't.Http.Server', require't.Loop'
local fmt,t_insert,t_concat = string.format, table.insert, table.concat
local s_byte,s_char         = string.byte, string.char

-- curl -i -X GET "http://localhost:8002/newUser?username=username&password=password"
-- curl -i "http://localhost:8002/auth?username=username&password=password"
-- ab -k -c 20 -n 250 "http://localhost:8002/auth?username=username&password=password"

local users = { }  -- Users lookup  table
local r_cnt = 0    -- Counts all auth operations

local rot47 = function( wrd )
	local ret = { }
	for i=1, #wrd do
		local k = s_byte( wrd, i )
		--t_insert( ret, s_char( '!' + (k - '!' + 47) % 94 ) )
		t_insert( ret, s_char( 33 + (k + 14)%94 ) )
	end
	return t_concat( ret, '' )
end

local callback = function( req, res )
	if req.path == "/auth" then
		local username = req.query.username or ''
		local password = req.query.password or ''

		username = username:gsub( '[!@#$%%^&*]', '' )

		if not username or not password or not users[ username ] then
			res:finish( 400 )
		end

		if users[ username ] == rot47( password ) then
			r_cnt = r_cnt + 1
			res:finish( fmt( "%6d This user was authorized", r_cnt ) )
		else
			res:finish( 401, "Authorization failed" )
		end
	elseif req.path == "/newUser" then
		local username = req.query.username or ''
		local password = req.query.password or ''

		username = username:gsub( '[!@#$%%^&*]', '' )

		if not username or not password or users[ username ] then
			res:finish( 400, "Creating new user failed\n" )
		else
			users[ username ] = rot47( password ) ;
			print( fmt( "Created User -> %s:%s", username, users[ username ] ) );
			res:finish( "Created new user `" .. username .."`\n" );
		end
	else
		res:finish( 404, "There is nothing to do here\n");
	end
end

local httpServer = Server( Loop(), callback )

local host,port = '0.0.0.0',8000
if arg[ 1 ] then
	port = tonumber( arg[ 1 ] )
end
if arg[ 2 ] then
	host = arg[ 2 ]
end

--httpServer:on( 'connection', function( client )
--	print("Connected new connection:", client.descriptor)
--end )

local srv, adr   = httpServer:listen( host, port )
print( fmt( "Started `%s` at `%s` (%s)", srv, adr, srv.family ) )
httpServer.ael:run( )
