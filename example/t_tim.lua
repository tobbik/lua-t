#!../out/bin/lua -i
Time = require't.Time'
fmt  = string.format

t1 = Time( 7500 )  -- create a 7.5s timer
t2 = Time( t1 )    -- clone this timer
print( t1, t2 );
t2:set( 2700 )       -- reset clones time to 2.7 seconds
t3 = t1 + t2         -- create timer as sum of t1 and t2
print( t1, t2, t3 );


tBefore = Time( )  -- create e timer with now()
print("Sleeping for:", t3 )
t3:sleep( )          -- sleep for t3 (10.2s)
print( "Before milli seconds: ", tBefore:get( ) )
tAfter = Time( )
print( "After milli seconds: ", tAfter:get( ) )
tSpan  = tAfter - tBefore
tBefore:since( )     -- reset to:  now() - Timer value
print( "Time spend on sleep measured: ", tBefore:get( ), tSpan:get( ) )
tBefore:now( )       -- reset to: now
print( "Now milli seconds: ", tBefore:get( ) )

