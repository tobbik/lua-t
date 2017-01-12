T      = require't'
Pack   = T.Pack
Oht    = T.OrderedHashTable
fmt    = string.format
wrt    = io.write

b    = 'aBcDeüHiJkLmNoPö'
fmts = '>I3<i2bB>I5<I4h'
o    = Oht(
	  { threeUInt  = '>I3' }
	, { twoIntegs  = '<I2' }
	, { signedByte = 'b'   }
	, { unsignByte = 'B'   }
	, { fiveSInt   = '>I5' }
	, { fourSInt   = '<I4' }
	, { signShort  = 'h'   }
)

s    = Pack( fmts )
p1   = Pack(
	  { threeUInt  = '>I3' }
	, { twoIntegs  = '<I2' }
	, { signedByte = 'b'   }
	, { unsignByte = 'B'   }
	, { fiveSInt   = '>I5' }
	, { fourSInt   = '<I4' }
	, { signShort  = 'h'   }
)
p2   = Pack(
	  { threeUInt  = s[1] } -- reuse packers from sequence
	, { twoIntegs  = s[2] }
	, { signedByte = s[3] }
	, { unsignByte = s[4] }
	, { fiveSInt   = s[5] }
	, { fourSInt   = s[6] }
	, { signShort  = s[7] }
)
p3   = Pack(
	  { threeUInt  = p1[1] } -- reuse packers from another packer
	, { twoIntegs  = p1[2] }
	, { signedByte = p1[3] }
	, { unsignByte = p1[4] }
	, { fiveSInt   = p1[5] }
	, { fourSInt   = p1[6] }
	, { signShort  = p1[7] }
)
p4   = Pack( o )            -- can use OrderedHashTable instance

assert( #s == #p1, "Packer Sequence and Packer Struct shall have the same length" )
print( #s , s )
print( #p1, p1 )
print( #p2, p2 )
print( #p3, p3 )
print( #p4, p4 )

for k,v in pairs( Pack.getReference( p1 ) ) do print( k, v ) end
for k,v in pairs( Pack.getReference( p2 ) ) do print( k, v ) end
for k,v in pairs( Pack.getReference( p3 ) ) do print( k, v ) end
for k,v in pairs( Pack.getReference( p4 ) ) do print( k, v ) end

print( '\nindex\t    \tPack.Field Definition \t\t\tValue read from index' )
for i=1,#s do
	wrt( i .. '\t      \t'  .. tostring(s[i])        .. '\t' .. s[i]( b )        .. '\n' )
	wrt( i .. '\t      \t'  .. tostring(p1[i])       .. '\t' .. p1[i]( b )       .. '\n' )
	wrt( i .. '\t      \t'  .. tostring(p2[i])       .. '\t' .. p2[i]( b )       .. '\n' )
	wrt( i .. '\t      \t'  .. tostring(p3[i])       .. '\t' .. p3[i]( b )       .. '\n' )
	wrt( i .. '\t      \t'  .. tostring(p4[i])       .. '\t' .. p4[i]( b )       .. '\n' )
end

