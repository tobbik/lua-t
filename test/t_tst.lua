#!../out/bin/lua

---
-- \file    test/t_tst.lua
-- \brief   Test for T.Test
local Test  = require( 't' ).Test

local tests = {
	beforeEach = function( self )
		-- base test elements -> all of them successful
		self.t = Test( {
			beforeEach   = function( s )   s.something = 1 end,
			afterEach    = function( s )   s.something = nil end,
			doStuff      = function( s,x ) s.something = s.something +x end,
			test_success = function( s ) assert( true,  "This better be true" ) end
		} )
	end,

	afterEach = function( self ) -- not necessary for this suite
	end,

	-- -----------------------------------------------------------------------
	-- Simple tests
	-- -----------------------------------------------------------------------

	test_Success = function( self, done )
		Test.Case.describe( "Test to success" )
		self.t.test_Test = function( s ) assert( true, "This better works" ) end
		assert( self.t(), "Test suite should have succeeded" )
		assert( self.t.test_Test.pass, "Test.test_Test case suite should have passed" )
		assert( self.t.test_Test.message   == nil, "No Message   should be set" )
		assert( self.t.test_Test.location  == nil, "No Location  should be set" )
		assert( self.t.test_Test.traceback == nil, "No Traceback should be set" )
	end,

	test_Fails = function( self, done )
		Test.Case.describe( "Test to fail" )
		local errorMsg = "This is supposed to fail !!!"
		self.t.test_Test = function( s ) assert( false, errorMsg ) end
		assert( not self.t(), "Test suite should have failed" )
		assert( not self.t.test_Test.pass, "Test.test_Test case suite should NOT have passed" )
		assert( self.t.test_Test.message == errorMsg, "Error Message should be set" )
		assert( self.t.test_Test.location, "Error Location  should be set" )
		assert( self.t.test_Test.location:match(":%d+$"),
			"Error Location should contain trailing line number" )
		assert( self.t.test_Test.traceback, "Error Traceback should be set" )
		assert( self.t.test_Test.traceback:match( "in function 'assert'" ),
			"Error Traceback should contain 'assert' function reference" )
	end,

	test_TodoFails = function( self, done )
		Test.Case.describe( "A failed TODO should NOT fail the suite" )
		self.t.test_Test = function( s ) Test.Case.todo('todo'); assert( false, "This better fails" ) end
		assert( self.t(), "Test suite should have succeeded" )
		assert( self.t.test_Test.todo == "todo", "Test Todo reason should be set" )
		assert( not self.t.test_Test.pass, "Test.test_Test case suite should NOT have passed" )
	end,

	test_TodoSuccess = function( self, done )
		Test.Case.describe( "A successful TODO should fail the suite" )
		self.t.test_Test = function( s ) Test.Case.todo('todo'); assert( true,  "This better works" ) end
		assert( not self.t(), "Test suite should have failed" )
		assert( self.t.test_Test.todo == "todo", "Test Todo reason should be set" )
		assert( self.t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,

	test_Skip = function( self, done )
		Test.Case.describe( "Skip should skip test" )
		self.t.test_Test = function( s ) Test.Case.skip('skip me'); assert( false,  "This better fails" ) end
		assert( self.t(), "Test suite should not have failed because failing test got skipped" )
		assert( self.t.test_Test.skip == "skip me", "Test Skip reason should be set" )
		assert( self.t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,

	test_Description = function( self, done )
		Test.Case.describe( "Test.describe should set test case description" )
		self.t.test_Test = function( s ) Test.Case.describe('describe me'); assert( true,  "This better works" ) end
		assert( self.t(), "Test suite should not have failed." )
		assert( self.t.test_Test.description == "describe me", "Test Description reason should be set" )
		assert( self.t.test_Test.pass, "Test.test_Test case suite should have passed" )
	end,
}


t = Test( tests )
t( )
print( '\n\n\n\n\n' )
print( t )
