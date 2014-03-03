#!out/bin/lua

xt=require'xt'
r  = xt.Encode.Arc4('thekey')
r1 = xt.Encode.Arc4('thekey')
ms = r:crypt("This is my Message to you")
mc = r1:crypt(ms)
print(ms,mc)
