---
-- \file    test/t_tst.lua
-- \brief   Test for T.Test
local Test   = require( 't.Test' )
local Suite  = require( 't.Test.Suite' )

return {
	beforeEach = function( self )
		-- base test elements -> all of them successful
		self.t = {
			beforeEach   = function( s )   s.something = 1 end,
			afterEach    = function( s )   s.something = nil end,
			doStuff      = function( s,x ) s.something = s.something + (x and x or 0) end,
			success      = function( s )
				assert( true,  "This better be true" )
				local a = s.something
				s:doStuff( 14 )
				assert( s.something == a+14 )
			end
		}
	end,

	-- -----------------------------------------------------------------------
	-- Simple tests
	-- -----------------------------------------------------------------------

	Success = function( self, done )
		Test.describe( "Test to success" )
		self.t.Case = function( s ) assert( true, "This better works" ) end
		local suite, time, fails = Suite( self.t, false, true )
		assert( fails == nil               , "No Failed Test should have returned" )
		assert( suite.Case.pass            , "Test.TestSuccess case suite should have passed" )
		assert( suite.Case.status == "PASS", "Test.TestSuccess case suite should have passed" )
		assert( suite.Case.message   == nil, "No Message   should be set" )
		assert( suite.Case.location  == nil, "No Location  should be set" )
		assert( suite.Case.traceback == nil, "No Traceback should be set" )
	end,

	Fails = function( self, done )
		Test.describe( "Test to fail" )
		local errorMsg = "This is supposed to fail !!!"
		self.t.Case = function( s ) assert( false, errorMsg ) end
		local suite, time, fails = Suite( self.t, false, true )
		assert( 'table' == type( fails ), "Failed Test table should have returned" )
		assert( not suite.Case.pass, "Test.test_Test case suite should NOT have passed" )
		assert( suite.Case.message == errorMsg, "Error Message should be set" )
		assert( suite.Case.location, "Error Location  should be set" )
		assert( suite.Case.location:match(":%d+$"),
			"Error Location should contain trailing line number" )
		assert( suite.Case.traceback, "Error Traceback should be set" )
		assert( suite.Case.traceback:match( "in function 'assert'" ),
			"Error Traceback should contain 'assert' function reference" )
	end,

	Skip = function( self, done )
		Test.describe( "Skip should skip test" )
		self.t.Case = function( s ) Test.skip('skip me'); assert( false, "This better fails" ) end
		local suite, time, fails = Suite( self.t, false, true )
		assert( fails == nil                   , "No Failed Test should have returned" )
		assert( suite.Case.message == "skip me", "Test Skip reason should be set" )
		assert( suite.Case.pass                , "Test.Case case suite should have passed" )
		assert( tostring(suite.Case):match( "# SKIP: skip me" ),
			("`# SKIP: __reason__` shall occur in test description, but was <%s>"):format( tostring(suite.Case) ) )
	end,

	Description = function( self, done )
		Test.describe( "Test.describe should set test case description" )
		self.t.Case = function( s ) Test.describe('describe me'); assert( true,  "This better works" ) end
		local suite, time, fails = Suite( self.t, false, true )
		assert( fails == nil               , "No Failed Test should have returned" )
		assert( suite.Case.description == "describe me", "Test Description reason should be set" )
		assert( suite.Case.pass, "Test.test_Test case suite should have passed" )
	end,

	Todo = function( self, done )
		Test.describe( "TODO should mark test as todo" )
		self.t.Case = function( s ) Test.todo('todo me'); assert( false, "This better fails" ) end
		local suite, time, fails = Suite( self.t, false, true )
		assert( fails == nil                   , "No Failed Test should have returned" )
		assert( suite.Case.message == "todo me", "Test Todo reason should be set" )
		assert( suite.Case.pass                , "Test.Case case suite should have passed" )
		assert( tostring(suite.Case):match( "# TODO: todo me" ),
			("`# TODO: __reason__` shall occur in test description, but was <%s>"):format( tostring(suite.Case) ) )
	end,

	--]]
}
