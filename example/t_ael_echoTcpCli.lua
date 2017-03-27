#!../out/bin/lua
t      = require't'
host   = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888
l      = t.Loop( 10 )
bOut   = t.Buffer( string.rep( '0123456789', 1234567 ) )
bInc   = t.Buffer( 1024*10 )
ofs    = 0

read   = function( c )
	local rcvd,cnt = c:recv( bInc )
	if rcvd then
		print( "GOT ECHO:", cnt, bInc:read( 1,40 ) )
	else
		print( "REMOVE HANDLE" )
		l:removeHandle( c, true )
		print( "CLOSE" )
		c:close()
		l:stop()
	end
end

write  = function( c )
	local seg = t.Buffer.Segment( bOut, ofs+1 );
	local snt = c:send( seg )
	if snt then
		ofs = ofs + snt
		print( "SENT: ", snt, ofs, seg:read(1,40) )
	else
		l:removeHandle( c, false )
		print( "DONE SENDING", ofs )
		c:shutdown( 'write' )
	end
end

cSck,cAdr = t.Net.Socket.connect( host, port )
cSck.nonblock = true
print( cSck, cAdr, l, bOut )
l:addHandle( cSck, false, write, cSck )
l:addHandle( cSck, true, read, cSck )
l:run( )


