#!../out/bin/lua
t      = require't'
host   = t.Net.Interface( 'default' ).address:get( )
port   = 8888
l      = t.Loop( 10 )
bInc   = t.Buffer( 1024*20 )
iCnt   = 0

echo = function( c, close )
	if close then
		local snt  = c:send( '', 0 )
		print( "Close Client" )
		l:removeHandle( c, 'write' )
		c:shutdown( 'write' )
		c:close();
	else
		local snt  = c:send( bInc )
		print( "RSPD:", snt, iCnt, bInc:read(1,39) )
		l:removeHandle( c, 'write' )
	end
end

read = function( c )
	local rcvd,cnt = c:recv( bInc )
	if rcvd then
		iCnt = iCnt+cnt
		print( "RCVD:", cnt, iCnt, bInc:read(1,39) )
		l:addHandle( c, 'write', echo, c )
	else
		print("DONE___")
		l:removeHandle( c, 'read' )
		c:shutdown( 'read' )
		l:addHandle( c, 'write', echo, c, true )
		iCnt = 0
	end
end

accept = function( s )
	local c,cAdr = s:accept()
	c.nonblock = true
	print( c, cAdr )
	l:addHandle( c, 'read', read, c )
end

sSck,sAdr = t.Net.Socket.listen( host, port, 5 )
sSck.nonblock = true
print( sSck, sAdr, l )
l:addHandle( sSck, 'read', accept, sSck )
l:run( )


