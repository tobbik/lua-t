---
-- \file    test/t_pck_fmt.lua
-- \brief   Test for t.Pack (binary packing/unpacking)
--          Testing that the various formatting options are creating the
--          appropriate internal Packer types.  This is only testing atomic
--          packers.
local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local pAssert = T.require'assertHelper'.Packer


local tests = {
	byteSize = (math.tointeger( math.log( math.maxinteger, 2 ) ) + 1) // Pack.charsize,

	-- Test cases
	test_SizedIntegerPacker = function( self )
		Test.Case.describe( "Test Sized Integer Packers   'i1..n', 'I1..n'" )
		for i=1,self.byteSize do
			for k,v in pairs( { B='>', L='<' } ) do
				local pi = Pack( v .. 'i' .. i )
				local pu = Pack( v .. 'I' .. i )
				pAssert( pi,  'Int',  i, k )
				pAssert( pu,  'UInt', i, k )
			end
		end
	end,

	test_FloatPacker = function( self )
		Test.Case.describe( "Test Float Packers     'f', 'd', 'n'" )
		for k,v in pairs( { B='>', L='<' } ) do
			local pfl = Pack( v..'f' )
			local pdb = Pack( v..'d' )
			local pln = Pack( v..'n' )
			pAssert( pfl, 'Float', 4, k )
			pAssert( pdb, 'Float', 8, k )
			pAssert( pln, 'Float', self.byteSize, k )
		end
	end,

	test_StandardIntegerPacker = function( self )
		Test.Case.describe( "Test Standard Integer Packers 'bB','hH','iI','lL','jJ','T'" )
		local sizeFormat = { b=1, B=1, h=2, H=2, i=4, I=4, l=8, L=8,
		                     j=Pack.luanumsize, J=Pack.luanumsize,
		                     T=self.luaintsize }
		for f,s in pairs( sizeFormat ) do
			for e,x in pairs( { B='>', L='<' } ) do
				local pi = Pack( x ..''.. f )
				local fmtString = string.upper( f ) == f and 'UInt' or 'Int'
				pAssert( pi, fmtString, s, e )
			end
		end
	end,

	test_SizedBitPacker = function( self )
		Test.Case.describe( "Test Sized Bit Packers   'r1..n', 'R1..n'" )
		local mxBit = math.tointeger( math.log( math.maxinteger, 2 ) )
		for i=1,mxBit do
			local ps = Pack( 'r' .. i )
			local pu = Pack( 'R' .. i )
			pAssert( ps,  'SBit', i, 0 )
			pAssert( pu,  'UBit', i, 0 )
		end
	end,

	test_GenericBitPacker = function( self )
		Test.Case.describe( "Test Generic Bit Packers   'r', 'R', 'v'" )
		local ps = Pack( 'r' )
		local pu = Pack( 'R' )
		local pb = Pack( 'v' )
		pAssert( ps,  'SBit', 1, 0 )
		pAssert( pu,  'UBit', 1, 0 )
		pAssert( pb,  'Bool' )
	end,

	test_StringPacker = function( self )
		Test.Case.describe( "Test sized String Packers   'c1,cX,cY,cZ,cMax'" )
		for i,n in pairs( {1,7,36,583,9523,12845,778293,1234567,87654321,918273645,1073741824} ) do
			local pc = Pack( 'c' .. n )
			pAssert( pc,  'Raw', n )
		end
	end,
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )

