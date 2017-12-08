---
-- \file      test/t_pck_mix.lua
-- \brief     Test for a combination of all (numeric) packer types in combintaion
--            This is testing the proper reading and writing to a complex
--            structure that mixes bit and byte based packers
--            It really is more of an integration test that probes the entire
--            t.Pack module.  If anything in here breaks the problem sits
--            probably very deep.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local Buffer  = require( "t.Buffer" )

local NB      = Pack.charbits
--     0     1     2     3     4     5     6     7
--  0000  0001  0010  0011  0100  0101  0110  0111
--     8     9     A     B     C     D     E     F
--  1000  1001  1010  1011  1100  1101  1110  1111

-- The bitfield 'bits' aka. self.p.bits
-- fmt:   r4       R7 r3[8]                                               r15  v  v R1 R1 r1 r1
-- idx:    1        2 3[1] 3[2] 3[3] 3[4] 3[5] 3[6] 3[7] 3[8]               4  5  6  7  8  9 10
-- dec:   -7       85  -4   -3   -2   -1    0    1    2    3            -3692  t  f  0  1  0 -1
-- bin: 1001  1010101 100  101  110  111  000  001  010  011  111000110010100  1  0  0  1  0  1
-- bin: 10011010 10110010  11101110   00001010   01111100 01100101 00100101
-- hex:       9A       B2        EE         0A         7C       65       25

local tests = {
	b = Buffer( 'aBcDeü' .. string.char( 0x9A, 0xB2, 0xEE, 0x0A, 0x7C, 0x65, 0x25 ) .. 'HiJkLmNoPö' ),
	v = {
		  int3ub = 6373987
		, int2sl = 25924
		, bytes  = { signed = -61 , unsigned = 188 }
		, bits   = { -7, 85 , {-4, -3, -2, -1, 0, 1, 2, 3} , -3692, true, false, 0, 1, 0, -1 }
		, int5sb = 311004130124
		, int4sl = 1349471853
		, int2s  = -18749
	},
	p = Pack(
		  { int3ub = '>I3' }                                                                 -- 3
		, { int2sl = '<i2' }                                                                 -- 2
		, { bytes  = Pack( { signed = 'b' }, { unsigned = 'B' }) }                           -- 2
		, { bits   = Pack( 'r4','R7',Pack( 'r3', 8 ),'r15','v','v','R1','R1','r1','r1' ) }   -- 7
		, { int5sb = '>I5' }                                                                 -- 5
		, { int4sl = '<I4' }                                                                 -- 4
		, { int2s  = 'h'   }                                                                 -- 2
	),

	-- Test cases

	test_ReadMixedNumericTypes = function( self )
		Test.Case.describe( "Read Sequence, struct, array of mixed numeric packers" )
		local s = self.b:read()
		--assert main struct
		for i,k in ipairs({ 'int3ub', 'int2sl', 'int5sb','int4sl', 'int2s' }) do
			T.assert( self.p[k]( s ) == self.v[k], 
			   "Expected %d; got%d", self.v[k], self.p[k]( s ) )
		end
		-- assert bytes struct
		for i,k in ipairs({ 'signed', 'unsigned' }) do
			T.assert( self.p.bytes[k]( s ) == self.v.bytes[k],
			   "Expected %d; got %d", self.v.bytes[k], self.p.bytes[k]( s ) )
		end
		-- assert bits sequence
		for i = 1,10 do
			if i~=3 then
				T.assert( self.p.bits[i]( s ) == self.v.bits[i],
				   "Expected %s; got %s", self.v.bits[i], self.p.bits[i]( s ) )
			end
		end
		-- assert bits sub array
		for i = 1,8 do
			T.assert( self.p.bits[3][i]( s ) == self.v.bits[3][i],
			   "Expected %d; got %d", self.v.bits[3][i], self.p.bits[3][i]( s ) )
		end
	end,

	test_WriteMixedNumericTypes = function( self )
		Test.Case.describe( "Write Sequence, struct, array of mixed numeric packer values to buffer" )
		local nb = Buffer( #self.b )
		for i,k in ipairs({ 'int3ub', 'int2sl', 'int5sb','int4sl', 'int2s' }) do
			self.p[k]( nb, self.v[k] )
		end
		for i,k in ipairs({ 'signed', 'unsigned' }) do
			self.p.bytes[k]( nb, self.v.bytes[k] )
		end
		for i = 1,10 do
			if i~=3 then
				self.p.bits[i]( nb, self.v.bits[i] )
			end
		end
		for i = 1,8 do
			self.p.bits[3][i]( nb, self.v.bits[3][i] )
		end
		T.assert( nb:read() == self.b:read(),
		   "Expected buffer not equal actual\n%s\n%s", nb:toHex(), self.b:toHex() )
	end,

	test_getSize = function( self )
		Test.Case.describe( "t.Pack.size() on various selected packers" )
		local exp,sByte,sBit = 25, Pack.size( self.p )
		T.assert( exp     == sByte, "Expected %d; got %d bytes of length", exp, sByte )
		T.assert( exp*NB  == sBit,  "Expected %d; got %d bits  of length", exp*NB, sBit )
		exp,sByte,sBit = 7, Pack.size( self.p.bits )
		T.assert( exp     == sByte, "Expected %d; got %d bytes of length", exp, sByte )
		T.assert( exp*NB  == sBit,  "Expected %d; got %d bits  of length", exp*NB, sBit )
		exp,sByte,sBit = 24, Pack.size( self.p.bits[3] ) -- this looks for the bits
		T.assert( sBit//8 == sByte, "Expected %d; got %d bytes of length", exp//8, sByte )
		T.assert( exp     == sBit,  "Expected %d; got %d bits  of length", exp, sBit )
		exp,sByte,sBit = 1, Pack.size( self.p.bytes.unsigned )
		T.assert( exp     == sByte, "Expected %d; got %d bytes of length", exp, sByte )
		T.assert( exp*NB  == sBit,  "Expected %d; got %d bits  of length", exp*NB, sBit )
	end,

	test_getOffset = function( self )
		Test.Case.describe( "t.Pack.offset() on various selected packers" )
		local exp,sByte,sBit = 19, Pack.offset( self.p.int4sl )
		T.assert( exp     == sByte, "Expected %d; got %d bytes of offset", exp, sByte )
		T.assert( exp*NB  == sBit,  "Expected %d; got %d bits  of offset", exp*NB, sBit )
		-- this looks for bits (3+2+2)*charbits + 4 + 7 + 5*3
		exp,sByte,sBit = 7*NB +4+7+ 5*3, Pack.offset( self.p.bits[3][6] )
		T.assert( exp     == sBit,  "Expected %d; got %d bits  of offset", exp, sBit )
		T.assert( exp//NB == sByte, "Expected %d; got %d bytes of offset", exp//NB, sByte )
		local pb = Pack( self.p.bits[3] )
		exp,sByte,sBit = 4*3, Pack.offset( pb[5] )
		T.assert( exp     == sBit,  "Expected %d; got %d bits  of offset", exp, sBit )
		T.assert( exp//NB == sByte, "Expected %d; got %d bytes of offset", exp//NB, sByte )
	end,
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )
