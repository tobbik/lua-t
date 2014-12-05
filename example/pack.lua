T    = require't'
sfmt = string.format

p   = { 24898, 17507, 101, 70 }
fmt = '>i2<i2bb'
s   = string.pack( fmt, table.unpack( p ) )

sq = T.Pack( fmt )          -- autodetect this as multiple packers -> create sequence

st = T.Pack(                -- key/value pairs create Struct 
	{ length       = sq[1] },  -- reuse definitions from sq ..
	{ ['type']     = sq[2] },
	{ ['@status']  = sq[3] },
	{ Count        = sq[4] }
)

print( s, string.unpack( fmt, s ) )
print( s, table.unpack( sq(s) ) )

print()
-- the result of pair(st) is using the internal ordering mechanism!
for k,v in pairs( st ) do print( k, v, v(s) ) end

print()
-- the result of st(s) is a table -> iteration not ordered!
for k,v in pairs( st(s) ) do print( k,v ) end

--p=T.Pack(">i2")
--P=T.Pack(">I2")
--for i=0x00,0xFF do  for j=0x00,0xFF do
--	s=string.char(i,j)
--	print( string.unpack(">i2",s), string.unpack(">I2", s), p(s), P(s) )
--end end
--
p = T.Pack('R3r5')[2]
for c=0,31 do
	print( c, p(string.char(c)) )
end
--P= T.Pack('B')
--p= T.Pack('b')
b= T.Buffer(1)
b:toHex()
for c=-16,15 do
	p(b,c)
	print( c, sfmt("%02X", c), b:toHex() )
end

