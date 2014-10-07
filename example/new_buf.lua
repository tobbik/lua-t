#!../out/bin/lua
xt = require("xt")

b = xt.Buffer('Alles wird irgendwann wieder gut!')
print("\t\t\tBUFFER ACCESS");
print( "READING ACCESS by BYTES" )
print( b:toHex() )
print( b:readInt( 4,4,'l' ) )
print( b:readInt( 4,4,'b' ) )

print( b:readInt( 9,6,'l' ) )
print( b:readInt( 9,6,'b' ) )

print( "WRITING ACCESS by BYTES" )
b = xt.Buffer(20)
print( b:toHex() )


b:writeInt(0x0000656772692064, 12,6,'l')
b:writeInt(0x0000642069726765,  3,6, 'b')
print( b:toHex() )


print( "READING ACCESS by BITS" )
-- b = xt.Buffer( string.char( 0x00, 0x81,0x42, 0xC3, 0x24, 0xA5, 0x66, 0xE7, 0x18, 0x99 ) )
b = xt.Buffer( string.char( 0x00, 0x00,0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00 ) )
print( b:toHex() )
-- pos 4, ofs 0, length 10
print( b:readBit( 3, 0, 10 ) )
print( b:readBit( 3, 3, 9 ) )
print( b:readBit( 3, 4, 8 ) )
