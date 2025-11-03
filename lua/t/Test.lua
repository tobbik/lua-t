-- vim: ts=2 sw=2 sts=2 et
-- \file      lua/Test.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test Case executes a test function and returns a table of results
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local t_concat    , t_insert    , getmetatable, setmetatable, pairs =
      table.concat, table.insert, getmetatable, setmetatable, pairs
local Loop = require"t.Loop"

local T_TST_CSE_SKIPINDICATOR = "<t.test_skip_indicator>:" -- must have trailing ":"
local _mt

-- ---------------------------- general helpers  --------------------
-- returns a YAML conmpatb
local getFunctionSource = function( dbg )
  dbg = 'function'==type( dbg ) and debug.getinfo( dbg, "Sl" ) or dbg
  if "C" == dbg.what then return { [ 0 ] = "C (Compiled code)" } end
  local c, src, loc = 1, {}, dbg.source:sub( 2 )
  -- TODO: make sure short_src is a file, not stdin or rubbish
  for l in io.lines( loc ) do
    if c >= dbg.linedefined and c <= dbg.lastlinedefined then
      src[ c ] = l
    end
    c = c+1
  end
  return src, loc
end

local traceback = function( tbk )
  local loc, msg = tbk:match( '^([^:]*:%d+): (.*)' )  -- "foo.lua:22: What went wrong"
  -- level 2 is where it failed, level 1 is this traceback function itself
  local tb = debug.traceback( nil, 2 ):gsub( "\n\t+", "\n  " ):gsub( "stack traceback:", "" )
  if msg then
    local skipm = msg:match( T_TST_CSE_SKIPINDICATOR .. "(.*)$" )
    if skipm then
      return { pass=true, status="SKIP", message=skipm, location=loc }
    else
      return { pass=false, status="FAIL", message=msg, location=loc, traceback=tb, failedSource=getFunctionSource( debug.getinfo( 3, "Sl" ) ) }
    end
  else
    return { pass=false, status="FAIL", message=tbk, location=loc, traceback=tb, failedSource=getFunctionSource( debug.getinfo( 3, "Sl" ) ) }
  end
end

local findInstanceOnStack = function( )
  local i, level = 0, debug.getinfo( 0, "fu" )
  while level do
    local n, name, val = 1, debug.getlocal(i, 1)
    while name do
      if _mt == getmetatable( val ) then return val end
      n, name, val = n+1, debug.getlocal(i, n+1)
    end
    i, level = i+1, debug.getinfo( i+1, "fu" )
  end
  return 0
end


-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
  -- essentials
  __name     = "t.Test",
  __tostring = function( self )
    return ('SKIP'==self.status or "TODO"==self.status)
      and ('%s # %s: %s'):format( self.description, self.status, self.message )
      or  self.description
  end
}
_mt.__index    = _mt

return setmetatable(
  {
    _VERSION     = _mt.__name .. ' 0.1.0',
    _DESCRIPTION = 'lua-t unit-testing.',
    _URL         = 'https://gitlab.com/tobbik/lua-t',
    _LICENSE     = 'MIT',
    describe     = function( dsc, ... ) findInstanceOnStack( ).description = dsc:format( ... ) end,
    todo         = function( dsc, ... ) findInstanceOnStack( ).todo = dsc:format( ... ) end,
    skip         = function( why, ... ) return error( T_TST_CSE_SKIPINDICATOR .. why:format( ... ) ) end,
    info         = function( inf, ... )
      local instance = findInstanceOnStack( )
      instance.info = instance.info or { }
      t_insert( instance.info, inf:format( ... ) )
    end,
    getSource    = getFunctionSource,
  },
  {
    __call = function( _, test_func, ... )
      local result   = setmetatable(
        { description="Unnamed test"
        , pass=true
        , status="PASS"
        , executionTime=Loop.timemonotonic( )
        , testSource=getFunctionSource( test_func ) },
        _mt
      )
      local ok, tbk = xpcall( test_func, traceback, ... )
      result.executionTime = Loop.timemonotonic( ) - result.executionTime
      if not ok then
        for k,v in pairs( tbk ) do result[ k ] = v end
      end
      if result.todo then
        result.message = result.todo
        result.todo    = true
        result.pass    = true
        result.status  = 'TODO'
      end
      return result.pass, result
    end
  }
)
