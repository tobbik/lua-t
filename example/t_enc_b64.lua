#!out/bin/lua

B64  = require't.Encode.Base64'
src  = "This is my Message to you"
enc  = B64.encode( src )
dec  = B64.decode( enc )
print( '\n', src, '\n', enc, '\n', dec )



