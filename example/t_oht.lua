Oht = require( 't' ).OrderedHashTable

o   = Oht(
	  { one   = 'first   position' }
	, { two   = 'second  position' }
	, { three = 'third   position' }
	, { four  = 'fourth  position' }
)
print( "Length after running constructor", #o )

o['five']  = 'fifth   position'
o['six']   = 'sixth   position'
o['seven'] = 'seventh position'
print( "Length after adding more elements", #o )

for k,v in pairs( Oht.getReference( o ) ) do print( k, v ) end
print(o)

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

print( Oht.concat( o, '_|||||_' ) )
print( table.concat( Oht.getValues( o ), '_|||||_' ) )
print( table.concat( Oht.getKeys( o ), '_|||||_' ) )

for k,v in pairs( Oht.getTable( o ) ) do print( k, v ) end


