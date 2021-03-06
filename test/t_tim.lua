#!../out/bin/lua

---
-- \file    t_ael.lua
-- \brief   Test for the t.Time
local t_assert = require"t".assert
local Test     = require't.Test'
local Time     = require't.Time'
local os_time  = os.time


local   tests = {

	-- -----------------------------------------------------------------------
	-- Timer Tests
	-- -----------------------------------------------------------------------

	test_Create = function( self, done )
		Test.Case.describe( "Time() create timer since epoch" )
		local tm = os_time( )
		local t  = Time( )
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

	test_CreateClone = function( self, done )
		Test.Case.describe( "Time( tm ) creates a clone of timer" )
		local t  = Time( )
		local tc = Time( t )
		t_assert( t.s  == tc.s,  "Seconds should be `%d` but was `%d`",       t.s,  tc.s )
		t_assert( t.ms == tc.ms, "Milli seconds should be `%d` but was `%d`", t.ms, tc.ms )
		t_assert( t.us == tc.us, "Micro seconds should be `%d` but was `%d`", t.us, tc.us )
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
		local s_s, ms_min, ms_max = 1, 232, 238
		Time.sleep( 1234 )
		t:since()
		t_assert( t.s == s_s, "Seconds should be `%d` but was `%d`", s_s, t.s )
		t_assert( t.ms > ms_min and t.ms < ms_max,
		   "Milliseconds should be between `%d` and `%d` but was `%d`", ms_min, ms_max, t.ms )
	end,

	test_Set = function( self, done )
		Test.Case.describe( "Time t:set( ) milliseconds sets time properly" )
		local t  = Time( )
		t:set( 123456 )
		t_assert( t.s  == 123, "Seconds should be `123` but was `%d`", t.s )
		t_assert( t.ms == 456, "MilliSeconds should be `456` but was `%d`", t.ms )
		t_assert( t.us == 456000, "MicroSeconds should be `456000` but was `%d`", t.us )
	end,

	test_Get = function( self, done )
		Test.Case.describe( "Time t:get( ) milliseconds gets time properly" )
		local t  = Time( )
		t.s, t.us = 123, 456789
		t_assert( t:get()  == 123456, "t:get() should return `123456` but was `%d`", t:get() )
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

	test_Equals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for equality" )
		local t1 = Time( 4723 )
		local t2 = Time( 4723 )
		assert( t1 == t2, "Identical t.Time values must measure equal" )
	end,

	test_NotEquals = function( self )
		Test.Case.describe( "__eq metamethod properly compares for inequality" )
		local t1 = Time( 4723 )
		local t2 = Time( 4724 )
		local t3 = Time( 5723 )
		assert( t1 ~= t2, "Non Identical microseconds t.Time values must measure inequal" )
		assert( t1 ~= t3, "Non Identical seconds t.Time values must measure inequal" )
	end,

}

return Test( tests )
