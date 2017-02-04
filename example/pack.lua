T    = require't'
sfmt = string.format

--          >I3       <I2   b   B    <I5           <I4          h
--          aBc       De      ü      HiJkL         mNoP         ö
expect = { 6373987, 25924, -61, 188, 311004130124, 1349471853, -18749 }


--       aBc       De      ü      HiJkL            mNoP         ö
p   = { 6373987, 25924, -61, 188, 311004130124,    1349471853, -18749 }
fmt = '>I3<i2bB>I5<I4h'
s   = string.pack( fmt, table.unpack( p ) ) --use Lua 5.3 built in to pack
print( T.Buffer(s):toHex() )

sq = T.Pack( fmt )          -- autodetect this as multiple packers -> create sequence

st = T.Pack(                -- key/value pairs create Struct 
	{ threeUInt  = sq[1] },  -- reuse definitions from sq ..
	{ twoIntegs  = sq[2] },
	{ signedByte = sq[3] },
	{ unsignByte = sq[4] },
	{ fiveSInt   = sq[5] },
	{ fourSInt   = sq[6] },
	{ signShort  = sq[7] }
)

--print( s, string.unpack( fmt, s ) )
print( s, table.unpack( sq(s) ) )

lt = table.pack( string.unpack( fmt, s ) )

print()
-- the result of pair(st) is using the internal ordering mechanism!
i = 1
for k,v in pairs( st ) do
	print( sfmt("%-10s   %-37s   %14d   %14d", k, v, v(s), lt[i] ) )
	i = i+1
end

--print()
-- the result of st(s) is a table -> iteration not ordered!
-- for k,v in pairs( st(s) ) do print( k,v ) end

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


