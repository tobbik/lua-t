#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(10)
l1="This is the first part before we finish. A string te repeated soo man times\n"
l2="This is the finish line of the response.\n"
rp  = 30
rc  = 10
s00 = string.rep( l1, rp )

x00=function( msg )
	msg:writeHead( 200, (rc * #s00) + #l2 )
	for i=1,rc do
		msg:write( s00 )
	end
	msg:finish( l2 )
end


s01  = string.rep( l1, rp*rc )
x01=function( msg )
	--msg:writeHead( 200, #l2 + (rep*#rx) )
	msg:finish( s01 .. l2 )
end

fileName = 'buf.lua';
--stats    = fs.statSync( fileName );
fSz      = 2500;
ln       = 0;
x02 = function( msg )
	msg:writeHead( 200, fSz )
	local f = io.open( fileName, "r" )
	l:addHandle( f, true, function( )
		local d = f:read( 200 )
		if d then
			print( f, #d )
			msg:write( d )
		else
			print( f, "end" )
			msg:finish()
			l:removeHandle( f, true )
			f:close()
		end
	end )
	l:show()
end


h00=t.Http.Server( l, x00 )
h01=t.Http.Server( l, x01 )
h02=t.Http.Server( l, x02 )
sc00,ip00 = h00:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
sc01,ip01 = h01:listen( 8001, 10 )  -- listen on 0.0.0.0 INADDR_ANY
sc02,ip02 = h02:listen( 8002, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc00, ip00 )
print( sc01, ip01 )
print( sc02, ip02 )

l:show()
l:run()
