---
-- \file      test/t_pck.lua
-- \brief     Test for t.Pack (binary packing/unpacking)
--            General, Error checking, etc
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h


local T       = require( "t" )
local Test    = require( "t.Test" )
local Pack    = require( "t.Pack" )
local Buffer  = require( "t.Buffer" )
local        unpack,        char,        rep,        format  =
      string.unpack, string.char, string.rep, string.format

local NB      = Pack.charbits

return {
	-- Test cases
	SetOutofRangeIntUnsignedFail = function( self )
		Test.describe( "Set out of ranged value to unsigned packer fails" )
		local e ="packed value must fit the value range for the packer size"
		local b = Buffer( 2 )
		local p = Pack( 'H' )
		local f = function( x ) p( b, x ) end
		local r,m   = pcall( f, -1 )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
		local r,m   = pcall( f, 2^16 + 1 )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
	end,

	SetOutofRangeIntSignedFail = function( self )
		Test.describe( "Set out of ranged value to signed packer fails" )
		local e ="packed value must fit the value range for the packer size"
		local b = Buffer( 2 )
		local p = Pack( 'h' )
		local x = math.tointeger( 2^15 )
		local f = function( x ) p( b, x ) end
		local r,m   = pcall( f, 0-x-1 )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
		local r,m   = pcall( f, x )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
	end,

	SetMinOrMaxIntUnsigned = function( self )
		Test.describe( "Set min or max ranged value to unsigned packer" )
		local b = Buffer( 2 )
		local p = Pack( 'H' )
		p( b, 0 )
		T.assert( 0 == p( b ), "Packed Value should be %d but was %d", 0, p( b ) )
		p( b, 2^16-1 )
		T.assert( 2^16-1 == p( b ), "Packed Value should be %d but was %d", 2^16-1, p( b ) )
	end,

	SetMinOrMaxIntSigned = function( self )
		Test.describe( "Set min or max ranged value to signed packer" )
		local b = Buffer( 2 )
		local x = math.tointeger( 2^15 )
		local p = Pack( 'h' )
		p( b, 0 - x )
		T.assert( 0-x == p( b ), "Packed Value should be %d but was %d", 0-x, p( b ) )
		p( b, x-1 )
		T.assert( x-1 == p( b ), "Packed Value should be %d but was %d", x-1, p( b ) )
	end,

	SignedSetWrongTypeFails = function( self )
		Test.describe( "Set wrong value type to signed packer fails" )
		local e_bool   ="number expected, got boolean"
		local e_string ="number expected, got string"
		local e_func   ="number expected, got function"
		local b = Buffer( 8 )
		local p = Pack( 'l' )
		local f = function( x ) p( b, x ) end
		local r,m   = pcall( f, true )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e_bool ), "Error Message should have been `%s', but was `%s`", e_bool, m )
		r,m   = pcall( f, 'foobar' )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e_string ), "Error Message should have been `%s', but was `%s`", e_string, m )
		r,m   = pcall( f, print )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e_func ), "Error Message should have been `%s', but was `%s`", e_func, m )
	end,

}
