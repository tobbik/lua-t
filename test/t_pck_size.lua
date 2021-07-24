---
-- \file      test/t_pck_size.lua
-- \brief     Test for t.Pack sizes
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h


local t_require = require( "t") .require
local Test      = require( "t.Test" )
local Pack      = require( "t.Pack" )
local s_format  = string.format

local NB        = Pack.charbits


return {

	-- Test cases
	StringPacker = function( self )
		Test.describe( "Sized String Packers     'c1, ... ,cMax'" )
		for i,n in pairs( {1,7,36,583,9523,12845,778293,1234567,87654321,214748363} ) do
			local pc = Pack( 'c' .. n )
			local sz_byte,sz_bit = Pack.size( pc )
			assert( n    == sz_byte, s_format( 'Expected size %d bytes, got %d', n   , sz_byte ) )
			assert( n*NB == sz_bit,  s_format( 'Expected size %d bytes, got %d', n*NB, sz_bit  ) )
		end
	end,
}
