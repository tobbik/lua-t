-- \file    research/http/http_perf.lua
-- \detail  Modified version of a nodeJS sample app translated to lua-t
--          -- NEVER USE THAT IN PRODUCTION. ROT47 IS NOT CRYPTO --

local Server,Loop       = require't.Http.Server', require't.Loop'
local t_insert,t_concat = table.insert, table.concat
local s_byte,s_char     = string.byte, string.char

-- curl -i -X GET "http://localhost:8002/newUser?username=username&password=password"
-- curl -i "http://localhost:8002/auth?username=username&password=password"
-- ab -k -c 20 -n 250 "http://localhost:8002/auth?username=username&password=password"
--
--
local payload  = ("This is a simple dummy load that is meant to generate some load"):rep( 10 )

local getUsers = function(n)
	local users   = {};
	local getWord = function()
		local wrd = {}
		for i=1,math.random(6,12) do
			t_insert( wrd, string.char( math.random( 32, 123 ) ) )
		end
		return t_concat( wrd,'' )
	end
	for x=1,n do
		users[ getWord() ] = getWord()
	end
	return users
end

local users = getUsers(5000)  -- Users lookup  table
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
			return res:finish( 400, "Server error bad arguments or empty result" )
		end

		if users[ username ] == rot47( password ) then
			r_cnt = r_cnt + 1
			res.headers = { [ "Content-Type" ] = "text/plain; charset=utf-8" }
			res:finish( ("%7d This user was authorized at %d\n"):format( r_cnt, Loop.time( ) ) )
		else
			res:finish( 401, "Authorization failed" )
		end
	elseif req.path == "/newUser" then
		local username = req.query.username or ''
		local password = req.query.password or ''

		username = username:gsub( '[!@#$%%^&*]', '' )

		if not username or not password or users[ username ] then
			return res:finish( 400, ("Creating a new user failed -> %s\n"):format( users[ username ] and "User already exists" or "Insufficient arguments" ) )
		end
		users[ username ] = rot47( password ) ;
		print( ("Created User -> %s:%s"):format( username, users[ username ] ) );
		return res:finish( "Created new user `" .. username .."`\n" );
	elseif req.path == "/multi" then
		local multiplier = req.query.multiplier and tonumber(req.query.multiplier) or nil

		if not multiplier then
			return res:finish( 400, "Server error bad arguments" )
		else
			return res:finish( 200, payload:rep(multiplier) )
		end
	else
		return res:finish( 404, "There is nothing to do here\n");
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

--httpServer:on( 'connection', function( stream )
--	print("Connected new connection:", stream.socket.descriptor)
--end )

local srv, adr   = httpServer:listen( host, port )
print( ("Started `%s` at `%s` (%s)"):format( srv, adr, srv.family ) )
httpServer.ael:run( )
