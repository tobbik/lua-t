write,fmt = io.write, string.format
Interface = require"t.Net.Interface"

ifs = Interface.list( )
for ifn, ifc in pairs( ifs ) do
	print( ifn )
	for n,t in pairs( ifc ) do
		write( fmt( '\t%s:\t%s\n', n, t ) )
		if 'table' == type( t ) then
			for k,v in pairs( t ) do
				write( fmt( '\t\t%s:\t%s\n', k, v ) )
			end
		end
	end
end



ifc = Interface( "default" )
print( ifc )
for n,t in pairs( ifc ) do
	write( fmt( '\t%s:\t%s\n', n, t ) )
	if 'table' == type( t ) then
		for k,v in pairs( t ) do
			write( fmt(  '\t\t%s:\t%s\n', k, v ) )
		end
	end
end

