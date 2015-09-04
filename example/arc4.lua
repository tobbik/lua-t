#!out/bin/lua

t  = require't'
r  = t.Encode.Arc4('thekey')
r1 = t.Encode.Arc4('thekey')
ms = r:crypt("This is my Message to you")
mc = r1:crypt(ms)
print( ms, mc)
