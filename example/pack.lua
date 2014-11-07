#!../out/bin/lua
xt = require("xt")

p = xt.Pack.IntB(2)
bt = xt.Pack.Byte()
nl = xt.Pack.Nibble('l')
nh = xt.Pack.Nibble('h')

print( p, nl, nh )

print( p:pack( 16706 ) )

print( p:unpack( 'AB' ) )

print( string.format("%02X", nl:pack( 0xA ):byte(1) ) )
print( string.format("%02X", nh:pack( 0x6 ):byte(1) ) )

print( nl:unpack( string.char(171) ) )     -- 0xAB
print( nh:unpack( string.char(171) ) )



