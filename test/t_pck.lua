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
	SetNegativeIntForUnsignedPackerFail = function( self )
		Test.describe( "Set negative value to unsigned packer fails" )
		local e ="packed value must fit the value range for the packer size"
		local b = Buffer( 2 )
		local p = Pack( 'H' )
		local f = function() p( b, -4 ) end
		local r,m   = pcall( f )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
	end,

	SetNumberTooBigUnsignedPackerFail = function( self )
		Test.describe( "Set too high value to unsigned packer fails" )
		local e ="packed value must fit the value range for the packer size"
		local b = Buffer( 2 )
		local p = Pack( 'H' )
		local f = function() p( b, 65537 ) end
		local r,m   = pcall( f )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
	end,

	SetNumberTooBigsignedPackerFail = function( self )
		Test.describe( "Set too high value to signed packer fails" )
		local e ="packed value must fit the value range for the packer size"
		local b = Buffer( 2 )
		local p = Pack( 'h' )
		local f = function() p( b, 32769 ) end
		local r,m   = pcall( f )
		assert( not r, "Setting value should have failed" )
		T.assert( m:match( e ), "Error Message should have been `%s', but was `%s`", e, m )
	end,
}
