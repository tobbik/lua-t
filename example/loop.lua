#!../out/bin/lua
xt = require'xt'
l  = nil
n  = 10
tm = {}

p = function( ... )
	print( table.unpack( {...} ) )
	return xt.Time( math.random( 1500, 11500 ) )
end


function r(s)
	local msg, len, ip_cli = s:recvFrom()
	print(msg, len, ip_cli, "\n")
	if msg:sub( 1, 4 ) == 'exit' then
		print( "go exit" )
		l:stop()
	elseif msg:sub( 1, 7 ) == 'remove ' then
		local t = msg:match("remove (%d+)")
		print( "remove timer ".. tonumber(t) )
		l:removeTimer( tm[ tonumber(t) ] )
	end
end

l   = xt.Loop( n )      ---< Loop with n slots for filehandles
for i=1,n do
	table.insert( tm, xt.Time( i*1000 + math.random( 100,950 ) ) )
	print( tm[i] )
	l:addTimer( tm[i], p, "----------------Timer " ..i.. "-----------------" )
end
s  = xt.Socket.bind( 'UDP', '192.168.0.219', 8888 )
l:addHandle( s, true, r, s )

l:run()
