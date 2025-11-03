---
-- \file    test/t_tst.lua
-- \brief   Test for T.Test
local Test   = require( 't.Test' )
local T      = require( 't' )

return {
  -- -----------------------------------------------------------------------
  -- Simple tests
  -- -----------------------------------------------------------------------

  Success = function(self)
    Test.describe( "Test to success" )
    local tfn = function(s) assert( true, "This better works" ) end
    local result,ok = Test(tfn)
    assert(ok                     , "Test execution should have succeeded")
    assert(result.pass            , "Test.pass should be true" )
    assert(result.status == "PASS", "Test.severity should be <PASS>" )
    assert(result.message   == nil, "No Test.Message   should be set" )
    assert(result.location  == nil, "No Test.Location  should be set" )
    assert(result.traceback == nil, "No Test.Traceback should be set" )
  end,

  Fails = function(self)
    Test.describe("Test to fail")
    local errorMsg = "This is supposed to fail !!!"
    local tfn = function(s) assert(false, errorMsg) end
    local result,ok = Test(tfn)
    assert(not ok                    , "Test execution should have failed")
    assert(not result.pass           , "Test.pass should be false")
    assert(result.status == "FAIL"   , "Test.severity should be <PASS>")
    assert(result.message == errorMsg, "No Test.Message   should be set")
    assert(result.location:match(":%d+$"),
      "Error Location should contain trailing line number" )
    assert(result.traceback, "Error Traceback should be set" )
    assert(result.traceback:match( "in function 'assert'" ),
      "Error Traceback should contain 'assert' function reference" )
  end,

  Skip = function(self)
    Test.describe("Skip should skip test")
    local tfn = function(s) Test.skip('skip me'); assert(false, "This better fails") end
    local result,ok = Test(tfn)
    assert(ok                     , "Test execution should have succeeded")
    assert(result.pass            , "Test.pass should be true")
    assert(result.status == "SKIP", "Test.severity should be <SKIP>")
    assert(result.message == "skip me", "Test Skip reason should be set")
    assert(tostring(result):match( "# SKIP: skip me"),
      ("`# SKIP: __reason__` shall occur in test description, but was <%s>"):format(tostring(result)))
  end,

  Todo = function(self)
    Test.describe("TODO should mark test as todo")
    local tfn = function(s) Test.todo('todo me'); assert(false, "This better fails") end
    local result,ok = Test(tfn)
    assert(ok                     , "Test execution should have succeeded")
    assert(result.pass            , "Test.pass should be true")
    assert(result.status == "TODO", "Test.severity should be <TODO>")
    assert(result.message == "todo me", "Test Todo reason should be set")
    assert(tostring(result):match( "# TODO: todo me"),
      ("`# TODO: __reason__` shall occur in test description, but was <%s>"):format(tostring(result)))
  end,

  Description = function(self)
    Test.describe("Test.describe should set test case description")
    local tfn = function(s) Test.describe('describe me'); assert(true,  "This better works") end
    local result,ok = Test(tfn)
    assert(ok                     , "Test execution should have succeeded")
    assert(result.description == "describe me", "Test Description reason should be set")
    assert(result.pass            , "Test.test_Test case suite should have passed")
  end,

  ExternalCodeFails = function( self )
    Test.describe("externalcode is failing")
    local failureModule = T.require'tst.external'
    local expected = "fails = function"
    local tfn = function() Test.describe("Failure");failureModule.fails() end
    local result,ok = Test(tfn)
    assert(not ok                     , "Test execution should have failed")
    assert(not result.pass            , "Test.test_Test case suite should not have passed")
    assert(result.location:match("tst/external.lua:%d+$"),
      "Error Location should point to external file" )
    assert(result.failedSource[5]:match(expected),
      ("Failed Source should contain expected text <%s> but was <%s>"):format(expected, result.failedSource[5]) )
    --print(result:toYaml('v_FULL'))
  end,

}
