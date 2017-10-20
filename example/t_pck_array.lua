Buffer = require't.Buffer'
Pack   = require't.Pack'

b=Buffer( 'ABCDEFGH' )

a=Pack( '>I2', 4)

x = a( b )  -- create a result array

for i,v in pairs(x) do print( i, v, a[i](b) ) end
