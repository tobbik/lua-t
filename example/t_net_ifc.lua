write,fmt = io.write, string.format
If = require"t".Net.Interface

ifNames = If.list( )
print( table.unpack(ifNames) );

for i, ifName in ipairs( ifNames ) do
	print( ifName )
	local ifc = If( ifName )
	print( ifc )
	for n,v in pairs( ifc ) do
		write( fmt(  '\t%s:\t%s\n', n, v ) )
	end
end

ifc = If( "default" )
print( ifc )
for n,v in pairs( ifc ) do
	write( fmt(  '\t%s:\t%s\n', n, v ) )
end
