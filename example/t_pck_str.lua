T      = require't'
Pack   = T.Pack
Buffer = T.Buffer
Oht    = T.OrderedHashTable
rep    = string.rep
utl    = T.require('t_pck_utl')

--          >I3       <i2   b   B    >I5           <I4          h
--          aBc       De      ü      HiJkL         mNoP         ö
expect = Oht(
	  { threeUInt  = 6373987      }
	, { twoIntegs  = 25924        }
	, { twoBytes   = Oht(
		  { signedByte = -61          }
		, { unsignByte = 188          }
		) }
	, { fiveSInt   = 311004130124 }
	, { fourSInt   = 1349471853   }
	, { signShort  = -18749       }
)

b      = Buffer( 'aBcDeüHiJkLmNoPö' )
fmts   = '>I3<i2bB>I5<I4h'
print( b:toHex(), '', '', #b, b:read() )
print( string.unpack( fmts, b:read() ) )

s = Pack( fmts )
p = Pack(
	  { threeUInt  = s[1] }
	, { twoIntegs  = s[2] }
	, { twoBytes      = Pack(
		  { signedByte = s[3] }
		, { unsignByte = s[4] }
	) }
	, { fiveSInt   = s[5] }
	, { fourSInt   = s[6] }
	, { signShort  = s[7] }
)

print( "Sequence:", #s , s )
print( "Struct:  ", #p , p )

print( "Sequence Iterator" );
utl.get( p, b )

b1     = Buffer( #b )
print( b1:toHex(), '', '', #b1 )
utl.set( p, b1, expect )
print( b1:toHex(), '', '', #b1, b1:read() )  -- expecting same as buffer b

assert( b1 == b, "The buffer shall be identical" )

