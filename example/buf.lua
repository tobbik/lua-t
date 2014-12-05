#!../out/bin/lua
t = require("t")
fmt= string.format

s = 'Alles wird irgendwann wieder gut!'
b = t.Buffer(s)
rbyte = function( fmt, pos )
	local lb = b:unpack( fmt, pos )
	local ls = string.unpack( fmt, s, pos)
	print( fmt, pos, lb, string.format("%016X", lb), ls, string.format("%016X", ls)  )
end

print("\t\t\tBUFFER ACCESS");
print( "READING ACCESS by BYTES" )
print( b:toHex() )
rbyte( '>i4', 4)
rbyte( '<i4', 4)
rbyte( '>i6', 9)
rbyte( '<i6', 9)

--[[
print( "WRITING ACCESS by BYTES" )
b = t.Buffer(20)
print( b:toHex() )
b:write(15, t.Pack.IntB4, 0x0000000033445566)
print( b:toHex() )
b:write(11, t.Pack.IntL3, 0x0000000000445566)
print( b:toHex() )
b:write(3, t.Pack.IntL6, 0x0000998877665544)
print( b:toHex() )


print( "WRITING ACCESS by BITS" )
b:write( 7, t.Pack.Bits(8,6), 128)
print( b:toHex() )
b:write( 9, t.Pack.Int1, 255)
print( b:toHex() )
b:write( 8, t.Pack.Bits(8,6), 128)
print( b:toHex() )
--]]


print( "READING ACCESS by BITS" )
b = t.Buffer( string.char( 0x00, 0x00,0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ) )
print( b:toHex() )
-- pos 4, ofs 1, length 10
print( 4, 1, 10, b:unpack( 'R10', 4) )  -- t.Pack.Bits(10,1))
print( 4, 3, 9,  b:unpack( t.Pack('R2R9R4')[2], 4 ) )  -- t.Pack.Bits(9, 3))
print( 4, 8, 8,  b:unpack( t.Pack('R7R8r')[2], 4 ) )   -- t.Pack.Bits(8, 6))


print( "READING ACCESS by BITS AGAIN" )
l = t.Buffer( string.char( 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 ) )
io.write( 'bits')
for k=1,8 do io.write( fmt("      %02X", l:unpack( 'B', k ) ) ) end
print()
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( '       ' ..  l:unpack( t.Pack('R1', 8)[k], i ) ) end
	io.write( '\n' )
end

print()
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', l:unpack( t.Pack('r', 8)[k], i ) ) ) end
	io.write( '\n' )
end

print()
-- for single boolean bits the following number is interpreted as offset
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', l:unpack( t.Pack('v'..k), i ) ) ) end
	io.write( '\n' )
end

print()
-- for single boolean bits the following number is interpreted as offset
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', l:unpack( t.Pack('R'..k), i ) ) ) end
	io.write( '\n' )
end

print()
-- for single boolean bits the following number is interpreted as offset
for i=1,8 do
	io.write( i..'   ' )
	for k=1,8 do io.write( fmt( '%8s', l:unpack( t.Pack('r'..k), i ) ) ) end
	io.write( '\n' )
end

p = t.Pack('R3r5')[2]
b = t.Buffer(1)
p(b,-5)

