Time = require( 't.Time' )

tm = Time( 200 )
print( Time, "Members:" )
for k,v in pairs( Time ) do
	print( '', k, type( v ), v )
end

print( Time, "Meta-Members:" )
for k,v in pairs( getmetatable( Time ) ) do
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

