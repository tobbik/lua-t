#!../out/bin/lua
Net,Loop = require't.Net',require't.Loop'
host     = Net.Interface( 'default' ).address:get( )
port     = 8888
l        = Loop( 10 )
msg      = nil
sSck     = Net.Socket( 'udp' )
sAdr     = Net.Address( host, port )
cAdr     = Net.Address( )
rcv,lst,cnt = 0,0,0

echo = function( c, close )
	local snt = 0
	if close then
		snt  = c:send( cAdr, '', 0 )
		print( "GOT BURST:", cnt, lst, rcv )
		rcv,lst,cnt = 0,0,0
	else
		snt  = c:send( cAdr, msg )
	end
	l:removeHandle( c, 'write' )
end

read = function( c )
	local rcvd = nil
	msg,rcvd   = c:recv( cAdr  )
	if msg then
		local num  = tonumber( msg:sub( 1, 6 ) )
		cnt = cnt + 1
		rcv = rcv+rcvd
		if lst+1 ~=num then
			print( "MISSED:", cnt, lst+1, num )
		end
		lst=num
		l:addHandle( c, 'write', echo, c )
	else
		l:addHandle( c, 'write', echo, c, true )
	end
end

print( sSck, sAdr, l )
sSck:bind( sAdr )
l:addHandle( sSck, 'read', read, sSck )
l:run( )


