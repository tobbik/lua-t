Oht = require( 't' ).OrderedHashTable


o   = Oht( )

-- insertion order is preserved inside the Hash table
o['one']   = 'a'
o['two']   = 'b'
o['three'] = 'c'
o['four']  = 'd'
o['five']  = 'e'
o['six']   = 'f'
o['seven'] = 'g'
print( o.one, o.two, o.three, o.four, o.five, o.six, o.seven )
print( o[1],  o[2],  o[3],    o[4],   o[5],   o[6],  o[7] )
print( #o )

for i,v,k in ipairs( o ) do
	print( "IPAIRS:", i, v, k )
end

for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end

print( "Index of 'five' key:", Oht.getIndex( o, 'five' ) )

o[2] = nil;
print( "Length after deleting o[2]:", #o )
print( o.one, o.two, o.three, o.four, o.five, o.six, o.seven )
print( o[1],  o[2],  o[3],    o[4],   o[5],   o[6],  o[7] )
for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end

o.eight = 'h'
print( #o )
o.four  = nil;
print( #o )
for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end

Oht.insert( o, 3, 'eleven', 'l' );

print( "After Insert:", #o )
for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end


