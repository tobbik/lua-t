#!../out/bin/lua
fmt    = string.format
t      = require't'
host   = arg[1] and arg[1] or t.Net.Interface( 'default' ).address:get()
port   = arg[2] and arg[2] or 8888
l      = t.Loop( 10 )
oCnt   = 512
str    = t.Buffer( "            " .. string.rep( '0123456789', (oCnt//10)-1 ) )
ofs,i  = 0,0
mSck   = t.Net.Socket( 'udp' )
sAdr   = t.Net.IPv4( host, port )
iAdr   = t.Net.IPv4()

read   = function( c )
	local msg,cnt = c:recv( iAdr )
	if msg then
		print( "GOT ECHO:", iAdr, cnt, msg:sub( 1, 10 ) )
	else
		print( "CLOSE", iAdr )
		l:removeHandle( c, true )
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
		l:removeHandle( c, false )
	end
end

print( mSck, sAdr, l, bOut )
mSck.nonblock = true
l:addHandle( mSck, false, write, mSck )
l:addHandle( mSck, true, read, mSck )
l:run( )


