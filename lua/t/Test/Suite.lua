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


-- ---------------------------- output formatters  --------------------
--                  green    red      yellow   blue
local colors    = { PASS=32, FAIL=31, SKIP=33, TODO=36 }
local verbosity = { "QUIET", "COMPACT", "INFO", "TAP", "TAPVERBOSE" }
local v_FULL    = { "executionTime", "runTime", "status", "message", "location", "info", "traceback", "testSource", "failedSource" }
local v_FAIL    = { "executionTime", "status", "message", "location", "failedSource" }
local v_INFO    = { "info" }

local colorize = function( value, style )
  local colorcode = style and colors[ style ] or colors[ value ]
  assert( colorcode, "The style for colorization must be in {PASS, FAIL, SKIP, TODO}")
  return ('[%dm%s[0m'):format( colorcode, value )
end

local yamlFields = function( tst, fields, wrapper )
  local yaml = wrapper and { "\n  ---"} or { }
  for _, fld in ipairs( fields ) do
    if tst[ fld ] then
      if "info" == fld then
        t_insert( yaml, "\n  Info:" )
        for _,n in ipairs( tst.info ) do
          t_insert( yaml, ("\n    - %s"):format( n ) )
        end
      elseif fld:match('.*Source') then
        t_insert( yaml, ("\n  %s: "):format( fld ) )
        local line_numbers = t_keys( tst[fld] )
        t_sort( line_numbers )
        for _,nr in ipairs( line_numbers ) do
          t_insert( yaml, ("\n    %d: %s"):format( nr, tst[fld][nr] ) )
        end
      else
        t_insert( yaml, ( "\n  %s: %s" ):format( fld, tostring(tst[ fld ]):gsub( "\n", "\n  " ) ) )
      end
    end
  end
  return wrapper and t_concat( yaml, "") .. "\n  ...\n" or t_concat( yaml, "" )
end

local composeDescription = function( tst, color )
  return ('SKIP'==tst.status or "TODO"==tst.status)
      and ('%s # %s: %s'):format(
        tst.description,
        color  and colorize(tst.status )              or tst.status,
        color  and colorize(tst.message, tst.status)  or tst.message
      )
      or  tst.description
end

local yamlHeader = function( suite )
  return ("1..%d\n"):format( #suite )
end

local compactLine = function( nme, tst, color )
  return ('%s [%dms] [%dms] [%s] %s'):format(
    color and colorize( tst.status ) or tst.status,
    tst.executionTime,
    tst.runTime,
    nme, composeDescription( tst, color )
  )
end

local yamlLine = function( tst, idx, color )
  if tst.pass then
    return ( "%s %d - %s"):format( color and colorize('ok', 'PASS') or 'ok', idx, composeDescription(tst, color) )
  else
    return ( "%s %d - %s"):format( color and colorize('not ok', 'FAIL') or 'not ok', idx, composeDescription(tst, color) )
  end
end

local formatTest = function( nme, idx, tst, verbosity, color )
  if not verbosity or "QUIET" == verbosity then
    return
  elseif "COMPACTINFO" == verbosity then
    return compactLine( nme, tst, color ) .. yamlFields( tst, v_INFO )
  elseif "TAP" == verbosity then
    return yamlLine( tst, idx, color ) .. (tst.pass and "" or yamlFields( tst, v_FAIL, true ))
  elseif "TAPVERBOSE" == verbosity then
    return yamlLine( tst, idx, color ) .. (yamlFields( tst, v_FULL, true ))
  else -- default to "COMPACT"
    return compactLine( nme, tst, color ) .. (tst.pass and "" or yamlFields( tst, v_FAIL ))
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
      --if "FAIL" == tst.status then
      --  t_insert( buf, "\n" .. tapOutput( tst ) )
      --end
    end
    return t_concat( buf, "" )
  end
}

return setmetatable(
  {
    _VERSION     = _mt.__name .. ' 0.1.0',
    _DESCRIPTION = 'lua-t unit-testing suite test harness.',
    _URL         = 'https://gitlab.com/tobbik/lua-t',
    _LICENSE     = 'MIT',
    colorize     = colorize,
    coloredTap   = coloredTap,
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
        local ok, result = Test(tbl[ name ], tbl)
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
