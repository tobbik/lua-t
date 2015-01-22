#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(10)

x=function( msg )
	msg:write( "This is my answer" )
	msg:finish( )
end

h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show()
l:run()
