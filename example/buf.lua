#!../out/bin/lua
xt = require("xt")
fmt= string.format

rbyte = function( pos,len,endian )
	local lb,ls = b:readInt( 4,4,'l'), s:undumpint( 5,4,'l')
	lb,ls = b:readInt( pos, len, endian), s:undumpint( pos+1, len, endian)
	print( lb, string.format("%016X", lb), ls, string.format("%016X", ls)  )
end

s = 'Alles wird irgendwann wieder gut!'
b = xt.Buffer(s)
print("\t\t\tBUFFER ACCESS");
print( "READING ACCESS by BYTES" )
print( b:toHex() )
rbyte( 4, 4, 'b' )
rbyte( 4, 4, 'l' )
rbyte( 9, 6, 'b' )
rbyte( 9, 6, 'l' )


print( "WRITING ACCESS by BYTES" )
b = xt.Buffer(20)
print( b:toHex() )
b:writeInt(0x0000112233445566, 12, 6, 'b')
print( b:toHex() )
b:writeInt(0x0000998877665544,  3, 6, 'l')
print( b:toHex() )


print( "READING ACCESS by BITS" )
b = xt.Buffer( string.char( 0x00, 0x00,0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ) )
print( b:toHex() )
-- pos 4, ofs 0, length 10
print( b:readBits( 3, 0, 10 ) )
print( b:readBits( 3, 3, 9 ) )
print( b:readBits( 3, 5, 8 ) )

print( "WRITING ACCESS by BITS" )
b:writeBits( 128, 7, 5, 8 )
print( b:toHex() )
b:writeInt( 255, 9, 1 )
print( b:toHex() )
b:writeBits( 128, 8, 5, 8 )
b:writeInt( 123, 1, 2 )
print( b:toHex() )

l = xt.Buffer( string.char( 0x01, 0x03,0x07, 0x0C, 0x10, 0x30, 0x70, 0xC0) )

