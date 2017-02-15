#!../out/bin/lua

---
-- \file    test/t_tst.lua
-- \brief   Test for T.Test
local Test  = require ('t').Test

local tests = {
	setUp = function( self )
		-- base test elements -> all of them successful
		self.r = {
			setUp        = function( s )   s.something = 1 end,
			tearDown     = function( s )   s.something = nil end,
			doStuff      = function( s,x ) s.something = s.something +x end,
			test_success = function( s ) assert( true,  "This better be true" ) end
		}
	end,

	tearDown = function( self ) -- not necessary for this suite
	end,

	-- -----------------------------------------------------------------------
	-- Simple tests
	-- -----------------------------------------------------------------------

	test_Success = function( self, done )
		Test.describe( "Test to success" )
		self.r.test_Test = function( s ) assert( true, "This better works" ) end
		local t = Test( self.r )
		assert( t(), "Test suite should have worked" )
		assert( t.test_Test.pass, "Test.test_Test case suite should have passed" )
		assert( t.test_Test.message   == nil, "No Message   should be set" )
		assert( t.test_Test.location  == nil, "No Location  should be set" )
		assert( t.test_Test.traceback == nil, "No Traceback should be set" )
	end,

	test_Fails = function( self, done )
		Test.describe( "Test to fail" )
		local errorMsg = "This is supposed to fail !!!"
		self.r.test_Test = function( s ) assert( false, errorMsg ) end
		local t = Test( self.r )
		assert( not t(), "Test suite should have failed" )
		assert( not t.test_Test.pass, "Test.test_Test case suite should NOT have passed" )
		assert( t.test_Test.message == errorMsg, "Error Message   should be set" )
		assert( t.test_Test.location, "Error Location  should be set" )
		assert( t.test_Test.location:match(":%d+$"),
			"Error Location should contain trailing line number" )
		assert( t.test_Test.traceback, "Error Traceback should be set" )
		assert( t.test_Test.traceback:match("in function 'assert'"),
			"Error Traceback should contain 'assert' function reference" )
	end,

	test_TodoFails = function( self, done )
		Test.describe( "A failed TODO should NOT fail the suite" )
		self.r.test_Test = function( s ) Test.todo('todo'); assert( false, "This better fails" ) end
		local t = Test( self.r )
		assert( t(), "Test suite should have succed" )
		assert( t.test_Test.todo == "todo", "Test Todo reason should be set" )
		assert( not t.test_Test.pass, "Test.test_Test case suite should NOT have passed" )
	end,

	test_TodoSuccess = function( self, done )
		Test.describe( "A successful TODO should fail the suite" )
		self.r.test_Test = function( s ) Test.todo('todo'); assert( true,  "This better works" ) end
		local t = Test( self.r )
		assert( not t(), "Test suite should have failed" )
		assert( t.test_Test.todo == "todo", "Test Todo reason should be set" )
		assert( t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,

	test_Skip = function( self, done )
		Test.describe( "Skip should skip test" )
		self.r.test_Test = function( s ) Test.skip('skip me'); assert( false,  "This better fails" ) end
		local t = Test( self.r )
		assert( t(), "Test suite should not have failed because failing test got skipped" )
		assert( t.test_Test.skip == "skip me", "Test Skip reason should be set" )
		assert( t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,

	test_Description = function( self, done )
		Test.describe( "Test.describe should set test case description" )
		self.r.test_Test = function( s ) Test.describe('describe me'); assert( true,  "This better works" ) end
		local t = Test( self.r )
		assert( t(), "Test suite should not have failed." )
		assert( t.test_Test.description == "describe me", "Test Description reason should be set" )
		assert( t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,

}


t = Test( tests )
t( )
print( '\n\n\n\n\n' )
print( t )
