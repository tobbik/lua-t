T      = require( "t" )
Pack   = require( "t.Pack" )
Buffer = require( "t.Buffer" )
utl    = T.require( "t_pck_utl" )

--          >I3       <i2   b   B    >I5           <I4          h
--          aBc       De      ü      HiJkL         mNoP         ö
expect = { 6373987, 25924, -61, 188, 311004130124, 1349471853, -18749 }
b      = Buffer( 'aBcDeüHiJkLmNoPö' )
fmts   = '>I3<i2bB>I5<I4h'
print( b:toHex(), '', '', #b, b:read() )
print( string.unpack( fmts, b:read() ) )

s = Pack( fmts )

print( "Sequence:", #s , s )
print( "-------------------------------------------------------------")
utl.get(s,b)

b1     = Buffer( #b )                        -- create empty buffer of #b length
print( b1:toHex(), '', '', #b1 )
utl.set(s,b1,expect)                         -- fill with except values
print( b1:toHex(), '', '', #b1, b1:read() )  -- expecting same as buffer b

assert( b1 == b, "The buffer shall be identical" )
