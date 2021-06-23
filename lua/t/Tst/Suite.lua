-- \file      lua/Test/Suite.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test suite implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local    Test ,            Time,          T =
require't.Tst', require't.Time', require't'

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
local makeTst = function( prx )
  return setmetatable( { [ prxTblIdx ] = prx }, _mt )
end

local printTst = function( nme, tst, tme )
  -- console colours      green      red        yellow      blue
  local colors        = { passed=32, failed=31, skipped=33, todo=36}
  print( ('[%dm%s[0m [%dms] [%s] %s'):format( colors[ tst.status ], tst.status, tme.ms, nme, tostring(tst) ) )
end

local addTapDiagnostic = function( tst )
  local fields, diag = { "description", "executionTime", "status",
                 "message", "location", "traceback", "failedSource" }, { "  ---" }
  for i, fld in ipairs(fields) do
    local val = tst[ fld ] and tostring(tst[ fld ]):gsub( "\n", "\n  " ) or ''
    t_insert( diag, ( "\n  %s: %s"):format( fld, val ) )
  end
  t_insert( diag, "\n  ..." )
  return t_concat( diag, "" )
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
    local prx = getPrx( self )
    if "number"==type(key) then assert( key%1==0, "Can't set or overwrite numeric indices" ) end
    --if "string"==type(key) and key:match( "^test_" ) then
    if "string"==type(key) then
      o_setElement( prx, key, val )
    else
      prx[ key ] = val
    end
  end,
  __tostring = function( self )
    local buf = { }
    t_insert( buf, format( "1..%d", #self ) )
    for i,tst in ipairs( self ) do
      t_insert( buf, format( "\n%s %d %s",
        tst.pass and 'ok' or 'not ok', i, tostring(tst) ) )
      if "failed" == tst.status then
        t_insert( buf, "\n" ..addTapDiagnostic( tst ) )
      end
    end
    return t_concat( buf, "" )
  end
}


return setmetatable( {
  hasPassed  = function( ste, ctx ) return ctx:getMetrics( ste ).success end,
  getMetrics = function( ste, ctx ) return ctx:getMetrics( ste ) end,
}, {
  __call   = function( self, tbl )
    if tbl and 'table' == type( tbl ) then
      local suite = makeTst( { } )
      if tbl.beforeAll then tbl:beforeAll( ) end
      for name, case in pairs( tbl ) do
        if type(case)=='function' and name~='beforeAll' and name~='afterAll' and name~='beforeEach' and name~='afterEach' then
          local start = Time()
          if tbl.beforeEach then tbl:beforeEach( ) end

          local ok, result = Test( case, tbl )
          _mt.__newindex(suite, name, result )

          if tbl.afterEach then tbl:afterEach( ) end
          printTst( name, result, Time()-start )
        end
      end
      if tbl.afterAll then tbl:afterAll( ) end
      return suite
    else
      return makeTst( { } )
    end
  end
} )
