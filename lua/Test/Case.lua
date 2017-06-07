-- \file      lua/Test/Case.lua
-- \brief     lua-t unit testing framework (t.Test.Case)
--            Test Case implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local t_concat    , t_insert    , format       , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, string.format, getmetatable, setmetatable, pairs, assert, type
local T,Time,Context = require"t", require"t.Time", require"t.Test.Context"

local _mt
local T_TST_CSE_SKIPINDICATOR = "<test_case_skip_indicator>:" -- must have trailing ":"
local STG_BFE = 1
local STG_EXC = 2
local STG_AFE = 3
local STG_DNE = 4

-- ---------------------------- general helpers  --------------------
-- create a Test.Case instance from a table
local makeCse = function( prx )
end

local setFunctionSource = function( f, b )
	local dbg, c = debug.getinfo( f, "S" ), 1
	for l in io.lines( dbg.short_src ) do
		if c>=dbg.linedefined and c<=dbg.lastlinedefined then
			t_insert( b, format( "\n    %d: %s", c, l ) )
		end
		c = c+1
	end
end

local addTapDiagnostic = function( tbl )
	local f, b = { "description", "testtype", "executionTime", "pass", "skip", "todo",
	               "message", "location", "traceback" }, { "  ---" }
	for i = 1, #f do
		if nil ~= tbl[ f[i] ] then
			if     "boolean" == T.type( tbl[ f[i] ]) then
				t_insert( b, format( "\n  %s: %s", f[i], tbl[ f[i] ] and "True" or "False" ) )
			elseif "T.Time"  == T.type( tbl[ f[i] ]) then
				t_insert( b, format( "\n  %s: %s", f[i], tbl[ f[i] ] ) )
			else
				t_insert( b, format( "\n  %s: %s", f[i], tbl[ f[i] ]:gsub( "\n","\n  ") ) )
			end
		end
	end
	t_insert( b, "\n  source: " )
	setFunctionSource( tbl.func, b )
	t_insert( b, "\n  ..." )
	return t_concat( b, "" )
end

local traceback = function( tbk )
	local loc, msg = tbk:match( '^([^:]*:%d+): (.*)' )
	--print('\n',tbk,'\n',loc,'\n',msg)
	if msg then
		local skipm = msg:match( T_TST_CSE_SKIPINDICATOR .. "(.*)$" )
		if skipm then
			return { pass=true,  skip=skipm }
		else
			return { pass=false, message=msg, location=loc, traceback=debug.traceback( ):gsub("\n\t+","\n  ") }
		end
	else
		return { message=tbk, traceback=debug.traceback( ):gsub("\n\t+","\n  ") }
	end
end

local getCaseFromStack = function( )
	local t, i = debug.getinfo( 0, "fu" ), 0
	while t do
		for k=1,t.nups do
			local nme, upv = debug.getupvalue( t.func, k )
			if _mt == getmetatable( upv ) then return upv end
		end
		i = i + 1
		t = debug.getinfo( i, "fu" )
	end
	return 0
end

local getDescription = function( self )
	if     self.skip then return self.description .. " # SKIP: " .. self.skip
	elseif self.todo then return self.description .. " # TODO: " .. self.todo
	else                  return self.description end
end

local reset      = function( s )
	s.pass,s.skip,s.todo,s.message,s.location,s.executionTime = nil,nil,nil,nil,nil,nil
end

local syncRunner = function( cse, ste, dne )
	return function( )
		if ste.beforeEach then ste.beforeEach( ste ) end
		cse.executionTime = Time( )
		local s,m = xpcall( cse.func, traceback, ste )
		cse.executionTime:since( )
		if not s then
			for k,v in pairs( m ) do cse[k] = v end
		else
			cse.pass = true
		end
		if ste.afterEach then ste.afterEach( ste ) end
		if dne and "function" == type( dne ) then dne( ste, cse ) end
	end
end


local callbackRunner
local makeClosure = function( stg, cse, ste, dne )
	return function( )
		callbackRunner( stg, cse, ste, dne )
	end
end

callbackRunner = function( stg, cse, ste, dne )
	if STG_BFE == stg then
		if ste.beforeEach_cb then
			ste:beforeEach_cb( makeClosure( STG_EXC, cse, ste, dne ) )
		else
			stg = STG_EXC
		end
	end
	if STG_EXC == stg then
		cse.executionTime = Time()
		local s,m = xpcall( cse.func, traceback, ste, makeClosure( STG_AFE, cse, ste, dne ) )
	end
	if STG_AFE == stg then
		cse.executionTime:since( )
		if nil == cse.pass then cse.pass=true end
		if ste.afterEach_cb then
			ste:afterEach_cb( makeClosure( STG_DNE, cse, ste, dne ) )
		else
			stg = STG_DNE
		end
	end
	if STG_DNE == stg and dne and "function" == type( dne ) then dne( ste, cse ) end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Test.Case",
	__tostring = function( self )
		return getDescription( self ) .. "\n" .. addTapDiagnostic( self )
	end,
	__call     = function( self, ste, dne )
		if "callback" == self.testtype then
			return makeClosure( STG_BFE, self, ste, dne )()
		else
			return syncRunner( self, ste, dne )( )
		end
	end,
	addTapDiagnostic = addTapDiagnostic,
	getDescription   = getDescription,
	reset            = reset,
}
_mt.__index    = _mt

return setmetatable( {
	skip     = function( why ) return error( T_TST_CSE_SKIPINDICATOR .. why ) end,
	todo     = function( why ) getCaseFromStack().todo        = why           end,
	describe = function( dsc ) getCaseFromStack().description = dsc           end,
}, {
	__call   = function( self, nme, typ, fnc )
		assert( 'string'   == type( nme ), "`Test.Case` name must be a string" )
		assert( 'string'   == type( typ ), "`Test.Case` type must be a string" )
		assert( 'standard' == typ or 'callback' == typ or 'coroutine' == typ,
			"`Test.Case` type must be 'standard, 'callback', or 'coroutine'" )
		assert( 'function' == type( fnc ), "`Test.Case` definition must be a function" )
		local cse = { testtype = typ, description = nme, func = fnc }
		return setmetatable( cse, _mt )
	end
} )
