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

	-- Methods from here
	test_Now = function( self, done )
		Test.Case.describe( "Time t:now() resets timer value to time  since epoch" )
		local t  = Time( 123456 )
		local tm = os_time()
		t:now()
		t_assert( t.s == tm, "Seconds should be `%d` but was `%d`", tm, t.s )
	end,

	test_Since = function( self, done )
		Test.Case.describe( "Time t:since() measures time since last" )
		local t  = Time( )
		Time.sleep( 1234 )
		t:since()
		t_assert( t.s == 1, "Seconds should be `1` but was `%d`", t.s )
		t_assert( t.ms > 234 and t.ms < 239,
		   "Milliseconds should be between 234 and 239 but was `%d`", t.ms )
	end,

	-- Operands
	test_Add = function( self, done )
		Test.Case.describe( "t1 + t2 = t3 should add up" )
		local t1 = Time( 1784 )
		local t2 = Time( 4923 )
		local t  = t1 + t2
		t_assert( t.s  == 6,   "Seconds should be `%d` but was `%d`", 6, t.s )
		t_assert( t.ms == 707, "Milliseconds should be `%d` but was `%d`", 707, t.ms )
	end,

	test_Sub = function( self, done )
		Test.Case.describe( "t1 - t2 = t3 should add up" )
		local t1 = Time( 4723 )
		local t2 = Time( 1984 )
		local t  = t1 - t2
		t_assert( t.s  == 2,   "Seconds should be `%d` but was `%d`", 2, t.s )
		t_assert( t.ms == 739, "Milliseconds should be `%d` but was `%d`", 739, t.ms )
	end,

}

return Test( tests )
