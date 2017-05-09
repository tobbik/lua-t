-- \file      lua/Test/Context.lua
-- \brief     lua-t unit testing execution context
--            For the duration of the execution of a T.Test suite it will
--            keep important parameters and runtime information
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local write   , format       , getmetatable, setmetatable, pairs, type =
      io.write, string.format, getmetatable, setmetatable, pairs, type
local t_clone = require"t.Table".clone
local Time  = require"t.Time"

local _mt = {
	__name   = 't.Test.Context'
}


local print_case_start = function( self, name, case )
	write( format( "%-".. (self.name_width) .."s :", name ) )
end

local print_case_end   = function( self, case )
	print( case:getDescription( ) )
end

local getMetrics = function( suite, inc_pat, exc_pat )
	local count,pass,skip,todo,time = 0,0,0,0,Time(1)-Time(1)
	local inc_pat, exc_pat = inc_pat or '', exc_pat or '^$'
	for n,cse,i in pairs( suite ) do
		if n:match( inc_pat ) and not n:match( exc_pat ) then
			count = count + 1
			pass  = pass + (cse.pass and 1 or 0)
			skip  = skip + (cse.skip and 1 or 0)
			todo  = todo + (cse.todo and 1 or 0)
			time  = time +  cse.executionTime
		end
	end
	return {
		success = (count == pass+todo),
		count   = count,
		pass    = pass,
		skip    = skip,
		todo    = todo,
		time    = time
	}
end

local print_report     = function( self, suite )
	local res = getMetrics( suite, self.include, self.exclude );
	print( format( "---------------------------------------------------------\n"..
			  "Handled %d tests in %.3f seconds\n\n"..
			  "Executed         : %d\n"..
			  "Skipped          : %d\n"..
			  "Expected to fail : %d\n"..
			  "Failed           : %d\n"..
			  "status           : %s\n"
		, res.count, res.time:get()/1000.0
		, res.count - res.skip
		, res.skip
		, res.todo
		, res.count - res.pass
		, res.success and "OK" or "FAIL" ) )
end

local match_name = function( self, name )
	print(self, name, self.include, self.exclude )
	if name:match( self.include ) and not name:match( self.exclude ) then
		return true
	else
		return false
	end
end

return setmetatable( {
	getMetrics = getMetrics
}, {
	__call   = function( self, inc_pat, exc_pat, p_case_s, p_case_e, p_report )
		print( "__call" ,self, inc_pat, exc_pat, p_case_s, p_case_e, p_report )
		if ctx and mt.__name == T.type( inc_pat ) then
			return setmetatable( t_clone( inc_pat ), _mt )
		else
			return setmetatable( {
				  name_width    = 0
				, include       = inc_pat  and "string"   == type( inc_pat )  or ''
				, exclude       = exc_pat  and "string"   == type( exc_pat )  or '^$'
				, p_case_before = p_case_s and "function" == type( p_case_s)  or print_case_start
				, p_case_after  = p_case_e and "function" == type( p_case_e)  or print_case_end
				, p_report      = p_report and "function" == type( p_report ) or print_report
				, match         = match_name
			}, _mt )
		end
	end
} )
