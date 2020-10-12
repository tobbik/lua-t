---
-- \file    example/t_ael_echoUdpSrv.lua
--          A UDP echo Server assuming each incoming UDP package has a payload
--          with an incremented number.  Keeps track of missing packages.

Socket,Address,Interface,Loop = require't.Net.Socket',require't.Net.Address',require't.Net.Interface',require't.Loop'
host     = Interface.default( ).address.ip
port     = 8888
l        = Loop( 10 )
msg      = nil
sSck     = Socket( 'udp' )
sAdr     = Address( host, port )
cAdr     = Address( )
rcv,lst,cnt = 0,0,0

echo = function( c, close )
	local snt = 0
	if close then
		snt  = c:send( '', cAdr, 0 )
		print( "GOT BURST:", cnt, lst, rcv )
		rcv,lst,cnt = 0,0,0
	else
		snt  = c:send( msg, cAdr )
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


