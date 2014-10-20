#!../out/bin/lua
xt = require("xt")

p = xt.Packer.Int(2)
print( p )

print( p:pack( 16706 ) )

print( p:unpack( 'ab' ) )

