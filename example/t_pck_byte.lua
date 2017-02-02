T      = require't'
Pack   = T.Pack
Buffer = T.Buffer
rep    = string.rep

--          >I3       <i2   b   B    >I5           <I4          h
--          aBc       De      ü      HiJkL         mNoP         ö
expect = { 6373987, 25924, -61, 188, 311004130124, 1349471853, -18749 }
b      = Buffer( 'aBcDeüHiJkLmNoPö' )
fmts   = '>I3<i2bB>I5<I4h'
print( b:toHex(), '', '', #b, b:read() )
print( string.unpack( fmts, b:read() ) )

s = Pack( fmts )

print( "Sequence:", #s , s )

print( "Sequence Iterator" );
for k,v in pairs( s ) do print( k, v,    v(b)   , expect[k] ) end
for i,v in pairs( s ) do print( i, s[i], s[i](b), expect[i] ) end

b1     = Buffer( #b )
print( b1:toHex(), '', '', #b1 )
for k,v in pairs( s ) do v(b1, expect[k]); print( k, v, v(b1) ) end
--for k,v in ipairs( expect ) do print( k, v, v(b) ) end
print( b1:toHex(), '', '', #b1, b1:read() )  -- expecting same as buffer b

assert( b1 == b, "The string shall be identical" ) -- confirm equality
