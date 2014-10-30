#!../out/bin/lua
xt=require'xt'
l = nil

p = function( ... )
	print( table.unpack( {...} ) )
	return xt.Time( 5000 )
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
tm3 = xt.Time( 3300 )
tm4 = xt.Time( 4600 )
tm5 = xt.Time( 5200 )
tm6 = xt.Time( 6100 )
tm7 = xt.Time( 7400 )
-- s  = xt.Socket.bind( 'UDP', '192.168.0.219', 8888 )
-- l:addHandle( s, true, r, s )
print('' , tm1, '\n', tm2, '\n', tm3, '\n', tm4, '\n', tm5, '\n', tm6, '\n', tm7)
l:addTimer( tm4, f(print, "-----------------Timer 4------------------------") )
l:addTimer( tm6, print, "-----------------Timer 6------------------------")
l:addTimer( tm7, print, "-----------------Timer 7------------------------")
l:addTimer( tm1, f(print, "-----------------Timer 1------------------------") )
l:addTimer( tm3, print, "-----------------Timer 3------------------------")
l:addTimer( tm2, p, "-----------------Timer 2------------------------" )
l:addTimer( tm5, print, "-----------------Timer 5------------------------")

l:run()
