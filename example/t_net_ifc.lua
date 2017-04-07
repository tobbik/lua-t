write,fmt = io.write, string.format
Interface = require"t.Net.Interface"

ifNames = Interface.list( )
print( table.unpack(ifNames) );

for i, ifName in ipairs( ifNames ) do
	print( ifName )
	local ifc = Interface( ifName )
	print( ifc )
	for n,v in pairs( ifc ) do
		write( fmt(  '\t%s:\t%s\n', n, v ) )
	end
end

ifc = Interface( "default" )
print( ifc )
for n,v in pairs( ifc ) do
	write( fmt(  '\t%s:\t%s\n', n, v ) )
end
