T      = require't'
Pack   = T.Pack
Buffer = T.Buffer
utl    = T.require('t_pck_utl')

--     0     1     2     3     4     5     6     7
--  0000  0001  0010  0011  0100  0101  0110  0111
--     8     9     A     B     C     D     E     F
--  1000  1001  1010  1011  1100  1101  1110  1111

-- idx:    1      2    3   4   5   6   7   8   9  10               111213141516 17
-- dec:   -7     85   -4  -3  -2  -1   0   1   2   3            -3692 t f 0 1 0 -1
-- bin: 1001 1010101 100 101 110 111 000 001 010 011 111000110010100  1 0 0 1 0 1
-- bin: 10011010 10110010  11101110   00001010   01111100 01100101 00100101
b  = Buffer( string.char( 0x9A, 0xB2, 0xEE, 0x0A, 0x7C, 0x65, 0x25 ) )

expect = { -7, 85, -4, -3, -2, -1, 0, 1, 2, 3, -3692, true, false, 0, 1, 0, -1 }
s  = Pack( 'r4R7r3r3r3r3r3r3r3r3r15vvR1R1r1r1' )

print( b:toHex(), '', '', #b, b:read() )
print( b:toBin( ) )

print( "Sequence s:", #s, s )
print( "-------------------------------------------------------------")
utl.get(s,b)

b1     = Buffer( #b )             -- create empty buffer of #b length
print( b1:toHex(), '', '', #b1 )  -- expecting all zeros
utl.set(s,b1,expect)
print( b1:toHex(), '', '', #b1, b1:read() )  -- expecting same as buffer b

assert( b1 == b, "The buffer shall be identical" )
