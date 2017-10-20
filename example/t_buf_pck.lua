#!../out/bin/lua
t      = require("t")
Buffer = require( "t.Buffer" )
Pack   = require( "t.Pack" )
fmt    = string.format

s     = 'Alles wird irgendwann wieder gut!'
b     = Buffer(s)
rbyte = function( pfmt, pos )
	local lb = Pack( pfmt )( b:read(pos) )
	local ls = string.unpack( pfmt, s, pos)
	print( pfmt, pos, lb, string.format("%016X", lb), ls, string.format("%016X", ls)  )
end

print("\t\t\tBUFFER ACCESS");
print( "READING ACCESS by BYTES" )
print( b:toHex() )
rbyte( '>i4', 4)
rbyte( '<i4', 4)
rbyte( '>i6', 9)
rbyte( '<i6', 9)


print( "READING ACCESS by BITS" )
b = Buffer( string.char( 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ) )
print( b:toHex() )
print( b:toBin() )
-- pos 4, ofs 1, length 10
print( 4, 1, 10, Pack('R9')( b:read( 4 ) ) )          -- Pack.Bits(10,1))
print( 4, 3, 9,  Pack('R2R9R4')[2]( b:read( 4 ) ) )   -- Pack.Bits(9, 3))
print( 4, 8, 8,  Pack('R7R8r')[2]( b:read( 4 ) ) )    -- Pack.Bits(8, 6))


print( "READING ACCESS by BITS" )
l = Buffer( string.char( 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 ) )
print( l:toBin() )
io.write( 'bits')
p = Pack('B')
for k=1,8 do io.write( fmt("      %02X", p(l:read( k ) ) ) ) end
print()

-- fake offset by creating a packer with leading bitfield and then read the
-- second element
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', Pack('r'..k..'v')[2](l:read( i )) ) ) end
	io.write( '\n' )
end
print('--------------------------------------------------------------------')
-- create bitfield and only read the single bit in question
p = Pack( 'RRRRRRRR')
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( '       ' .. p[k]( l:read( i ) ) ) end
	io.write( '\n' )
end

print('--------------------------------------------------------------------')
p = Pack( 'r', 8 )
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', p[k]( l:read( i ) ) ) ) end
	io.write( '\n' )
end

print('--------------------------------------------------------------------')
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', Pack('R'..k)( l:read( i ) ) ) ) end
	io.write( '\n' )
end

print('--------------------------------------------------------------------')
-- for single boolean bits the following number is interpreted as offset
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', Pack('r'..k)( l:read( i ) ) ) ) end
	io.write( '\n' )
end

p = Pack('R3r5')[2]
b = Buffer(1)
p( b, -5 )

