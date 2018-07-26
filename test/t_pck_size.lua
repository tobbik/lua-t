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


local tests = {

	-- Test cases
	test_StringPacker = function( self )
		Test.Case.describe( "Sized String Packers     'c1, ... ,cMax'" )
		local maxIntSize = math.floor( ((2 ^ (Pack.charbits*Pack.intsize - 1))-9) // 10 )
		for i,n in pairs( {1,7,36,583,9523,12845,778293,1234567,87654321,918273645,1073741824} ) do
			-- print("\n", n, maxIntSize, (Pack.charbits*Pack.intsize - 1), 2^(Pack.charbits*Pack.intsize - 1)  )
			if n > maxIntSize then Test.Case.skip("Size not supported on Sytem") end
			local pc = Pack( 'c' .. n )
			local sz_byte,sz_bit = Pack.size( pc )
			assert( n    == sz_byte, s_format( 'Expected size %d bytes, got %d', n   , sz_byte ) )
			assert( n*NB == sz_bit,  s_format( 'Expected size %d bytes, got %d', n*NB, sz_bit  ) )
		end
	end,
}

--t =  Test( tests )
--t()
--print(t)

return Test( tests )

