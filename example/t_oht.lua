Oht = require( 't' ).OrderedHashTable


o   = Oht( )

-- insertion order is preserved inside the Hash table
o['one']   = 'first   position'
o['two']   = 'second  position'
o['three'] = 'third   position'
o['four']  = 'fourth  position'
o['five']  = 'fifth   position'
o['six']   = 'sixth   position'
o['seven'] = 'seventh position'
print( "Length after adding elements", #o )

for i,v,k in ipairs( o ) do
	print( "IPAIRS:", i, v, k, o[i], o[k] )
end

for k,v,i in pairs( o ) do
	print( "PAIRS: ", k, v, i, o[k], o[i] )
end

print( "Index of 'five' key:", Oht.getIndex( o, 'five' ) )

o[2] = nil;
print( "Length after deleting index 2:", #o )
for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i, o[k], o[i] )
end

o.eight = 'eighth  position'
print( "Length after adding key 'eight':", #o )
o.four  = nil;
print( "Length after deleting key 'four':", #o )
for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end

Oht.insert( o, 3, 'new 3', 'new 3rd position' );
print( "Length after Insert:", #o )

for k,v,i in pairs( o ) do
	print( "PAIRS:", k, v, i )
end

print( Oht.concat( o, '_what_' ) )


