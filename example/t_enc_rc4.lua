#!out/bin/lua

RC4  = require't.Encode.RC4'
r  = RC4('thekey')
r1 = RC4('thekey')
ms = r:crypt("This is my Message to you")
mc = r1:crypt(ms)
print( ms, mc)
