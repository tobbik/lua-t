-- \file      lua/OrderedHashTable.lua
-- \brief     ordered hash table implementation
-- \detail    Elements can be accessed and updated by their name OR their index.
--            Basically an ordered hashmap.  It is implemented as intelligent
--            mapper around a Lua table.  The basic design handles the values as
--            following:
--               1   = 'a',
--               2   = 'b',
--               3   = 'c',
--               4   = 'd',
--               "a" = "value 1",
--               "b" = "value 2",
--               "c" = "value 3",
--               "d" = "value 4"
--            The object itself is a Lua table wrapping a proxy table, which
--            allows to use __index and __newindex to control table access.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local T, Table   = require( "t" ), require( "t.Table" )
local t_concat    , t_insert,     t_remove =
      table.concat, table.insert, table.remove
local getmetatable, setmetatable, pairs, assert, next, type =
      getmetatable, setmetatable, pairs, assert, next, type
local t_keys    , t_values    , t_clone    , t_asstring     , equals   , prxTblIdx =
      Table.keys, Table.values, Table.clone, Table.asstring , T.equals , T.proxyTableIndex

local _mt = { }

-- ---------------------------- general helpers  --------------------
-- assert Oht type and return the proxy table
local getPrx     = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ prxTblIdx ]
end

local getValues  = function( tbl )
	local ret = { }
	for i=1,#tbl do t_insert( ret, tbl[ tbl[i] ] ) end
	return ret
end

local getKeys    = function( tbl )
	local ret = { }
	for i=1,#tbl do t_insert( ret, tbl[i] ) end
	return ret
end

local getTable   = function( tbl )
	local ret = { }
	for i=1,#tbl do ret[ tbl[i] ] = tbl[ tbl[i] ] end
	return ret
end

--TODO: consider a 't.*' global type system allowing this to be more generic
local cloneTable
cloneTable       = function( tbl )
	local ret = { }
	for k,v in pairs( tbl ) do
		if _mt == getmetatable( v ) then
			ret[ k ] = setmetatable( { [ prxTblIdx ] = cloneTable( getPrx( v ) ) }, _mt )
		else
			ret[ k ] = v
		end
	end
	return ret
end

local getIndex   = function( tbl, key )
	for i=1,#tbl do if key == tbl[i] then return i end end
	return nil
end

local insertElem = function( tbl, idx, key, val )
	assert( 0<idx and idx<#tbl,  "Index out of range of #OrderedHashTable" )
	assert( nil==tbl[key],       "Can't insert value with existing key" )
	tbl[ key ] = val
	t_insert( tbl, idx, key )
end

local getElement = function( tbl, key )
	local idx = 'number'==type(key) and 0==key%1 and key or false
	if idx then return tbl[ tbl[ idx ] ]
	else        return tbl[ key ]      end
end

local setElement = function( tbl, key, val )
	local idx = 'number'==type(key) and 0==key%1 and key or false
	if nil==val then
		if idx then key = tbl[idx] else idx = getIndex( tbl, key ) end
		if idx then tbl[ key ] = nil; table.remove( tbl, idx ) else return end
	else
		if idx then  -- can only replace values
			assert( 0<idx and idx<#tbl,  "Index out of range of #OrderedHashTable" )
			tbl[ tbl[idx] ] = val
		elseif nil == tbl[ key ] then
			t_insert( tbl, key )
			tbl[ key ] = val
		else
			tbl[ key ] = val
		end
	end
end

local iters      = function( tbl, int )
	local idx, init  = 0, nil
	if int then init = 0 end
	return function( key, t )
		idx = idx + 1
		local key = tbl[ idx ]
		if nil ~= key and int     then return idx, tbl[ key ], key end
		if nil ~= key and not int then return key, tbl[ key ], idx end end,
		tbl, init
end

local readArgs   = function( ... )
	local ret, args = { }, { ... }
	for i=1,#args do
		local k,v = next( args[i] )
		setElement( ret, k, v )
	end
	return ret
end


-- ---------------------------- Instance metatable --------------------
-- essentials
_mt = {       -- local _mt at top of file
	__name     = "t.OrderedHashTable",
	__len      = function( self )           return #(getPrx( self )) end,
	__index    = function( self, key )      return getElement( getPrx( self ), key ) end,
	__newindex = function( self, key, val ) setElement( getPrx( self ), key, val) end,
	__pairs    = function( self )           return iters( getPrx( self ) ) end,
	__ipairs   = function( self )           return iters( getPrx( self ), true )   end,
-- comparing operations
	__eq       = function( self, othr )     return equals( getPrx(self), getPrx( othr ) ) end,
}


return setmetatable( {
	index    = function( oht, key ) return getIndex( getPrx( oht), key ) end,
	key      = function( oht, idx ) return getPrx( oht )[ idx ] end,
	values   = function( oht )      return getValues( getPrx( oht) ) end,
	keys     = function( oht )      return getKeys( getPrx( oht) ) end,
	table    = function( oht )      return getTable( getPrx( oht) ) end,
	toString = function( oht )      return t_asstring( getPrx( oht ), 'keys' ) end,
	insert   = function( oht, idx, key, val ) insertElem( getPrx( oht ), idx, key, val ) end,
	concat   = function( oht, sep, i, j )
		return t_concat( getValues( getPrx( oht )) , sep, i, j )
	end,
	-- allow other to use Oht style semantics
	getElement = getElement,
	setElement = setElement,
	iters      = iters,
}, {
	__call   = function( self, tbl, ... )
		local prx
		if tbl and _mt == getmetatable( tbl ) then
			prx = cloneTable( getPrx( tbl ) )
		elseif tbl and type( tbl ) == "table" then
			-- read each argument as table of key value pairs
			prx = readArgs( tbl, ... )
		else
			prx = { }
		end
		return setmetatable( { [ prxTblIdx ] = prx }, _mt )
	end
} )

