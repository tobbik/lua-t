T   = require( 't' )
Oht = require( "t.OrderedHashTable" )

o   = Oht(
	  { one   = 'first   position' }
	, { two   = 'second  position' }
	, { three = 'third   position' }
	, { four  = 'fourth  position' }
)
print( "Length after running constructor", #o )

-- insertin values adds them in order of insertion
o['five']  = 'fifth   position'
o['six']   = 'sixth   position'
o['seven'] = 'seventh position'
print( "Length after adding more elements", #o )

for k,v in pairs( o[ T.proxyTableIndex ] ) do print( k, v ) end
print(o)

for i,v,k in ipairs( o ) do
	print( "IPAIRS:", i, v, k, o[i], o[k] )
end

for k,v,i in pairs( o ) do
	print( "PAIRS: ", k, v, i, o[k], o[i] )
end

print( "Index of 'five' key:", Oht.index( o, 'five' ) )

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

print( "CONCAT OHT:", Oht.concat( o, '_|||||_' ) )
print( "CONCAT VALUES:", table.concat( Oht.values( o ), '_|||||_' ) )
print( "CONCAT KEYS:", table.concat( Oht.keys( o ), '_|||||_' ) )

for k,v in pairs( Oht.table( o ) ) do print( k, v ) end


