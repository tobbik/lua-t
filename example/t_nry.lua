#!../out/bin/lua
Numarray = require( "t" ).Numarray
fmt      = string.format

m      = Numarray( 6 )
m[ 1 ] = 1
m[ 2 ] = 2
m[ 3 ] = 3
m[ 4 ] = 4
m[ 5 ] = 5
m[ 6 ] = 6
n      = Numarray( 1, 2, 3, 4, 5, 6 )

for k,v in pairs( m ) do
	print( k, v )
end


