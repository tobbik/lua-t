write,fmt = io.write, string.format
If = require"t".Net.Interface

ifs = If.list( )

for ifName,ifValue in pairs( ifs ) do
	print( ifName, ifValue )
	for n,v in pairs( ifValue ) do
		write( fmt(  '\t%s:\t%s\n', n, v ) )
	end
end
