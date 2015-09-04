t = require( 't' )

bf = t.Buffer( "This is a buffer" )
print( t.Buffer, "Members:" )
for k,v in pairs( t.Buffer ) do
	print( '', k, type( v ), v )
end

print( t.Buffer, "Meta-Members:" )
for k,v in pairs( getmetatable( t.Buffer ) ) do
	print( '', k, type( v ), v )
end

print( bf, "Members:" )
--for k,v in pairs( bf ) do
--	print( '', k, type( v ), v )
--end

print( bf, "Meta-Members:" )
for k,v in pairs( getmetatable( bf ) ) do
	print( '', k, type( v ), v )
end

print( bf, "MetaMeta-Members:" )
for k,v in pairs( getmetatable( bf ).__index ) do
	print( '', k, type( v ), v )
end

