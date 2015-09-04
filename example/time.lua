#!../out/bin/lua -i
t  = require't'
fmt= string.format

t1 = t.Time( 500 )
t2 = t.Time( 2700 )

t3 = t1 + t2
tNow = t.Time( )
t3:sleep( )
print( "Now seconds: ", tNow:get( ) )
tNow:since( )
print( "time spend on sleep measured: ", tNow:get( ) )
tNow:now( )
print( "Now seconds: ", tNow:get( ) )

