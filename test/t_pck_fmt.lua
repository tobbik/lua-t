---
-- \file      test/t_pck_fmt.lua
-- \brief     Test for t.Pack (binary packing/unpacking)
--            Testing that the various formatting options are creating the
--            appropriate internal Packer types.  This is only testing atomic
--            packers.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h


local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local pAssert = T.require'assertHelper'.Packer

local NB      = Pack.charbits


local tests = {

	-- Test cases
	test_SizedIntegerPacker = function( self )
		Test.Case.describe( "Sized Integer Packers    'i1..n', 'I1..n'" )
		local byteSize = (math.tointeger( math.log( math.maxinteger, 2 ) ) + 1) // NB
		for i=1,byteSize do
			for k,v in pairs( { B='>', L='<' } ) do
				local pi = Pack( v .. 'i' .. i )
				local pu = Pack( v .. 'I' .. i )
				pAssert( pi,  'Int'..i,  i, i*NB, k )
				pAssert( pu,  'UInt'..i, i, i*NB, k )
			end
		end
	end,

	test_FloatPacker = function( self )
		Test.Case.describe( "Float Packers            'f', 'd', 'n'" )
		local byteSize = (math.tointeger( math.log( math.maxinteger, 2 ) ) + 1) // NB
		for k,v in pairs( { B='>', L='<' } ) do
			local pfl = Pack( v..'f' )
			local pdb = Pack( v..'d' )
			local pln = Pack( v..'n' )
			pAssert( pfl, 'Float4', 4, 4*NB, k )
			pAssert( pdb, 'Float8', 8, 8*NB, k )
			pAssert( pln, 'Float'..byteSize, byteSize, byteSize*NB, k )
		end
	end,

	test_StandardIntegerPacker = function( self )
		Test.Case.describe( "Standard Integer Packers 'bB','hH','iI','lL','jJ','T'" )
		local sizeFormat = { b=1, B=1, h=2, H=2, i=4, I=4, l=8, L=8,
		                     j=Pack.numsize, J=Pack.numsize,
		                     T=self.numsize }
		for f,s in pairs( sizeFormat ) do
			for e,x in pairs( { B='>', L='<' } ) do
				local pi = Pack( x ..''.. f )
				local fmtString = string.upper( f ) == f and 'UInt' or 'Int'
				pAssert( pi, fmtString..s, s, s*NB, e )
			end
		end
	end,

	test_SizedBitPacker = function( self )
		Test.Case.describe( "Sized Bit Packers        'r1..n', 'R1..n'" )
		local mxBit = math.tointeger( math.log( math.maxinteger, 2 ) )
		for i=1,mxBit do
			local ps = Pack( 'r' .. i )
			local pu = Pack( 'R' .. i )
			pAssert( ps,  'SBit'..i, i//NB, i, 0 )
			pAssert( pu,  'UBit'..i, i//NB, i, 0 )
		end
	end,

	test_GenericBitPacker = function( self )
		Test.Case.describe( "Generic Bit Packers      'r', 'R', 'v'" )
		local ps = Pack( 'r' )
		local pu = Pack( 'R' )
		local pb = Pack( 'v' )
		pAssert( ps,  'SBit1', 0, 1, 0 )
		pAssert( pu,  'UBit1', 0, 1, 0 )
		pAssert( pb,  'Bool' , 0, 1 )
	end,

	test_StringPacker = function( self )
		Test.Case.describe( "Sized String Packers     'c1, ... ,cMax'" )
		for i,n in pairs( {1,7,36,583,9523,12845,778293,1234567,87654321,918273645,1073741824} ) do
			local pc = Pack( 'c' .. n )
			pAssert( pc,  'Raw'..n, n, n*NB )
		end
	end,

	test_ConstructorReuse = function( self )
		Test.Case.describe( "Create Packer from Pack.Field instances" )
		local fmt = '>I3<i2bB>I5<I4h'
		local p1  = Pack( fmt )
		T.assert( 7 == #p1, "Got Packer length: %d; expected:%d", #p1, 7 )
		local p2  = Pack( p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7] )
		T.assert( #p2 == #p1, "Got Packer length: %d; expected:%d", #p2, #p1 )
		for i=1,#p1 do
			local s1,s2 = tostring(p1[i]):match("(.*):"), tostring(p2[i]):match("(.*):"), 
			T.assert( s1 == s2, "Got Packer length: %s; expected:%s", s2, s1 )
		end
		-- mix existing t.Pack instances and new format fields
		local p3  = Pack( ">I3", p1[2], "b", "B", p1[5], p1[6], "h" )
		T.assert( #p3 == #p1, "Got Packer length: %d; expected:%d", #p3, #p1 )
		for i=1,#p1 do
			local s1,s3 = tostring(p1[i]):match("(.*):"), tostring(p3[i]):match("(.*):"), 
			T.assert( s1 == s3, "Got Packer length: %s; expected:%s", s3, s1 )
		end
	end
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )
