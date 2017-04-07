#!out/bin/lua

B64  = require't.Encode.Base64'
t    = "This is my Message to you"
b    = B64.encode( t )
print( t, '\n', b, '\n', B64.decode( b ) )
