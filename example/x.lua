#!../out/bin/lua
Pack=require't.Pack'

-- S=Pack('Bb<i4c17')
-- s=Pack('Bb<i4c17', '<i4>i2c8H')
-- s1=Pack( S, s, '<i4>i2c8H' )
-- print( '###################################################################' )
-- s2= Pack( 'i2', 'i2', 'i1', 'i1' ) -- sequence of 5 packers, access by sq[3]
-- s3= Pack( 'i2', 'i2', 'i1', s1[1][3], 'i1' ) -- sequence of 5 packers, access by sq[3]
-- s4= Pack( 'h', 'l', 'i3', Pack('c12'), 'B' ) -- sequence of 5 packers, access by sq[3]
-- s= Pack( 'h', 'l', 'i3', 'c12', 'B' ) -- sequence of 5 packers, access by sq[3]
-- ts= Pack( '>i2<i2i1i1' )
-- tx= Pack( '>i2', '<i2', ts  ,'i1', 'i1' )
--print(S)
--print(s)
--print(s[1])
--print(s[2])
--print( Pack.size(S) )
--print( Pack.size(S) )
--print( Pack.size(s[1]) )
--print( Pack.size(s[2]) )
--print( Pack.size(s) )

--sq1= Pack( 'i2', 'i2', 'i1', s1[1][3], 'i1' ) -- sequence of 5 packers, access by sq[3]


-- examples for using the Packer constructor
-- pc = Pack( '<I6' )
-- sq1= Pack( 'i2i2i1<I6i1' )              -- compact way of definition
-- sq2= Pack( 'i2', 'i2', 'i1', pc, 'i1' ) -- sequence of 5 packers, access by sq[3]
-- sq3= Pack( 'i2i2', 'i1<I6i1' )          -- sequence of 2 sequences
-- ar = Pack( '>i2', 14 )
-- st = Pack(
-- 	{a='B'},         -- unsigned Byte, named a at pos 1
-- 	{b='>i2'},       -- little endian, signed integer of 2 bytes, named b at pos 2
-- 	{c='c24'},       -- raw string of 24 characters, named c at pos3
-- 	{d=sq3[2]},      -- sequence type named d at pos 4
-- 	{e=sq2[3]},      -- type of sq[2](i2) named e at pos 5
-- 	{f='Bb<i4c17'}   -- type of sequence named f at pos 6
-- )
p = Pack('<i2')
ss = Pack (
	p,
	p,
	'rrrrrrrr',
	Pack('i1'),
	Pack('i1')
)


s = Pack (
	{a='r'},
	{b='r'},
	{c='r'},
	{d='r'},
	{e='r'},
	{f='r'},
	{g='r'},
	{h='r'}
)
