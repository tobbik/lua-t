#!../out/bin/lua -i
t  = require't'
fmt= string.format

t1 = t.Time( 7500 )  -- create a 7.5s timer 
t2 = t.Time( t1 )    -- clone this timer
print( t1, t2 );
t2:set( 2700 )       -- reset clones time to 2.7 seconds
t3 = t1 + t2         -- create timer as sum of t1 and t2
print( t1, t2, t3 );


tBefore = t.Time( )  -- create e timer with now()
t3:sleep( )          -- sleep for t3 (10.2s)
print( "Before micro seconds: ", tBefore:get( ) )
tAfter = t.Time( )
print( "After micro seconds: ", tAfter:get( ) )
tSpan  = tAfter - tBefore   
tBefore:since( )     -- reset to:  now() - Timer value
print( "Time spend on sleep measured: ", tBefore:get( ), tSpan:get( ) )
tBefore:now( )       -- reset to: now
print( "Now micro seconds: ", tBefore:get( ) )

