#!../out/bin/lua
xt = require("xt")

p = xt.Pack.IntB(2)
print( p )

print( p:pack( 16706 ) )

print( p:unpack( 'AB' ) )

