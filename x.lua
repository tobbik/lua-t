t=require('t')
fmt=string.format

b = 0xABCD
l = 0xCDAB
print( fmt('%016X    %016X',b,l) )

s = string.char( 0xC3, 0x34, 0x45, 0x5D, 0x67, 0x78, 0x89, 0x7E )
--s = string.char( 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF )

pf = t.Pack( 'f' )
tf = pf(s)
f  = string.unpack( 'f', s )

print( tf )
print( f )

pd = t.Pack( 'd' )
td = pd(s)
d  = string.unpack( 'd', s )

print( td )
print( d )

ds=string.pack( 'd', d )
bl=t.Buffer( ds )
print( bl:toHex() )

b=t.Buffer(8)
pd( b, d )

print( b:toHex() )

