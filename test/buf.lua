#!../out/bin/lua
local xt = require("xt")

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
bs[4]=b:ByteField(2, #b-2)
bs[4]:write(0x4444)
print( b:readString() )
print( b:toHex() )
bs[4]:write( b:getCRC16(#b-2) )
print( b:toHex() )


local str = string.char(0).."ABCDEFGEHIJKL"
b1=xt.Buffer(#str, str)
print( b1:toHex() )
print( b1:readString() )
b1:writeString("ZYX")
print( b1:readString() )
b1:writeString("12345",5)
print( b1:readString() )
print( b1:readString(5) )
print( b1:readString(5,4) )
