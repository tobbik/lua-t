#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(10)
l1="This is the first part before we finish. A string te repeated soo man times\n"
l2="This is the finish line of the response.\n"
rep=3000
rx = string.rep( l1, rep )

x=function( msg )
	--msg:writeHead( 200, #l2 + (rep*#rx) )
	--msg:write( l1 )
	msg:finish( rx .. l2 )
	--l:show()
end

h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show()
l:run()
