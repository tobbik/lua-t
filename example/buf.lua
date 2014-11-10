#!../out/bin/lua
xt = require("xt")
fmt= string.format

rbyte = function( pos,len,endian )
	local lb,ls = b:readInt( pos, len, endian), s:undumpint( pos, len, endian)
	print( pos,len,endian,lb, string.format("%016X", lb), ls, string.format("%016X", ls)  )
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
b:writeInt(0x0000112233445566, 15, 4, 'b')
print( b:toHex() )
b:writeInt(0x0000112233445566, 11, 3, 'l')
print( b:toHex() )
b:writeInt(0x0000998877665544,  3, 6, 'l')
print( b:toHex() )


print( "READING ACCESS by BITS" )
b = xt.Buffer( string.char( 0x00, 0x00,0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ) )
print( b:toHex() )
-- pos 4, ofs 0, length 10
print( 4, 1, 10, b:readBits( 4, 1, 10 ) )
print( 4, 3, 9, b:readBits( 4, 3, 9 ) )
print( 4, 8, 8, b:readBits( 4, 6, 8 ) )

print( "WRITING ACCESS by BITS" )
b:writeBits( 128, 7, 6, 8 )
print( b:toHex() )
b:writeInt( 255, 9, 1 )
print( b:toHex() )
b:writeBits( 128, 8, 6, 8 )
print( b:toHex() )


print( "READING ACCESS by BITS AGAIN" )
l = xt.Buffer( string.char( 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01) )
io.write( 'bits   ')
for k=1,8 do io.write( fmt("%02X       ", l:readInt( k, 1 ) ) ) end
print()
for i=1,8 do
	io.write( i..'      ' )
	for k=1,8 do io.write( l:readBits( i, k, 1 ) .. '        ' ) end
	io.write( '\n' )
end

for i=1,8 do
	io.write( i )
	for k=1,8 do io.write( fmt( '%9s', l:readBit( i, k ) ) ) end
	io.write( '\n' )
end

