#!../out/bin/lua

---
-- \file    t_ael.lua
-- \brief   Test for the Set functionality
local   Test = require ('t.Test')
local   Loop = require ('t.Loop')
local   Time = require ('t.Time')

local   tests = {
	beforeAll = function( self, done )
		self.loop = Loop( 5 )
		done()
	end,

	beforeEach_cb = function( self, done )
		self.loop:addTimer( Time(1), done )
		self.loop:run( );
	end,

	afterEach_cb = function( self, done )  -- not necessary for this suite
		self.loop:stop( );
		done()
	end,

	-- -----------------------------------------------------------------------
	-- Timer Tests
	-- -----------------------------------------------------------------------

	test_cb_Timer = function( self, done )
		print("Da test...")
		Test.Case.describe( "Test simple Timer" )
		local arg1,arg2  = 100, 'this is a string'
		local tm, tmx    = Time( 5000 ), Time( )
		local success    = function(a,b)
			tmx:since()
			local ms_passed = tmx:get()
			assert( a == arg1, "First argument should be " .. arg1 )
			assert( b == arg2, "Second argument should be " .. arg2 )
			assert( tmx:get() > 4500 and tmx:get() < 5500,
				"Time passed should be between 4500 and 5500 milliseconds" )
			done()
		end
		self.loop:addTimer( tm, success, arg1, arg2 )
		self.loop:show()
	end,
}


return Test( tests )
