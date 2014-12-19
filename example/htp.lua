#!../out/bin/lua -i
t=require't'
l=t.Loop(50)

x=function(req)
	print('REQ:', req)
end

h=t.Http.Server( l, x )
sc,ip = h:listen( 8000, 10)
--h:listen('192.168.0.219', 8000, 10)
