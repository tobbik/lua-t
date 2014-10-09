#!../out/bin/lua
xt = require("xt")


p = xt.Packer.Int(1)
print( p )

b = xt.Buffer(14)
print( b:toHex() )

p:attach( b, 4 )

p:write( 144 )

print( b:toHex() )
