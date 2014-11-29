#!../out/bin/lua
t=require't'

-- examples for using the Packer constructor
pc= t.Pack( '<I6' )
sq= t.Pack( 'i2', 'i2', 'i1', 'i1' ) -- sequence of 4 packers, access by sq[3]
sq= t.Pack( 'i2i2i1i1' )             -- compact way of definition
ar= t.Pack( '>i2', 14 )
st= t.Pack(
	{a='B'},         -- unsigned Byte, named a at pos 1
	{b='>i2'},       -- little endian, signed integer of 2 bytes, named b at pos 2
	{c='c24'},       -- raw string of 24 characters, named c at pos3
	{d=sq},          -- sequence type named d at pos 4
	{e=sq[2]}        -- type of sq[2](i2) named e at pos 5
)

