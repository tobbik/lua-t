T      = require't'
Pack   = require( "t.Pack" )
Test   = require( "t.Test" )
Buffer = require( "t.Buffer" )
Oht    = require( "t.OrderedHashTable" )
equals = require( "t.core" ).equals
prxIdx = require( "t.core" ).proxyTableIndex
utl    = T.require('t_pck_utl')

--     0     1     2     3     4     5     6     7
--  0000  0001  0010  0011  0100  0101  0110  0111
--     8     9     A     B     C     D     E     F
--  1000  1001  1010  1011  1100  1101  1110  1111

-- idx:    1      2    3   4   5   6   7   8   9  10               111213141516 17
-- dec:   -7     85   -4  -3  -2  -1   0   1   2   3            -3692 t f 0 1 0 -1
-- bin: 1001 1010101 100 101 110 111 000 001 010 011 111000110010100  1 0 0 1 0 1
-- bin: 10011010 10110010  11101110   00001010   01111100 01100101 00100101
--
val = Oht(
	  { threeUInt  = 6373987      }
	, { twoIntegs  = 25924        }
	, { twoBytes   = Oht(
		  { signedByte = -61          }
		, { unsignByte = 188          }
		) }
	, { bits       = { -7, 85
	                 , {-4, -3, -2, -1, 0, 1, 2, 3}
	                 , -3692, true, false, 0, 1, 0, -1
	                 }
	  }
	, { fiveSInt   = 311004130124 }
	, { fourSInt   = 1349471853   }
	, { signShort  = -18749       }
)
v = {
	  threeUInt  = 6373987
	, twoIntegs  = 25924
	, twoBytes   = {
		  signedByte = -61
		, unsignByte = 188
		}
	, bits       = { -7, 85
	                 , {-4, -3, -2, -1, 0, 1, 2, 3}
	                 , -3692, true, false, 0, 1, 0, -1
	                 }
	, fiveSInt   = 311004130124
	, fourSInt   = 1349471853
	, signShort  = -18749
}

p   = Pack(
	  { threeUInt  = '>I3' }
	, { twoIntegs  = '<i2' }
	, { twoBytes      = Pack(
		  { signedByte = 'b' }
		, { unsignByte = 'B' }
	) }
	, { bits       = Pack( 'r4','R7',Pack( 'r3', 8 ),'r15','v','v','R1','R1','r1','r1' ) }
	, { fiveSInt   = '>I5' }
	, { fourSInt   = '<I4' }
	, { signShort  = 'h' }
)

b   = Buffer( 'aBcDeü' .. string.char( 0x9A, 0xB2, 0xEE, 0x0A, 0x7C, 0x65, 0x25 ) .. 'HiJkLmNoPö' )
utl.get(p,b)

b1     = Buffer( #b )             -- create empty buffer of #b length
print( b1:toHex(), '', '', #b1 )  -- expecting all zeros
utl.set(p,b1,v)
print( b1:toHex(), '', '', #b1, b1:read() )  -- expecting same as buffer b

x=p(b1)
assert( equals( x, val[ prxIdx ] ), "The input and output shall be identical" )
