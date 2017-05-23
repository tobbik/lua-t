---
-- \file      test/t_pck_bits.lua
-- \brief     Test for t.Pack (binary packing/unpacking)
--            Packing and unpacking of various types of bit based values.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h




local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local Buffer  = require( "t.Buffer" )
local        unpack,        char,        rep,        format  =
      string.unpack, string.char, string.rep, string.format

local NB      = Pack.charbits

local tests = {
	-- Test cases
	test_SingleBit = function( self )
		Test.Case.describe( "Single Bit Integer Packer" )
		local buf        = Buffer( char( 0x80 ) ) -- 1000 0000
		local pi, pu, pb = Pack('r'), Pack('R'), Pack('v')
		T.assert( pi(buf) == -1,   "Expected %d; was %d",   -1, pi(buf) )
		T.assert( pu(buf) ==  1,   "Expected %d; was %d",   -1, pu(buf) )
		T.assert( pb(buf) == true, "Expected %s; was %s", true, pb(buf) )
	end,

	test_MultiBitAllSet  = function( self )
		Test.Case.describe( "Multi bit Integer Packer with all bits as 111111 ..." )
		-- 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111
		-- 1             x             ^                                         n
		-- ----------------------------|..........................................
		-- rx                          |Rn-x
		local maxBit = math.tointeger( math.log( math.maxinteger, 2 ) ) + 1
		local buf    = Buffer( rep( char ( 0xFF ), maxBit//NB ) ) --1111 1111   1111 1111 ...
		-- testing every single permutation
		for n=1,maxBit-1 do
			local p = Pack( format( 'r%dR%d', n, maxBit-n ) )
			-- for any 2's complement a sequence of 111111... means -1
			T.assert( p[1](buf) == -1, "Expected -1; got %016X", p[1](buf) )
			-- math.maxinteger == 2^(maxBit-1)
			-- for any unsigned sequence of 111111... means (2^n)-1
			T.assert( p[2](buf) == math.maxinteger >> (n-1),
				"Expected %016X; got %016X", math.maxinteger >> (n-1), p[2](buf) )
		end
	end,

	test_MultiBitAlterSet  = function( self )
		Test.Case.describe( "Multi bit Integer Packer alternating bits 101010 ..." )
		-- 10101010 10101010 10101010 10101010 10101010 10101010 10101010 10101010
		-- 1             x             ^                                         n
		-- ----------------------------|..........................................
		-- rx                          |Rn-x
		-- p[1]                        |p[2]
		local maxBit = math.tointeger( math.log( math.maxinteger, 2 ) ) + 1
		local buf    = Buffer( rep( char ( 0xAA ), maxBit//NB ) ) -- 10101010 ...
		local x      = unpack('j', buf:read() )
		-- testing every single permutation
		for n=1,maxBit-1 do
			local p   = Pack( format( 'r%dR%d', n, maxBit-n ) )
			local exp = x>>(n+(n%2)) -- shift right every second run to skip the 0
			T.assert( p[2](buf) == exp, "Expected %016X; got %016X", exp, p[2](buf) )
			--print(n, n+(n%2), x>>2, exp, p[1](buf), p[2](buf))
		end
	end,

	test_MultiBitSliding  = function( self )
		Test.Case.describe( "8 bit Integer packer sliding over 11111111 00000000" )
		-- 000000000 11111111 00000000
		--           |      |
		--            |       |  ->
		local s,n = char( 0x00, 0xFF, 0x00 ), NB
		local x,r = math.tointeger(2^n)-1, 0
		for i = 0,n do
			local p = Pack( format( "r%dR%d", n+i, n ) )
			local a,b,r = p[1](s), p[2](s), math.tointeger(2^i)-1
			T.assert( a == r,   "Expected %d; got %d", r  , a )
			T.assert( b == x-r, "Expected %d; got %d", x-r, b )
			T.assert( b+a == x, "Expected %d; got %d", x, a+b )
			--print(p[1], p[2], a+b, x, a, r, b, x-r)
		end
	end,
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )

