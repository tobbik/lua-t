-- vim: ts=2 sw=2 sts=2 et
-- \file      lua/Test/Suite.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test suite implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local     Test,            Loop,          T =
require't.Test', require't.Loop', require't'

local           Table,            Oht  =
      require"t.Table", require"t.OrderedHashTable"
local t_concat    , t_insert    , format       , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, string.format, getmetatable, setmetatable, pairs, assert, type
local t_clone     , prxTblIdx              ,o_setElement  , o_getElement  , o_iters =
      Table.clone , Table.proxyTableIndex, Oht.setElement, Oht.getElement, Oht.iters

local _mt

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

local printTst = function( nme, tst, tme )
  -- console colours      green    red      yellow   blue
  local colors        = { PASS=32, FAIL=31, SKIP=33, TODO=36}
  print( ('[%dm%s[0m [%dms] [%s] %s'):format( colors[ tst.status ], tst.status, tme, nme, tostring(tst) ) )
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
  -- essentials
  __name     = "t.Test.Suite",
  __len      = function( self )      return #getPrx( self )                      end,
  __pairs    = function( self )      return o_iters( getPrx( self ), false )     end,
  __ipairs   = function( self )      return o_iters( getPrx( self ), true )      end,
  __index    = function( self, key ) return o_getElement( getPrx( self ), key )  end,
  __newindex = function( self, key, val )
    assert( false, "Overwriting members is not allowed" )
  end,
  __tostring = function( self )
    local buf = { }
    t_insert( buf, format( "1..%d", #self ) )
    for i,tst in ipairs( self ) do
      t_insert( buf, format( "\n%s %d %s",
        tst.pass and 'ok' or 'not ok', i, tostring(tst) ) )
      if "FAIL" == tst.status then
        t_insert( buf, "\n" .. Test.tapOutput( tst ) )
      end
    end
    return t_concat( buf, "" )
  end
}

local function getPlan( tbl )
  local plan = { }
  if tbl.beforeAll then t_insert(plan, "beforeAll") end
  for name, case in pairs( tbl ) do
    if type(case)=='function' and name~='beforeAll' and name~='afterAll' and name~='beforeEach' and name~='afterEach' then
      if tbl.beforeEach then t_insert(plan, "beforeEach") end
      t_insert( plan, name )
      if tbl.afterEach then t_insert(plan, "afterEach") end
    end
  end
  if tbl.afterAll then t_insert(plan, "afterAll") end
  return plan
end

return setmetatable( {
}, {
  __call   = function( self, tbl, quiet )
    local failedTests = { }
    if tbl and 'table' == type( tbl ) then
      local suite, startSuite = makeSuite( { } ), Loop.time( )
      for _, name in pairs( getPlan(tbl) ) do
        local startTest = Loop.time( )
        local ok, result = Test( tbl[name], tbl )
        if not ok then
          t_insert( failedTests, result  )
        end
        if name~='beforeAll' and name~='afterAll' and name~='beforeEach' and name~='afterEach' then
          o_setElement( suite[ prxTblIdx ], name, result )
          result.runTime = Loop.time( ) - startTest
          if not quiet then printTst( name, result, result.runTime ) end
        end
      end
      return suite, Loop.time() - startSuite, #failedTests~=0 and failedTests or nil
    else
      return makeSuite( { } ), 0
    end
  end
} )
