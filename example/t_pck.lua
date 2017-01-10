T      = require't'
Pack   = T.Pack
--Buffer = T.Buffer
fmt    = string.format
wrt    = io.write

names   = { 'threeUInt', 'twoIntegs', 'signedByte', 'unsignByte', 'fiveSInt', 'fourSInt', 'signShort' }
formats = { '>I3'      , '<i2'      , 'b'         , 'B'         , '>I5'     , '<I4'     , 'h'         }
fmts    = table.concat( formats, '' )
print( fmts )

b = 'aBcDeüHiJkLmNoPö'

s = Pack( fmts ) -- autodetect this as multiple packers -> create sequence
p = Pack(
	  { [ names[ 1 ] ] = formats[ 1 ] }
	, { [ names[ 2 ] ] = formats[ 2 ] }
	, { [ names[ 3 ] ] = formats[ 3 ] }
	, { [ names[ 4 ] ] = formats[ 4 ] }
	, { [ names[ 5 ] ] = formats[ 5 ] }
	, { [ names[ 6 ] ] = formats[ 6 ] }
	, { [ names[ 7 ] ] = formats[ 7 ] }
)

assert( #s == #p, "Packer Sequence and Packer Struct shall have the same length" )
print( #s, #p, s, p )

print( '\nindex\t    \tPack.Field Definition \t\t\tValue read from index' )
for i=1,#s do
	wrt( i .. '\t      \t'  .. tostring(s[i])        .. '\t' .. s[i]( b )        .. '\n' )
	wrt( i .. '\t      \t'  .. tostring(p[i])        .. '\t' .. p[i]( b )        .. '\n' )
	wrt( names[ i ] .. '\t' .. tostring(p[names[i]]) .. '\t' .. p[names[i]]( b ) .. '\n' )
end
print( '\nindex\t    \tPack.Field Definition \t\t\tPack.Field Definition\t\t\tValue read from index' )


-- iterator sequence
for i,v in pairs( s ) do
	print( i,'', v, s[i], v( b ) )
end

-- iterator sequence
for k,v in pairs( p ) do
	print( k, v, p[k], v( b ) )
end

for i,v in ipairs( s( b ) ) do print(i,v) end
for k,v in  pairs( p( b ) ) do print(k,v) end


-- work with arrays
ap = Pack( p, 10 )
as = Pack( s, 10 )
ab = string.rep( b, 10 )

print(ap, as)
print(as[6][3], as[6][3], ap[6].signedByte)
print(as[6][3](ab), as[6][3](ab), ap[6].signedByte(ab))



