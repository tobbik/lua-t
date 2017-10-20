---
-- \file       examples/t_ael_loop.lua
--             Push a few random timers on the loop that refresh themselves by
--             returning another timer.  If the random value is under a
--             treshhold it returns null, thus removing this timer from the
--             loop.  Allows to introspect whats going on, by sending commands
--             via UDP socket. Use udp_cmd_cli.lua to do so.

Net,Loop,Time  = require't.Net',require't.Loop',require't.Time'
s  = Net.Socket( 'UDP', 'ip4' )
s:bind( 8888 )
l  = nil
n  = 9
tm = { }

p = function( str, t )
	t:set( math.random( n*35, n*3000 ) )
	print( str, 'return   ' .. tostring( (t:get( ) > n*200) and t:get( ) or 0 ), t )
	return (t:get( ) > n*200) and t or nil
end

dR = function()
	local d = debug.getregistry()
	-- cheap way to find curried sockets and functions
	for i=#d,3,-1 do
		if type(d[i]) =='table' then
			print( "TABLE", i, d[i] )
			for k,v in pairs(d[i]) do print('',k,v) end
		end
	end
end


function r( s )
	local ip_cli   = Net.Address()
	local msg, len = s:recv( ip_cli )
	print( msg, len, ip_cli, "\n" )
	if msg:sub( 1, 4 ) == 'exit' then
		print( "go exit" )
		l:stop( )
	elseif msg:sub( 1, 8 ) == 'show' then
		print( "SHOW LOOP:" )
		l:show( )
	elseif msg:sub( 1, 7 ) == 'remove ' then
		local t = msg:match( "remove (%d+)" )
		print( "remove timer ".. tonumber( t ) )
		l:removeTimer( tm[ tonumber( t ) ] )
	elseif msg:sub( 1, 2 ) == 'rL' then
		dR()
		l:removeHandle( s, 'read' )
		print( "remove listener -> go exit" )
		dR()
		l:stop( )
	elseif msg:sub( 1, 3 ) == 'reg' then
		dR()
	end
end

l   = Loop( 3 )      ---< Loop with n slots for filehandles
for i=1,n do
	table.insert( tm, Time( math.random( n*35, n*3000 ) ) )
	print( tm[i] )
	l:addTimer( tm[i], p, "-------Timer " ..i.. "--------", tm[i] )
end
l:addHandle( s, 'read', r, s )

l:run( )
