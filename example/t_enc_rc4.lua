#!out/bin/lua

RC4 = require't.Encode.Rc4'
r   = RC4('thekey')
r1  = RC4('thekey')
src = "This is my Message to you"
enc = r:crypt( src )
dec = r1:crypt( enc )
print( '\n', src, '\n', enc, '\n', dec )
