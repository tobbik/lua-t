#!../out/bin/lua
xt=require'xt'
l = nil

p = function( ... )
	print( table.unpack( {...} ) )
	return xt.Time( 2500 )
end

f = function(func, ...)
	local args = {...}
	return function()
		return func( table.unpack( args ) )
	end
end

function r(s)
	local msg, len, ip_cli = s:recvFrom()
	print(msg, len, ip_cli, "\n")
	if msg:sub( 1, 4 ) == 'exit' then print( "go exit" ) ;l:stop() end
end

l   = xt.Loop( 40 )
tm1 = xt.Time( 1500 )
tm2 = xt.Time( 2700 )
-- s  = xt.Socket.bind( 'UDP', '192.168.0.219', 8888 )
-- l:addHandle( s, true, r, s )
print( tm1, tm2 )
l:addTimer( tm1, p, 1,2,3,4,5,6,7 )
l:addTimer( tm2, f(print,8,7,6,5,4,3,2,1) )

l:run()
