---
-- \file      test/t_pck_seq.lua
-- \brief     Test for t.Pack Combinators (Array,Sequene,Struct)
--            make sure that returned size and offsets are correct
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h




local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local Buffer  = require( "t.Buffer" )
local        unpack,        char,        rep,        format,      tinsert  =
      string.unpack, string.char, string.rep, string.format, table.insert 

local NB      = Pack.charbits

local tests = {
	
	-- Test cases
	test_EqualSizedByteSequence = function( self )
		Test.Case.describe( "Sequence of equal sized byte Packer" )
		local p = Pack("<i5>I5i5<I5i5>I5i5<I5")
		T.assert( 8 == #p, "Packer length was %d, expected %d", #p, 8 )
		for i=1,#p do
			local byOff, biOff = (i-1)*5, (i-1)*5*NB --expected accumulated byte/bit offset
			local sby,sbi =  Pack.getSize( p[i] )
			local oby,obi =  Pack.getOffset( p[i] )
			T.assert( sby == 5,     "Int Packer Byte size was %d; expected: %d", sby, 5 )
			T.assert( sbi == 5*NB,  "Int Packer Bits size was %d; expected: %d", sbi, 5*NB )
			T.assert( oby == byOff, "Int Packer Byte offset was %d; expected: %d", oby, byOff )
			T.assert( obi == biOff, "Int Packer Bits offset was %d; expected: %d", obi, biOff )
		end
	end,

	test_MultiByteSequence = function( self )
		Test.Case.describe( "Sequence of differently sized byte Packer" )
		local p = Pack("bBhHi3I3iIi5I5i6I6i7I7i8I8")
		T.assert( 16 == #p, "Packer length was %d, expected %d", #p, 16 )
		for i=1,#p/2 do
			for x=0,1 do
				local byOff, biOff = ((i-1)*i)+(x*i), (((i-1)*i)+(x*i)) * NB
				local idx,siz = (i-1)*2 +x+1, i
				local sby,sbi = Pack.getSize(   p[idx] )
				local oby,obi = Pack.getOffset( p[idx] )
				T.assert( sby == siz,    "Int Packer Byte size was %d; expected: %d", sby, siz )
				T.assert( sbi == siz*NB, "Int Packer Bits size was %d; expected: %d", sbi, siz*NB )
				T.assert( oby == byOff,  "Int Packer Byte offset was %d; expected: %d", oby, byOff )
				T.assert( obi == biOff,  "Int Packer Bits offset was %d; expected: %d", obi, biOff )
			end
		end
	end,

	test_MultiByteArray = function( self )
		Test.Case.describe( "Array of byte Packer" )
		local n,s = 23, 3
		local p   = Pack( "<i"..s, n )
		T.assert( n == #p, "Array Packer length was %d, expected %d", #p, n )
		for i=1,#p do
			local byOff, biOff = (i-1)*3, ((i-1)*3) * NB
			local sby,sbi = Pack.getSize(   p[i] )
			local oby,obi = Pack.getOffset( p[i] )
			T.assert( sby == s,      "Int Packer Byte size was %d; expected: %d", sby, s )
			T.assert( sbi == s*NB,   "Int Packer Bits size was %d; expected: %d", sbi, s*NB )
			T.assert( oby == byOff,  "Int Packer Byte offset was %d; expected: %d", oby, byOff )
			T.assert( obi == biOff,  "Int Packer Bits offset was %d; expected: %d", obi, biOff )
		end
	end,

	test_MultiByteStruct = function( self )
		Test.Case.describe( "Struct of byte Packer" )
		local t = { }
		for i=1,Pack.numsize do
			table.insert( t, { ['signed'..i]   = 'i'..i } )
			table.insert( t, { ['unsigned'..i] = 'I'..i } )
		end
		local p = Pack( table.unpack( t ) )
		T.assert( Pack.numsize*2 == #p, "Packer length was %d, expected %d", #p, Pack.numsize*2 )
		for n=1,#p do
			local sz      = math.ceil(n/2)   -- size in bytes
			local cBy     = sz*(sz-1) + ((n-1)%2)*sz  -- expected accumulated byte Offset
			local sby,sbi = Pack.getSize( p[n] )
			local oby,obi = Pack.getOffset( p[n] )
			T.assert( sby == sz,     "Int Packer Byte size was %d; expected: %d", sby, sz )
			T.assert( sbi == sz*NB,  "Int Packer Bits size was %d; expected: %d", sbi, sz*NB )
			T.assert( obi == cBy*NB, "Int Packer Bits offset was %d; expected: %d", obi, cBy*NB )
			T.assert( oby == cBy,    "Int Packer Byte offset was %d; expected: %d", oby, cBy )
		end
	end,

	test_EqualSizedBitSequence = function( self )
		Test.Case.describe( "Sequence of equal sized bit Packer" )
		local f = "r%dR%d"
		for n=1,63 do
			local p = Pack( rep( format(f,n,n), 20 ) )
			T.assert( 20*2 == #p, "Packer length was %d, expected %d", #p, 20*2 )
			for i=1,#p do
				local cBi     =  n*(i-1)  -- expected accumulated bitOffset
				local sby,sbi =  Pack.getSize( p[i] )
				local oby,obi =  Pack.getOffset( p[i] )
				T.assert( sbi == n,       "Int Packer Bits size was %d; expected: %d", sbi, n )
				T.assert( sby == n//NB,   "Int Packer Byte size was %d; expected: %d", sby, n//NB )
				T.assert( obi == cBi,     "Int Packer Bits offset was %d; expected: %d", obi, cBi )
				T.assert( oby == cBi//NB, "Int Packer Byte offset was %d; expected: %d", oby, cBi//NB )
				--print( n, sby, sbi, oby, obi, cBi//NB, cBi, p[i] )
			end
		end
	end,

	test_MultiBitSequence = function( self )
		Test.Case.describe( "Sequence of differently sized bit Packer" )
		local s = 'r1'
		for n=2,62,2 do        s = s  .. format( "r%dR%d",n, n+1 )           end
		local p = Pack(s)
		T.assert( 63 == #p, "Packer length was %d, expected %d", #p, 63 )
		for n=1,#p do
			local cBi     =  (n*(n+1))/2 - n  -- expected accumulated bitOffset
			local sby,sbi =  Pack.getSize( p[n] )
			local oby,obi =  Pack.getOffset( p[n] )
			T.assert( sbi == n,       "Int Packer Bits size was %d; expected: %d", sbi, n )
			T.assert( sby == n//NB,   "Int Packer Byte size was %d; expected: %d", sby, n//NB )
			T.assert( obi == cBi,     "Int Packer Bits offset was %d; expected: %d", obi, cBi )
			T.assert( oby == cBi//NB, "Int Packer Byte offset was %d; expected: %d", oby, cBi//NB )
		end
	end,

	test_MultiBitArray = function( self )
		Test.Case.describe( "Array of bit Packer" )
		local n,s = 120, 27
		local p   = Pack( "<r"..s, n )
		T.assert( n == #p, "Array Packer length was %d, expected %d", #p, n )
		for i=1,#p do
			local byOff, biOff = ((i-1)*s)//NB, (i-1)*s
			local sby,sbi = Pack.getSize(   p[i] )
			local oby,obi = Pack.getOffset( p[i] )
			T.assert( sby == s//NB,  "Int Packer Byte size was %d; expected: %d", sby, s//NB )
			T.assert( sbi == s,      "Int Packer Bits size was %d; expected: %d", sbi, s )
			T.assert( oby == byOff,  "Int Packer Byte offset was %d; expected: %d", oby, byOff )
			T.assert( obi == biOff,  "Int Packer Bits offset was %d; expected: %d", obi, biOff )
		end
	end,

	test_MultiBitStruct = function( self )
		Test.Case.describe( "Struct of bit Packer" )
		local t = {}
		for i=1,31 do table.insert( t, { ['foo'..i] = 'r'..i } ) end
		local p = Pack( table.unpack( t ) )
		T.assert( 31 == #p, "Packer length was %d, expected %d", #p, 31 )
		for n=1,#p do
			local cBi    =  (n*(n+1))/2 - n  -- expected accumulated bitOffset
			local sby,sbi =  Pack.getSize( p[n] )
			local oby,obi =  Pack.getOffset( p[n] )
			T.assert( sbi == n,       "Int Packer Bits size was %d; expected: %d", sbi, n )
			T.assert( sby == n//NB,   "Int Packer Byte size was %d; expected: %d", sby, n//NB )
			T.assert( obi == cBi,     "Int Packer Bits offset was %d; expected: %d", obi, cBi )
			T.assert( oby == cBi//NB, "Int Packer Byte offset was %d; expected: %d", oby, cBi//NB )
		end
	end,
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )

