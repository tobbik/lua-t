Set = require( 't' ).Set

function p( s ) return #s, table.unpack( Set.toTable( s ) ) end

a = Set( {'one','two','three','four','eight'} ) -- create set from table

b = Set( a )  -- clone existing set and alter it
b['four']  = nil     -- delete elements
b['eight'] = nil
b['five']  = true    -- adding elements
b['six']   = true
b['seven'] = true

print( 'Set A:', p(a) )
print( 'Set B:', p(b) )

if     a.two     then print( 'Set a contains "two"' ) end
if     a.six     then print( 'Set a contains "six"' ) end
if not a.six then print( 'Set a does not contain "six"' ) end
print( 'Size of Set a: ' .. #a .. '\nSize of Set B: ' .. #b )

print( 'Union:', p( a|b ) )

print( 'Inters:', p( a&b ) )

print( 'ComA-B:', p( a-b ) )
print( 'ComB-A:', p( b-a ) )

print( 'SymDif:', p( a~b ) )


for k,v in pairs( a ) do print( k, v ) end

x = a|b
y = a|b

print( x,y )
print( p(x) )
print( p(y) )

if x==y then print( 'The sets are equal.' ) end
x.one = nil
print( p(x) )
print( p(y) )
if x~=y then print( 'The sets are not equal.' ) end
if x<=y then print( 'x is a subset of y.' ) end
x.ten = true
print( p(x) )
print( p(y) )
if x~=y then print( 'The sets are not equal.' ) end

if not (a%b) then print( 'The sets A and B are not disjoint.' ) end
a.one   = nil
a.two   = nil
a.three = nil
if a%b  then print( 'The sets A and B are disjoint.' ) end

