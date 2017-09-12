-- \file      lua/Set.lua
-- \brief     Set implementation as Lua table wrapper
-- \detail    The object itself is a Lua table wrapping a proxy table, which
--            allows to use __index and __newindex to control table access.
--            Internally the set is a table with the following structure:
--               elem1   = true,
--               elem2   = true,
--               elem3   = true,
--               elem4   = true,
--               ...
--            Adding an element to the Set is done via Table syntax:
--               set[newElement] = something
--            `something` will be ALWAYS be replace by  true via __newindex()
--            Deleting an element from the Set is also done via table syntax:
--               set[anyElement] = nil
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local T, Table  = require( "t" ), require( "t.Table" )
local prxTblIdx = T.proxyTableIndex
local t_concat     , getmetatable, setmetatable, pairs, assert, next, type =
      table.concat , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local getPrx     = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ prxTblIdx ]
end

-- create a Set instance from a table
local makeSet = function( prx )
	return setmetatable( { [ prxTblIdx ] = prx }, _mt )
end

-- ---------------------- meta-methods operator behaviour definitions
local opBor  = function( self, othr )        -- union
	return makeSet( t_merge( getPrx( self ), getPrx( othr ), true ) )
end

local opBand  = function( self, othr )       -- intersection
	return makeSet( t_merge( getPrx( self ), getPrx( othr ), false ) )
end

local opBxor  = function( self, othr )       -- symmetric difference
	return makeSet( t_complement( getPrx( self ), getPrx( othr ), true ) )
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Set",
	__len      = t_count,
	__pairs    = function( self )           return next, getPrx( self ), nil   end,
	__index    = function( self, key )      return getPrx( self )[ key ]       end,
	__newindex = function( self, key, val )        getPrx( self )[ key ] = val end,
	-- Set operations
	__bor   = opBor,  __add = opBor,   -- union        -> '+' and '|' operator
	__band  = opBand, __mul = opBand,  -- intersection -> '*' and '&' operator
	__bxor  = opBxor, __pow = opBxor,  -- symm diff    -> '^' and '~' operator
	__sub   = function( self, othr )   -- complement   -> '-' operator
		return makeSet( t_complement( getPrx( self ), getPrx( othr ), false ) )
	end,
	-- Set comparing operations
	__eq  = function( self, othr )   -- is equal
		return  t_contains( self, othr, false ) and     t_contains( othr, self, false )
	end,
	__lt  = function( self, othr )   -- is subset AND NOT equal
		return  t_contains( othr, self, false ) and not t_contains( self, othr, false )
	end,
	__le  = function( self, othr )   -- is subset OR equal
		return  t_contains( othr, self, false )
	end,
	__mod = function( self, othr )   -- is disjunct
		return  t_contains( self, othr, true )
	end,
}

return setmetatable( {
	values   = function( set ) return t_keys( getPrx( set ) ) end,
	toString = function( set ) return t_asstring( getPrx( set ), 'keys' ) end
}, {
	__call   = function( self, tbl, useKeys )
		local prx  = { }
		if tbl and getmetatable( tbl ) == _mt then
			useKeys = true
		elseif tbl and type( tbl ) == "table" then
			useKeys = useKeys or false
		end
		if tbl and useKeys then
			for k,_ in pairs( tbl ) do prx[ k ] = true end
		elseif tbl then
			for _,v in pairs( tbl ) do prx[ v ] = true end
		end
		return makeSet( prx )
	end
} )

