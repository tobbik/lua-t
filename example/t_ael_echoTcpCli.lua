#!../out/bin/lua
Net,Buffer,Loop  = require't.Net',require't.Buffer',require't.Loop'
host   = arg[1] and arg[1] or Net.Interface( 'default' ).AF_INET.address:get()
port   = arg[2] and arg[2] or 8888
l      = Loop( 10 )
bOut   = Buffer( string.rep( '0123456789', 1234567 ) )
bInc   = Buffer( 1024*10 )
ofs    = 0

read   = function( c )
	local rcvd,cnt = c:recv( bInc )
	if rcvd then
		print( "GOT ECHO:", cnt, bInc:read( 1,40 ) )
	else
		print( "REMOVE HANDLE" )
		l:removeHandle( c, 'read' )
		print( "CLOSE" )
		c:close()
		l:stop()
	end
end

write  = function( c )
	local seg = Buffer.Segment( bOut, ofs+1 );
	local snt = c:send( seg )
	if snt then
		ofs = ofs + snt
		print( "SENT: ", snt, ofs, seg:read(1,40) )
	else
		l:removeHandle( c, 'write' )
		print( "DONE SENDING", ofs )
		c:shutdown( 'write' )
	end
end

cSck,cAdr = Net.Socket.connect( host, port )
cSck.nonblock = true
print( cSck, cAdr, l, bOut )
l:addHandle( cSck, 'write', write, cSck )
l:addHandle( cSck, 'read', read, cSck )
l:run( )


