---
-- \file    test/t_pck_bytes.lua
-- \brief   Test for t.Pack (binary packing/unpacking)
--          Packing and unpacking of various types of byte based values.
local T       = require( "t" )
local Test    = require( "t.Test" )
local Buffer  = require( "t.Buffer" )
local Pack    = require( "t.Pack" )
local pAssert = T.require'assertHelper'.Packer
local        unpack,        char,      random,      tpack,      tremove  =
      string.unpack, string.char, math.random, table.pack, table.remove


local tests = {
	-- Test cases
	test_SingleByte = function( self )
		Test.Case.describe( "Single Byte Integer Packer" )
		local buf    = Buffer( 1 )
		local pi, pu = Pack('b'), Pack('B')
		local max    = 0xFF
		-- testing every single permutation
		for v=0,max do
			local i = (v>max//2 and v-max-1 or v )  --expected signed integer
			local b = char( v )
			T.assert( pu(b) == v, "Should be: %d; was %d", v, pu(b) )
			T.assert( pi(b) == i, "Should be: %d; was %d", i, pi(b) )
			-- write integer value into buffer
			pu( buf, v )
			T.assert( buf:read() == b, "Should be: `%s`; was `%s`", b, buf:read() )
			pi( buf, i )
			T.assert( buf:read() == b, "Should be: `%s`; was `%s`", b, buf:read() )
			--print(r, pu(b), v, (i>max//2 and v-max-1 or i ) )
		end
	end,

	test_ShortInteger = function( self )
		Test.Case.describe( "Short (2 Byte) Integer Packer" )
		local buf                = Buffer( 2 )
		local puB, piB, puL, piL = Pack('>H'),Pack('>h'),Pack('<H'),Pack('<h')
		local mx, max            = 0xFF,0xFFFF
		-- testing every single permutation
		for n=0,mx do
			for m=0,mx do
				local v  = n*(mx+1) + m
				local i  = (v>max//2 and v-max-1 or v )  --expected signed integer
				local bB = char( n ) .. char( m )
				local bL = char( m ) .. char( n )
				T.assert( puB(bB)         == v, "Should be: %d; was %d", v, puB(bB) )
				T.assert( piB(bB)         == i, "Should be: %d; was %d", i, piB(bB) )
				T.assert( puL(bL)         == v, "Should be: %d; was %d", v, puL(bL) )
				T.assert( piL(bL)         == i, "Should be: %d; was %d", i, piL(bL) )
				T.assert( unpack('>H',bB) == v, "Should be: %d; was %d", v, unpack('>H',bB) )
				T.assert( unpack('>h',bB) == i, "Should be: %d; was %d", i, unpack('>h',bB) )
				T.assert( unpack('<H',bL) == v, "Should be: %d; was %d", v, unpack('<H',bL) )
				T.assert( unpack('<h',bL) == i, "Should be: %d; was %d", i, unpack('<h',bL) )
				-- write integer value into buffer
				puB( buf, v )
				T.assert( buf:read() == bB, "Should be: `%s`; was `%s`", bB, buf:read() )
				piB( buf, i )
				T.assert( buf:read() == bB, "Should be: `%s`; was `%s`", bB, buf:read() )
				puL( buf, v )
				T.assert( buf:read() == bL, "Should be: `%s`; was `%s`", bL, buf:read() )
				piL( buf, i )
				T.assert( buf:read() == bL, "Should be: `%s`; was `%s`", bL, buf:read() )
			end
		end
	end,

	test_SequenceOfMixedBytes = function( self )
		-- b   B   <I5          n                    >h     <f              T
		-- ä       BcDeF        gHiJkLmN              ö     PqRs            TÜvWxYz
		-- -61 132 439250862946 2.1271305064836e+223 -15466 1040748380160.0 6519339205175067508
		Test.Case.describe( "Sequence of bytes (integer and float)" )
		local s = 'ÄbCdEfGhIjKlMnÖpQrStüVwXyZ'
		local f = 'bB<I5n>h<fT'
		local p = Pack( f )
		local x = tpack( unpack( f, s ) )  -- get Lua internal way
		tremove( x, #x )                   -- remove byte position after read
		local r = p( s )
		T.assert( #r == #x, "Expected %d values; got %d", #x, #r )
		for i=1,#r do
			T.assert( r[i] == x[i], "Expected %s values; got %s", x[i], r[i] )
		end
	end,
	
	test_Float = function( self )
		Test.Case.describe( "Float Packers 'f', 'd', 'n'" )
		local sfL, sfB = "hGfE"    , "EfGh"
		local sdL, sdB = "HgFeDcBa", "aBcDeFgH"

		local pfB, pdB, pnB = Pack('>f'), Pack('>d'), Pack('>n')
		T.assert( pfB(sfB) == unpack('>f',sfB), "Expected %f, got %f", unpack('>f',sfB), pfB(sfB) )
		T.assert( pdB(sdB) == unpack('>d',sdB), "Expected %f, got %f", unpack('>d',sdB), pdB(sdB) )
		T.assert( pnB(sdB) == unpack('>n',sdB), "Expected %f, got %f", unpack('>n',sdB), pnB(sdB) )

		local pfL, pdL, pnL = Pack('<f'), Pack('<d'), Pack('<n')
		T.assert( pfL(sfL) == unpack('<f',sfL), "Expected %f, got %f", unpack('<f',sfL), pfB(sfL) )
		T.assert( pdL(sdL) == unpack('<d',sdL), "Expected %f, got %f", unpack('<d',sdL), pdB(sdL) )
		T.assert( pnL(sdL) == unpack('<n',sdL), "Expected %f, got %f", unpack('<n',sdL), pnB(sdL) )

		-- explictely comparing Endianess
		T.assert( pfL(sfL) == pfB(sfB), "Expected %f, got %f", pfB(sfB), pfB(sfL) )
		T.assert( pdL(sdL) == pdB(sdB), "Expected %f, got %f", pdB(sdB), pdB(sdL) )
		T.assert( pnL(sdL) == pnB(sdB), "Expected %f, got %f", pnB(sdB), pnB(sdL) )
	end,

	test_SizedRawByteSequences = function( self )
		Test.Case.describe( "Sized raw byte sequence Packers(strings) 'c1..n'" )
		local n = 5000
		local b = Buffer(n)
		for i=1,n do b:write( char( random(0,255) ), i ) end
		local s = b:read()
		for i = 1,n do
			local p = Pack( 'c'..i )
			T.assert( p(s) == s:sub(1,i), "Expected `%s`; got `%s`", s:sub(1,i), p(s) )
		end
	end,

}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )

