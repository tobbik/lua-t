#!../out/bin/lua
xt = require'xt'
s  = xt.Socket.bind( 'UDP',  8888 )
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
	elseif msg:sub( 1, 8 ) == 'show' then
		print( "SHOW LOOP:" )
		l:show( )
	elseif msg:sub( 1, 7 ) == 'remove ' then
		local t = msg:match("remove (%d+)")
		print( "remove timer ".. tonumber(t) )
		l:removeTimer( tm[ tonumber(t) ] )
	elseif msg:sub( 1, 2 ) == 'rL' then
		l:removeHandle( s )
		print( "remove listener -> go exit" )
		l:stop()
	end
end

l   = xt.Loop( n )      ---< Loop with n slots for filehandles
for i=1,n do
	table.insert( tm, xt.Time( math.random( 950, n*1000 ) ) )
	print( tm[i] )
	l:addTimer( tm[i], p, "----------------Timer " ..i.. "-----------------" )
end
l:addHandle( s, true, r, s )

l:run()
