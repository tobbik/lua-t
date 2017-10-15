#!../out/bin/lua

---
-- \file    t_ael.lua
-- \brief   Test for the t.Time
local t_assert  = require"t".assert
local   Test    = require't.Test'
local   Time    = require't.Time'
local os_time   = os.time


local   tests = {

	-- -----------------------------------------------------------------------
	-- Timer Tests
	-- -----------------------------------------------------------------------

	test_Create = function( self, done )
		Test.Case.describe( "Time() create timer since epoch" )
		local tm = os_time()
		local t  = Time()
		t_assert( t.s == tm, "Seconds should be `%d` but was `%d`", tm, t.s )
	end,

	test_CreateMs = function( self, done )
		Test.Case.describe( "Time( 4321 ) set in milliseconds" )
		local t  = Time( 4321 )
		t_assert( t.s  == 4, "Seconds should be `%d` but was `%d`", 4, t.s )
		t_assert( t.ms == 321, "Milli seconds should be `%d` but was `%d`", 321, t.ms )
		t_assert( t.us == 321000, "Micro seconds should be `%d` but was `%d`", 321000, t.us )
	end,

	test_CreateZero = function( self, done )
		Test.Case.describe( "Time(0) creates zero duration timer" )
		local t  = Time( 0 )
		t_assert( t.s  == 0, "Seconds should be `%d` but was `%d`", 9, t.s )
		t_assert( t.ms == 0, "Milli seconds should be `%d` but was `%d`", 0, t.ms )
		t_assert( t.us == 0, "Micro seconds should be `%d` but was `%d`", 0, t.us )
	end,


}

return Test( tests )
