-- \file      lua/Test/Context.lua
-- \brief     lua-t unit testing execution context
--            For the duration of the execution of a T.Test suite it will
--            keep important parameters and runtime information
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local write   , format       , getmetatable, setmetatable, pairs, type =
      io.write, string.format, getmetatable, setmetatable, pairs, type
local t_clone, Time = require"t.Table".clone, require"t.Time"

local _mt = {
	__name   = 't.Test.Context'
}


local beforeEach = function( self, case, suite )
	write( format( "%-".. (self.name_width) .."s : ", self.current_name ) )
end

local afterEach  = function( self, case, suite )
	print( case:getDescription( ) )
end

local beforeAll  = function( self, suite )
	-- do nothing
end

local afterAll   = function( self, suite )
	local res = self:getMetrics( suite );
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

return setmetatable( {
}, {
		__call   = function( self, incl, excl, befE, aftE, befA, aftA )
		if ctx and mt.__name == T.type( incl ) then
			return setmetatable( t_clone( incl ), _mt )
		else
			local ctx = setmetatable( {
				  name_width   = 0
				, current_name = nil
				, include      = (incl and "string"   == type( incl )) and incl or ''
				, exclude      = (excl and "string"   == type( excl )) and excl or '^$'
				, beforeEach   = (befE and "function" == type( befE )) and befE or beforeEach
				, afterEach    = (aftE and "function" == type( aftE )) and aftE or afterEach
				, beforeAll    = (befA and "function" == type( befA )) and befA or beforeAll
				, afterAll     = (aftA and "function" == type( aftA )) and aftA or afterAll
				, match        = function( self, name )
					if name:match( self.include ) and not name:match( self.exclude ) then
						return true
					else
						return false
					end
				end
				, getMetrics = function( self, suite )
					local count,pass,skip,todo,time = 0,0,0,0,Time(1)-Time(1)
					local incl, excl = incl or '', excl or '^$'
					for n,cse,i in pairs( suite ) do
						if self:match( n ) then
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
			}, _mt )
			return ctx
		end
	end
} )
