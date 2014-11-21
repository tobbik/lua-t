#!../out/bin/lua
t = require("t")
fmt= string.format

b1 = t.Pack.Bit1
b2 = t.Pack.Bit2
b3 = t.Pack.Bit3
b4 = t.Pack.Bit4
b5 = t.Pack.Bit5
b6 = t.Pack.Bit6
b7 = t.Pack.Bit7
b8 = t.Pack.Bit8
p  = t.Pack.IntB2
bt = t.Pack.Byte
nl = t.Pack.NibbleL
nh = t.Pack.NibbleH

print( p, nl, nh )

print( p:pack( 17519 ), t.Buffer( p:pack( 17519 ) ):toHex() )

print( p:unpack( 'Do' ) )

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


