#!../out/bin/lua -i
t=require't'
fmt=string.format
l=t.Loop(10)
l1="This is the first part before we finish"
l2="This is my answer"

x=function( msg )
	msg:writeHead( 200, #l2+(10*#l1) )
	msg:write( l1 )
	print(1)
	msg:write( l1 )
	print(2)
	msg:write( l1 )
	print(3)
	msg:write( l1 )
	print(4)
	msg:write( l1 )
	print(5)
	msg:write( l1 )
	print(6)
	msg:write( l1 )
	print(7)
	msg:write( l1 )
	print(8)
	msg:write( l1 )
	print(9)
	msg:write( l1 )
	print(10)
	msg:finish( l2 )
	print('DONE')
	--l:show()
end

h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10 )  -- listen on 0.0.0.0 INADDR_ANY
print( sc, ip )

l:show()
l:run()
