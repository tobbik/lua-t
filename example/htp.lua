#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(10)
l1="The first part before we finish"
l2="This is my answer"

x=function( msg )
	--msg:writeHead( 200, #l2 )
	msg:write( l1 )
	msg:finish( l2 )
end

h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show()
l:run()
