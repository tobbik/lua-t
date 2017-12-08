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

local NB      = Pack.charbits


local tests = {

	-- Test cases
	test_SizedIntegerPacker = function( self )
		Test.Case.describe( "Sized Integer Packers    'i1..n', 'I1..n'" )
		local byteSize = (math.tointeger( math.log( math.maxinteger, 2 ) ) + 1) // NB
		for i=1,byteSize do
			for k,v in pairs( { b='>', l='<' } ) do
				local ps = Pack( v .. 'i' .. i )
				local pu = Pack( v .. 'I' .. i )
				T.assert( ps,  'Int'..i*NB..'s'..k,  i, i*NB )
				T.assert( pu,  'Int'..i*NB..'u'..k,  i, i*NB )
			end
		end
	end,

	test_FloatPacker = function( self )
		Test.Case.describe( "Float Packers            'f', 'd', 'n'" )
		local byteSize = (math.tointeger( math.log( math.maxinteger, 2 ) ) + 1) // NB
		for k,v in pairs( { b='>', l='<' } ) do
			local pfl = Pack( v..'f' )
			local pdb = Pack( v..'d' )
			local pln = Pack( v..'n' )
			T.assert( pfl, 'Float4'.. k, 4, 4*NB, k )
			T.assert( pdb, 'Float8'.. k, 8, 8*NB, k )
			T.assert( pln, 'Float'..byteSize..k, byteSize, byteSize*NB, k )
		end
	end,

	test_StandardIntegerPacker = function( self )
		Test.Case.describe( "Standard Integer Packers 'bB','hH','iI','lL','jJ','T'" )
		local sizeFormat = { b=1, B=1, h=2, H=2, i=4, I=4, l=8, L=8,
		                     j=Pack.numsize, J=Pack.numsize,
		                     T=Pack.numsize }
		for f,s in pairs( sizeFormat ) do
			for e,x in pairs( { b='>', l='<' } ) do
				local pi = Pack( x ..''.. f )
				local signed = string.upper( f ) == f and 'u' or 's'
				T.assert( pi, 'Int'..s*NB..signed..e, s, s*NB, e )
			end
		end
	end,

	test_SequenceConstructor = function( self )
		Test.Case.describe( "Different Sequence Packers: P('b','B','h','H')  P('bBhH') P('b B h H')" )
		local f  = { 'b','B','h','H','i','I' }
		local p1 = Pack( table.unpack( f      ) )   --  Pack('b','B','h','H','i','I')
		local p2 = Pack( table.concat( f, ''  ) )   --  Pack('bBhHiI')
		local p3 = Pack( table.concat( f, ' ' ) )   --  Pack('b B h H i I')
		for i,v in ipairs( p1 ) do
			local _,t1 = Pack.type( p1[i] )
			local _,t2 = Pack.type( p2[i] )
			local _,t3 = Pack.type( p3[i] )
			T.assert( t1  ==  t2,  'Sub-Type should be `%s` but was `%s`', t1, t2 )
			T.assert( t1  ==  t3,  'Sub-Type should be `%s` but was `%s`', t1, t3 )
		end
	end,

	test_SizedBitPacker = function( self )
		Test.Case.describe( "Sized Bit Packers        'r1..n', 'R1..n'" )
		local mxBit = math.tointeger( math.log( math.maxinteger, 2 ) )
		for i=1,mxBit do
			local ps = Pack( 'r' .. i )
			local pu = Pack( 'R' .. i )
			T.assert( ps,  'Int'..i..'sb', i//NB, i, 0 )
			T.assert( pu,  'Int'..i..'ub', i//NB, i, 0 )
		end
	end,

	test_GenericBitPacker = function( self )
		Test.Case.describe( "Generic Bit Packers      'r', 'R', 'v'" )
		local ps = Pack( 'r' )
		local pu = Pack( 'R' )
		local pb = Pack( 'v' )
		T.assert( ps,  'Int1sb', 0, 1, 0 )
		T.assert( pu,  'Int1ub', 0, 1, 0 )
		T.assert( pb,  'Bool' , 0, 1 )
	end,

	test_StringPacker = function( self )
		Test.Case.describe( "Sized String Packers     'c1, ... ,cMax'" )
		for i,n in pairs( {1,7,36,583,9523,12845,778293,1234567,87654321,918273645,1073741824} ) do
			local pc = Pack( 'c' .. n )
			T.assert( pc,  'Raw'..n, n, n*NB )
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

