xt = require"xt"
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
bs[1]=b:BitSegment(6,7)
ext_bs(bs[1])
print(bs[1]:read())
bs[1]:write(1)
print(bs[1]:read())
print( b:toHex() )

print("\t\t\tBYTEFIELD  ACCESS");
bs[2]=b:ByteSegment(4,5)
print(bs[2]:read())
bs[2]:write(0x99ABCDEF)
print(bs[2]:read())
print( b:toHex() )

print("\t\t\tSTRINGSEGMENT ACCESS");
bs[3]=b:StringSegment(6,9)
print(bs[3]:read())
bs[3]:write("FOOBAR")
print(bs[3]:read())
print( b:toHex() )
exbs(bs[3])
