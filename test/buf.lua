#!../out/bin/lua
local xt = require("xt")
exbs=function(e)
	local m = getmetatable(e)
	print(e, m)
	for k,v in pairs(m) do
		print(k,v)
	end
	for k,v in pairs(m.__index) do
		print(k,v)
	end
end

ext_bs=function(e)
	print(e)
	local m = getmetatable(e).__index
	m.update = function(x)
		local v=x:read()
		x:write(v+4)
	end
end

n=15
b=xt.Buffer(n)
for i=0,n do
	b:write8(i,i)
end
print("\t\t\tBUFFER ACCESS");
print( b:toHex() )
b:writeBits(16,8,195)
print( b:toHex() )

print("\t\t\tBITFIELD  ACCESS");
bs={}
bs[1]=b:BitField(6,7)
ext_bs(bs[1])
print(bs[1]:read())
bs[1]:write(1)
print(bs[1]:read())
print( b:toHex() )

print("\t\t\tBYTEFIELD  ACCESS");
bs[2]=b:ByteField(4,5)
print(bs[2]:read())
bs[2]:write(0x99ABCDEF)
print(bs[2]:read())
print( b:toHex() )

print("\t\t\tSTRINGFIELD ACCESS");
bs[3]=b:StringField(6,9)
print(bs[3]:read())
bs[3]:write("FOOBAR")
print(bs[3]:read())
print( b:toHex() )
exbs(bs[3])

b1=xt.Buffer("ABCDEFGEH")
exbs(b1)
print( b1:toHex() )
print( b1:getString() )
b1:writeString("ZYX")
print( b1:getString() )
b1:writeString("1234",4)
print( b1:getString() )
