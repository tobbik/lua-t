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

local Table, T   = require( "t.Table" ), require( "t" )
local t_concat    , t_insert,     t_remove =
      table.concat, table.insert, table.remove
local getmetatable, setmetatable, pairs, assert, next, type =
      getmetatable, setmetatable, pairs, assert, next, type
local t_keys    , t_values    , t_clone    , t_asstring     , t_equals     , prxTblIdx             =
      Table.keys, Table.values, Table.clone, Table.asstring , Table.equals , Table.proxyTableIndex
local t_type, t_assert    =
      T.type, T.assert

local _mt

-- Test if ipairs() is compat or not
-- compat returns more than 2 variables of __ipairs
local t = setmetatable( {},{__ipairs=function(tbl) return function() return nil,nil,true end,tbl,0 end})

local a,b,c = ipairs(t)
local _,_,_isCompat = a(b,c)


-- ---------------------------- general helpers  --------------------
-- assert Oht type and return the proxy table
local getPrx     = function( self )
	t_assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, t_type( self ) )
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
			assert( 0<idx and idx<#tbl,  ("Index[%d] out of range of #OrderedHashTable[%d]"):format(idx,#tbl) )
			tbl[ tbl[idx] ] = val
		elseif nil == tbl[ key ] then
			t_insert( tbl, key )
			tbl[ key ] = val
		else
			tbl[ key ] = val
		end
	end
end

local iters = function( tbl, int )
	if int and _isCompat then
		return function( t, idx )
				if idx < #tbl then
					idx = idx + 1
					local key = tbl[ idx ]
					return idx, tbl[ key ], key
				end
			end,
			tbl, 0
	elseif int then
		return function( t, idx )
				if idx < #tbl then
					idx = idx + 1
					local key = tbl[ idx ]
					return idx, tbl[ key ]
				end
			end,
			tbl, 0
	else
		local k_idx = 0
		return function( t, last_key )
				if k_idx < #tbl then
					k_idx = k_idx + 1
					local key = tbl[ k_idx ]
					return key, tbl[ key ], k_idx
				end
			end,
			tbl, nil
	end
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
	__name     = "T.OrderedHashTable",
	__len      = function( self )           return #(getPrx( self )) end,
	__index    = function( self, key )      return getElement( getPrx( self ), key ) end,
	__newindex = function( self, key, val ) setElement( getPrx( self ), key, val )   end,
	__pairs    = function( self )           return iters( getPrx( self ), false )    end,
	__ipairs   = function( self )           return iters( getPrx( self ), true )     end,
-- comparing operations
	__eq       = function( self, othr )     return t_equals( getPrx(self), getPrx( othr ) ) end,
}
-- allow luaL_getmetatable( L )
debug.getregistry( )[ 't.OrderedHashTable' ] = _mt


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
	_isCompat  = _isCompat
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

