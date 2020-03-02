local t_insert,t_concat                = table.insert,table.concat
local s_fmt,s_rep                      = string.format,string.rep
local pairs,tostring,assert,type,print = pairs,tostring,assert,type,print

local t_map        = nil
t_map              = function( tbl, func )
	local ret = { }
	for k,v in pairs( tbl ) do  ret[ k ] = func( v, k )  end
	return ret
end

local t_foreach    = nil
t_foreach          = function( tbl, func )
	for k,v in pairs( tbl ) do  tbl[ k ] = func( v, k )  end
	return tbl
end

-- create union or intersection merely based on presence of keys
-- if values for keys differ in t1 vs t2, values from t2 will be used in the
-- result
local t_merge      = function( t1, t2, union )
	local ret = { }
	for k,v in pairs( t2 ) do
		if union or t1[k] then ret[k] = v end
	end
	if union then
		for k,v in pairs( t1 ) do
			ret[ k ] = nil == ret[ k ] and v or ret[ k ]
		end
	end
	return ret
end

local t_complement = function( t1, t2, sym )
	local ret = { }
	for k,v in pairs( t1 ) do
		if nil == t2[ k ] then ret[ k ] = v end
	end
	if sym then
		for k,v in pairs( t2 ) do
			if nil == t1[ k ] then ret[ k ] = v end
		end
	end
	return ret
end

-- only true for absolute cases: completely disjunct or completely contained
local t_contains   = function( t1, t2, disjunct )
	if not disjunct then disjunct = false else disjunct = true end  -- nil->false
	for k,v in pairs( t2 ) do
		if (nil == t1[ k ]) ~= disjunct then return false end
	end
	return true
end

-- This is an untested stub ... may not even be needed
local t_intersects = function( t1, t2 )
	for k,v in pairs( t2 ) do
		if (nil ~= t1[ k ]) then return true end
	end
	return false
end

local t_find       = function( tbl, val, idx )
	assert( type( tbl ) == "table", "Expected `Table`" )
	if idx then -- index forces to search numeric indizes only
		for i=idx,#tbl do      if tbl[i]==val then return i end   end
	else
		for k,v in pairs( tbl ) do  if v==val then return k end   end
	end
	return nil
end

local t_keys     = function( tbl )
	assert( type( tbl ) == "table", "Expected `Table`" )
	local ret = { }
	for k,v in pairs( tbl ) do    t_insert( ret, k )    end
	return ret
end

local t_values   = function( tbl )
	assert( type( tbl ) == "table", "Expected `Table`" )
	local ret = { }
	for k,v in pairs( tbl ) do    t_insert( ret, v )    end
	return ret
end

local t_count     = function( tbl )
	assert( type( tbl ) == "table", "Expected `Table`" )
	local cnt = 0
	for k,v in pairs( tbl ) do    cnt=cnt+1    end
	return cnt
end

local t_clone     = nil    -- recursive
t_clone           = function( tbl, deep )
	local ret = { }
	for k,v in pairs( tbl ) do
		if deep and 'table' == type( v ) then
			ret[ k ] = t_clone( v )
		else
			ret[ k ] = v
		end
	end
	return ret
end

local t_asstring   = function( tbl, t )
	if 'keys' == t then
		return '{' .. t_concat( t_foreach( t_keys( tbl ),   tostring ), ',' ) .. '}'
	elseif 'values' == t then
		return '{' .. t_concat( t_foreach( t_values( tbl ), tostring ), ',' ) .. '}'
	else
		return '{' .. t_concat( t_foreach( t_values( tbl ), tostring ), ',' ) .. '}'
	end
end

local t_isempty  = function( tbl )
	for k,v in pairs( tbl ) do return false end
	for i = 1, #tbl         do return false end
	return true
end

local t_pprint = nil    -- recursive
t_pprint = function( tbl, idnt )
	idnt = idnt or 0
	for k, v in pairs( tbl ) do
		local s_fmt = s_rep( "  ", idnt ) .. k .. ": "
		if "table" == type( v ) then
			print( s_fmt )
			t_pprint( v, idnt+1 )
		elseif 'boolean' == type( v ) then
			print( s_fmt .. tostring( v ) )
		else
			print( s_fmt .. tostring( v ) )
		end
	end
end


return {
	  map          = t_map
	, foreach      = t_foreach
	, merge        = t_merge
	, complement   = t_complement
	, contains     = t_contains
	, intersects   = t_intersects
	, find         = t_find
	, keys         = t_keys
	, values       = t_values
	, count        = t_count
	, length       = t_count
	, clone        = t_clone
	, asstring     = t_asstring
	, isempty      = t_isempty
	, pprint       = t_pprint
}
