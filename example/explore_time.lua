t = require( 't' )

tm = t.Time( 200 )
print( t.Time, "Members:" )
for k,v in pairs( t.Time ) do
	print( '', k, type( v ), v )
end

print( t.Time, "Meta-Members:" )
for k,v in pairs( getmetatable( t.Time ) ) do
	print( '', k, type( v ), v )
end

print( tm, "Members:" )
--for k,v in pairs( tm ) do
--	print( '', k, type( v ), v )
--end

print( tm, "Meta-Members:" )
for k,v in pairs( getmetatable( tm ) ) do
	print( '', k, type( v ), v )
end

print( tm, "MetaMeta-Members:" )
for k,v in pairs( getmetatable( tm ).__index ) do
	print( '', k, type( v ), v )
end

