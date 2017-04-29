-- \file      lua/Test.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test suite implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local Case = require't.Test.Case'

local prxTblIdx,                      Table,            Oht  =
      require( "t" ).proxyTableIndex, require"t.Table", require"t.OrderedHashTable"
local t_concat    , t_insert    , format       , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, string.format, getmetatable, setmetatable, pairs, assert, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_clone =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.clone
local o_setElement  , o_getElement  , o_iters =
      Oht.setElement, Oht.getElement, Oht.iters
local Time = require't.Time'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Test type and return the proxy table
local getPrx = function( tst )
	assert( _mt == getmetatable( tst ), "Expected `Test`" )
	return tst[ prxTblIdx ]
end
-- create a Test instance from a table
local makeTst = function( prx )
	return setmetatable( { [ prxTblIdx ] = prx }, _mt )
end

local getMetrics = function( tst, pattern, invert )
	local count,pass,skip,todo,time = 0,0,0,0,Time(1)-Time(1)
	local pttrn = pattern or ''
	for n,cse,i in pairs( tst ) do
		if (not invert and n:match( pttrn )) or (invert and not n:match( pttrn )) then
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

local finalizer = function( tst, pattern, invert )
	return function( )
		local res = getMetrics( tst, pattern, invert );
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
end

local callEnvelope = function( fnc, tst, run )
	if fnc and 'function'==type( fnc ) then
		local r,err = pcall( fnc, tst, run )
		if not r then print(err);end
		--if not r then print(err);error( "Test failed" ) end
	else
		run( )
	end
end

local joiner = function( pattern, invert )
	local pttrn = pattern or ''
	return function( ste, cse )
		local ran, cnt = 0, 0
		print( "Executed test: " .. cse:getDescription( ) )
		for k,v,i in pairs( ste ) do
			if (not invert and k:match( pttrn )) or (invert and not k:match( pttrn )) then
				cnt = cnt + 1
				ran = ran + (nil==v.pass and 0 or 1 )
			end
		end
		if ran == cnt then
			callEnvelope( ste.afterAll, ste, finalizer( ste, pattern, invert ) )
		end
	end
end

local caseRunner = function( ste, pattern, invert )
	local pttrn = pattern or ''
	return function()
		for k,v,i in pairs( ste ) do
			if (not invert and k:match( pttrn )) or (invert and not k:match( pttrn )) then
				v( ste, joiner( pattern, invert ) )
			end
		end
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "T.Test",
	__len      = function( self )      return #getPrx( self ) end,
	__pairs    = function( self )      return o_iters( getPrx( self ), false )         end,
	__ipairs   = function( self )      return o_iters( getPrx( self ), true )          end,
	__index    = function( self, key ) return o_getElement( getPrx( self ), key )      end,
	__newindex = function( self, key, val )
		local prx = getPrx( self )
		if "number"==type(key) then assert( key%1==0, "Can't set or overwrite numeric indices" ) end
		if "string"==type(key) and key:match( "^test_" ) then
			o_setElement( prx, key, Case( key, val) )
		else
			prx[ key ] = val
		end
	end,
	__tostring = function( self )
		local buf = { }
		t_insert( buf, format( "1..%d", #self ) )
		for i,cse in ipairs( self ) do
			t_insert( buf, format( "%s %d - %s",
				nil == cse.pass and 'not run' or cse.pass and 'ok' or 'not ok', i, cse:getDescription() ) )
			if false == cse.pass and not cse.todo then
				t_insert( buf, cse:addTapDiagnostic() )
			end
		end
		return t_concat( buf, "\n" )
	end,
	__call     = function( self, pattern, invert )
		local prx = getPrx( self )
		for i=1,#self do self[ i ]:prune( ) end
		callEnvelope( self.beforeAll, self, caseRunner( self, pattern, invert ) )
		return getMetrics( self, pattern, invert ).success
	end,
}

Case.done = done
return setmetatable( {
	hasPassed  = function( ste, p, inv ) return getMetrics( ste, p, inv ).success end,
	getMetrics = function( ste, p, inv ) return getMetrics( ste, p, inv ) end,
	Case       = Case,
}, {
	__call   = function( self, tbl )
		local prx;
		if tbl and 'table' == type( tbl ) and getmetatable( tbl ) == _mt then
			return makeTst( t_clone( getPrx( tbl ) ) )
		elseif tbl and 'table' == type( tbl ) then
			local tst = makeTst( { } )
			for k,v in pairs( tbl ) do
				_mt.__newindex( tst, k, v )
			end
			return tst
		end
		return makeTst( { } )
	end
} )
