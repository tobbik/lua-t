Buffer = require( 't.Buffer' )

bf = Buffer( "This is a buffer" )
print( Buffer, "Members:" )
for k,v in pairs( Buffer ) do
	print( '', k, type( v ), v )
end

print( Buffer, "Meta-Members:" )
for k,v in pairs( getmetatable( Buffer ) ) do
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


