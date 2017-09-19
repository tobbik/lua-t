#!../out/bin/lua
fmt    = string.format
Net,Buffer,Loop  = require't.Net',require't.Buffer',require't.Loop'
host   = arg[1] and arg[1] or Net.Interface( 'default' ).AF_INET.address.ip
port   = arg[2] and arg[2] or 8888
l      = Loop( 10 )
oCnt   = 512
str    = Buffer( "            " .. string.rep( '0123456789', (oCnt//10)-1 ) )
ofs,i  = 0,0
mSck   = Net.Socket( 'udp' )
sAdr   = Net.Address( host, port )
iAdr   = Net.Address()

read   = function( c )
	local msg,cnt = c:recv( iAdr )
	if msg then
		print( "GOT ECHO:", iAdr, cnt, msg:sub( 1, 10 ) )
	else
		print( "CLOSE", iAdr )
		l:removeHandle( c, 'read' )
		c:close()
		l:stop()
	end
end

write  = function( c )
	local wCnt = i<10000 and #str+5 or 0
	i = i+1
	local snt = c:send( sAdr, fmt( "%-5d     %s", i, str), wCnt )
	if snt then
		ofs = ofs + snt
		--print( "SENT: ", snt, ofs, bOut:read(1,6) )
	else
		print( "SENT SENTINEL", ofs )
		l:removeHandle( c, 'write' )
	end
end

print( mSck, sAdr, l, bOut )
mSck.nonblock = true
l:addHandle( mSck, 'write', write, mSck )
l:addHandle( mSck, 'read', read, mSck )
l:run( )


