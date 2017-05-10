-- \file      lua/Test.lua
-- \brief     lua-t unit testing framework (t.Test)
--            Test suite implemented as Lua Table
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local          Case ,                Context ,           Time,          T = 
require't.Test.Case', require't.Test.Context', require't.Time', require't'

local prxTblIdx,                   Table,            Oht  =
      T.proxyTableIndex, require"t.Table", require"t.OrderedHashTable"
local t_concat    , t_insert    , format       , getmetatable, setmetatable, pairs, assert, type =
      table.concat, table.insert, string.format, getmetatable, setmetatable, pairs, assert, type
local t_clone     , o_setElement  , o_getElement  , o_iters =
      Table.clone , Oht.setElement, Oht.getElement, Oht.iters

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

local joiner = function( ctx )
	return function( ste, cse )
		local ran, cnt = 0, 0
		ctx:afterEach( cse, ste )
		for k,v,i in pairs( ste ) do
			if ctx:match( k ) then
				cnt = cnt + 1
				ran = ran + (nil==v.pass and 0 or 1 )
			end
		end
		ctx.current_name = nil
		if ran == cnt then
			if ste.afterAll and "function" == type( ste.afterAll ) then
				ste.afterAll( ste, function( ) ctx:afterAll( ste ) end )
			else
				ctx:afterAll( ste )
			end
		end
	end
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Test",
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
	__call     = function( self, inc_pat, ... )
		local prx = getPrx( self )
		local ctx = 't.Test.Context' == T.type( inc_pat ) and inc_pat or Context( inc_pat, ... )
		-- reset and prepare
		for k,v,i in pairs( self ) do
			if ctx:match( k ) then
				v:reset( )
				ctx.name_width = #k>ctx.name_width and #k or ctx.name_width
			end
		end
		--execute
		if 0 ~= ctx.name_width then 
			local runner = function( )
				for k,v,i in pairs( self ) do
					if ctx:match( k ) then
						ctx.current_name = k
						ctx:beforeEach( v, self )
						v( self, joiner( ctx ) )
					end
				end
			end
			if self.beforeAll and "function" == type( self.beforeAll ) then
				local r,err = pcall( self.beforeAll, self, runner )
				if not r then print(err);end
			else
				runner( )
			end
			return Context.getMetrics( self, ctx.include, ctx.exclude ).success
		else
			return true
		end
	end,
}


return setmetatable( {
	hasPassed  = function( ste, incl, excl ) return Context.getMetrics( ste, incl, excl ).success end,
	getMetrics = function( ste, incl, excl ) return Context.getMetrics( ste, incl, excl ) end,
	Case       = Case,
	Context    = Context
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
