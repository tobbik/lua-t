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
local prxTblIdx              ,o_setElement  , o_getElement  , o_iters =
      Table.proxyTableIndex, Oht.setElement, Oht.getElement, Oht.iters

-- console colours  green    red      yellow   blue
local colors    = { PASS=32, FAIL=31, SKIP=33, TODO=36}
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

local printTst = function( nme, tst )
  print( ('[%dm%s[0m [%dms] [%dms] [%s] %s'):format( colors[ tst.status ], tst.status, tst.executionTime, tst.runTime, nme, tostring(tst) ) )
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

local function getPlan( tbl, sort )
  local plan = { }
  for name, case in pairs( tbl ) do
    if type( case )=='function' and name~='beforeAll' and name~='afterAll' and name~='beforeEach' and name~='afterEach' then
      t_insert( plan, name )
    end
  end
  if sort then table.sort(plan) end
  return plan
end

return setmetatable( { },
{
  __call   = function( self, tbl, sort, quiet )
    assert( 'table' == type( tbl ), "Test.Suite() requires a table as argument" )
    local suite, failedTests, startSuite = makeSuite( { } ), makeSuite( { } ), Loop.time( )
    if tbl.beforeAll then Test( tbl.beforeAll, tbl ) end
    for _, name in pairs( getPlan(tbl,sort) ) do
      local runTimeStart = Loop.time( )
      if tbl.beforeEach then Test( tbl.beforeEach, tbl ) end
      local ok, result = Test( tbl[ name ], tbl )
      o_setElement( suite[ prxTblIdx ], tostring(name), result )
      if tbl.afterEach then Test( tbl.afterEach, tbl ) end
      result.runTime = Loop.time( ) - runTimeStart
      if not ok then
        o_setElement( failedTests[ prxTblIdx ], tostring(name), result )
      end
      if not quiet then printTst( name, result ) end
    end
    if tbl.afterAll then Test( tbl.afterAll, tbl ) end
    return suite, Loop.time() - startSuite, #failedTests~=0 and failedTests or nil
  end
} )
