-- vim: ts=2 sw=2 sts=2 et
-- \file      lua/Test.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test Case implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local t_concat    , t_insert    , s_format     , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, string.format, getmetatable, setmetatable, pairs, assert, type
local T,Time,t_merge,pp = require"t", require"t.Time", require't.Table'.merge, require't.Table'.pprint

local T_TST_CSE_SKIPINDICATOR = "<test_case_skip_indicator>:" -- must have trailing ":"
local STG_BFE = 1  -- before
local STG_EXC = 2  -- execute
local STG_AFE = 3  -- after
local STG_DNE = 4  -- done

-- ---------------------------- general helpers  --------------------
local getFunctionSource = function( f )
  local dbg, c, src = debug.getinfo( f, "Sl" ), 1, {}
  --pp({DBG= dbg})
  -- TODO: make sure short_src is a file, not stdin or rubbish
  for l in io.lines( dbg.short_src ) do
    if c >= dbg.linedefined and c <= dbg.lastlinedefined then
      t_insert( src, s_format( "    %d: %s", c, l ) )
    end
    c = c+1
  end
  return t_concat( src, "\n" )
end

local getDescription    = function( x )
  if     x.skip then return x.description .. " # SKIP: " .. x.skip
  elseif x.todo then return x.description .. " # TODO: " .. x.todo
  else               return x.description end
end

local reset      = function( s )
  s.pass,s.skip,s.todo,s.message,s.location,s.executionTime = nil,nil,nil,nil,nil,nil
end

local traceback = function( tbk )
  local loc, msg = tbk:match( '^([^:]*:%d+): (.*)' )  -- "foo.lua:22: What went wrong"
  -- level 2 is where it failed, level 1 is this traceback function
  local tb = debug.traceback( nil, 2 ):gsub( "\n\t+", "\n  " ):gsub( "stack traceback:\n", "" )
  if msg then
    local skipm = msg:match( T_TST_CSE_SKIPINDICATOR .. "(.*)$" )
    if skipm then
      return { pass=true,  skip=true,  message=skipm }
    else
      return { pass=false, skip=false, message=msg, location=loc, traceback=tb }
    end
  else
    return { pass=false, message=tbk, traceback=tb }
  end
end

local execute = function( _, description, func, async )

end

return setmetatable(
  {
    _VERSION     = 't.Tst 0.1.0',
    _DESCRIPTION = 'lua-t unit-testing.',
    _URL         = 'https://gitlab.com/tobbik/lua-t',
    _LICENSE     = 'MIT',
    skip         = function( why, ... ) return error( T_TST_CSE_SKIPINDICATOR .. s_format( why, ... ) ) end,
    getSource    = getFunctionSource,
  },
  {
    __call = function( _, description, func, async )
      return function( ... )
        local rinse = function()
        end
        local start   = Time( )
        local ok, err = xpcall( func, traceback, ... )
        local result  = { description=description, executionTime=(Time()-start).ms, pass=ok, skip=false }
        if not ok then
          result = t_merge( result, err, true )
          result.source = getFunctionSource( func )
        end
        return result.pass, result
      end
    end
  }
)
