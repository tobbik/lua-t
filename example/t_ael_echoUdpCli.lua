---
-- \file    example/t_ael_echoUdpCli.lua
--          A UDP echo Client sending UDP package with a payload with an
--          incremented number.

fmt    = string.format
Socket,Address,Interface,Buffer,Loop  = require't.Net.Socket', require't.Net.Address',require't.Net.Interface',require't.Buffer',require't.Loop'
host   = arg[1] and arg[1] or Interface.default( ).address.ip
port   = arg[2] and arg[2] or 8888
l      = Loop( 10 )
pyl    = string.rep( '0123456789', 40 )
oCnt,iCnt,i  = 0,0,0
mSck   = Socket( 'udp' )
sAdr   = Address( host, port )
iAdr   = Address( )

read   = function( c )
	local msg,cnt = c:recv( iAdr )
	if msg then
		iCnt = iCnt + cnt
		print( "GOT ECHO:", iCnt, msg:sub( 1, 20 ) )
	else
		print( "CLOSE", iAdr )
		l:removeHandle( c, 'read' )
		c:close()
		l:stop()
	end
end

write  = function( c )
	local wCnt = i<10000 and #pyl+5 or 0
	i = i+1
	local msg   = fmt( "%-5d     %s", i, pyl)
	local snt,e = c:send( msg, sAdr, wCnt )
	if snt and snt>0 then
		oCnt = oCnt + snt
		print( "SENT MESSAGE: ", oCnt, msg:sub(1,20) )
	else
		print( "SENT SENTINEL", oCnt, snt, e, c )
		l:removeHandle( c, 'write' )
	end
end

print( mSck, sAdr, l )
mSck.nonblock = true
l:addHandle( mSck, 'write', write, mSck )
l:addHandle( mSck, 'read', read, mSck )
l:run( )


