#!../out/bin/lua
xt = require("xt")
fmt= string.format

b1 = xt.Pack.Bit(1)
b2 = xt.Pack.Bit(2)
b3 = xt.Pack.Bit(3)
b4 = xt.Pack.Bit(4)
b5 = xt.Pack.Bit(5)
b6 = xt.Pack.Bit(6)
b7 = xt.Pack.Bit(7)
b8 = xt.Pack.Bit(8)
p  = xt.Pack.IntB(2)
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


print( b1, fmt("%02X", b1:pack( true ):byte(1) ) )
print( b2, fmt("%02X", b2:pack( true ):byte(1) ) )
print( b3, fmt("%02X", b3:pack( true ):byte(1) ) )
print( b4, fmt("%02X", b4:pack( true ):byte(1) ) )
print( b5, fmt("%02X", b5:pack( true ):byte(1) ) )
print( b6, fmt("%02X", b6:pack( true ):byte(1) ) )
print( b7, fmt("%02X", b7:pack( true ):byte(1) ) )
print( b8, fmt("%02X", b8:pack( true ):byte(1) ) )


