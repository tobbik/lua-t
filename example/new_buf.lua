#!../out/bin/lua
xt = require("xt")

b = xt.Buffer('Alles wird irgendwann wieder gut!')
print("\t\t\tBUFFER ACCESS");
print( b:toHex() )
print( b:readInt( 4,4,'l' ) )
print( b:readInt( 4,4,'b' ) )

print( b:readInt( 9,6,'l' ) )
print( b:readInt( 9,6,'b' ) )


b = xt.Buffer(20)
print( b:toHex() )

b:writeInt(0x0000656772692064, 12,6,'l')
b:writeInt(0x0000642069726765,  3,6, 'b')
print( b:toHex() )
