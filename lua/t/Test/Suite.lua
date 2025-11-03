-- vim: ts=2 sw=2 sts=2 et list
-- \file      lua/Test/Suite.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test suite implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local     Test,            Loop,          T =
require't.Test', require't.Loop', require't'

local           Table,            Oht  =
      require"t.Table", require"t.OrderedHashTable"
local t_concat    , t_insert    , t_sort    , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, table.sort, getmetatable, setmetatable, pairs, assert, type
local prxTblIdx            , t_keys,     o_setElement  , o_getElement  , o_iters =
      Table.proxyTableIndex, Table.keys, Oht.setElement, Oht.getElement, Oht.iters
local _mt

local yamlHeader = function( suite )
  return ("1..%d\n"):format( #suite )
end

local formatTest = function( nme, idx, tst, verbosity, color )
  if not verbosity or "QUIET" == verbosity then
    return
  elseif "COMPACTINFO" == verbosity then
    return tst:toCompact(color, nme) .. tst:toYaml('v_INFO')
  elseif "TAP" == verbosity then
    return tst:toTap(color, idx) .. (tst.pass and "" or "\n  ---\n"..tst:toYaml('v_FAIL').."\n  ...")
  elseif "TAPVERBOSE" == verbosity then
    return tst:toTap(color, idx) .. (tst.pass and "" or "\n  ---\n"..tst:toYaml('v_FULL').."\n  ...")
  else -- default to "COMPACT"
    return tst:toCompact(color,nme) .. (tst.pass and "" or "\n  ---\n"..tst:toYaml('v_FAIL').."\n  ...")
  end
end

-- ---------------------------- general helpers  --------------------
-- assert Test type and return the proxy table
local getPrx = function( self )
  assert( _mt == getmetatable( self ), ("Expected `%s`, got %s"):format( _mt.__name, T.type( self ) ) )
  return self[ prxTblIdx ]
end

-- create a Test instance from a table
local makeSuite = function( prx )
  return setmetatable( { [ prxTblIdx ] = prx }, _mt )
end

local function createPlan( tbl, sort )
  local plan, ignore = { }, { beforeAll=true, afterAll=true, beforeEach=true, afterEach=true }
  for name, case in pairs( tbl ) do
    if not ignore[ name ] then t_insert( plan, name ) end
  end
  if sort then table.sort(plan) end
  return plan
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
  -- essentials
  __name     = "t.Test.Suite",
  __len      = function( self )      return #getPrx( self )                      end,
  __pairs    = function( self )      return o_iters( getPrx( self ), false )     end,
  __ipairs   = function( self )      return o_iters( getPrx( self ), true )      end,
  __index    = function( self, key ) return o_getElement( getPrx( self ), key )  end,
  __add      = function( self, oth )
    local res = makeSuite( {} )
    for k,v in o_iters( self, false ) do o_setElement( getPrx( res ), k, v ) end
    for k,v in o_iters( oth , false ) do o_setElement( getPrx( res ), k, v ) end
    return res
  end,
  __newindex = function( self, key, val )
    assert( false, "Overwriting members is not allowed" )
  end,
  __tostring = function( self )
    local buf = { }
    t_insert( buf, yamlHeader( self ))
    for i,tst in ipairs( self ) do
      t_insert( buf, formatTest( _, i, tst, "TAPVERBOSE", false ) )
    end
    return t_concat( buf, "\n" )
  end
}

return setmetatable(
  {
    _VERSION     = _mt.__name .. ' 0.1.0',
    _DESCRIPTION = 'lua-t unit-testing suite test harness.',
    _URL         = 'https://gitlab.com/tobbik/lua-t',
    _LICENSE     = 'MIT',
  },
  {
    __call   = function( _, tbl, verbosity, color, sort )
      assert( 'table' == type( tbl ), "Test.Suite() requires a table as argument" )
      local suite, failedTests, startSuite, testPlan = makeSuite({}), makeSuite({}), Loop.timemonotonic(), createPlan(tbl,sort)
      if tbl.beforeAll then Test(tbl.beforeAll, tbl) end
      for idx, name in pairs(testPlan) do
        assert(type(tbl[name]) == "function", ("Expected `Function` for <%s>, got `%s`"):format(name, type(tbl[name])))
        local runTimeStart = Loop.timemonotonic( )
        if tbl.beforeEach then Test(tbl.beforeEach, tbl) end
        local result, ok = Test(tbl[ name ], tbl)
        o_setElement( suite[ prxTblIdx ], tostring(name), result )
        if tbl.afterEach then Test(tbl.afterEach, tbl) end
        result.runTime = Loop.timemonotonic( ) - runTimeStart
        if not ok then
          o_setElement( failedTests[ prxTblIdx ], tostring(name), result )
        end
        print( formatTest( name, idx, result, verbosity, color ) )
      end
      if tbl.afterAll then Test( tbl.afterAll, tbl ) end
      return suite, Loop.timemonotonic() - startSuite, #failedTests~=0 and failedTests or nil
    end
  }
)
