local t_insert,t_concat,fmt = table.insert,table.concat,string.format

local Table      = { }
Table.map        = function( tbl, func )
	for k,v in pairs( tbl ) do  tbl[ k ] = func( v )  end
	return tbl
end

Table.merge      = function( t1, t2, union )
	local ret = { }
	for k,v in pairs( t1 ) do
		if union or t2[ k ] then ret[ k ] = v end
	end
	if union then
		for k,v in pairs( t2 ) do
			ret[ k ] = v
		end
	end
	return ret
end

Table.complement = function( t1, t2, sym )
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
Table.contains   = function( t1, t2, disjunct )
	if not disjunct then disjunct = false else disjunct = true end  -- nil->false
	for k,v in pairs( t2 ) do
		if (nil == t1[ k ]) ~= disjunct then return false end
	end
	return true
end

-- This is an untested stub ... may not even be needed
Table.intersects = function( t1, t2 )
	for k,v in pairs( t2 ) do
		if (nil ~= t1[ k ]) then return true end
	end
	return false
end

Table.find       = function( tbl, val, idx )
	assert( type(tbl) == "table", "Expected `Table`" )
	if idx then -- index forces to search numeric indizes only
		for i=idx,#tbl do      if tbl[i]==val then return i end   end
	else
		for k,v in pairs( tbl ) do  if v==val then return k end   end
	end
	return nil
end

Table.keys       = function( tbl )
	assert( type(tbl) == "table", "Expected `Table`" )
	local ret = { }
	for k,v in pairs( tbl ) do    t_insert( ret, k )    end
	return ret
end

Table.values     = function( tbl )
	assert( type(tbl) == "table", "Expected `Table`" )
	local ret = { }
	for k,v in pairs( tbl ) do    t_insert( ret, v )    end
	return ret
end

Table.count      = function( tbl )
	assert( type(tbl) == "table", "Expected `Table`" )
	local cnt = 0
	for k,v in pairs( tbl ) do    cnt=cnt+1    end
	return cnt
end
Table.length = Table.count

Table.clone      = function( tbl, deep )
	local ret = { }
	for k,v in pairs( tbl ) do
		if deep and 'table' == type(v) then
			ret[ k ] = Table.clone( v )
		else
			ret[ k ] = v
		end
	end
	return ret
end

Table.asstring   = function( tbl, t )
	if 'keys' == t then
		return '{' .. t_concat( Table.map( Table.keys( tbl ), tostring ), ',') .. '}'
	elseif 'values' == t then
		return '{' .. t_concat( Table.map( Table.values( tbl ), tostring ), ',') .. '}'
	else
		return '{' .. t_concat( Table.map( Table.values( tbl ), tostring ), ',') .. '}'
	end
end

Table.isempty    = function( tbl )
	for k,v in pairs( tbl ) do return false end
	for i = 1, #tbl         do return false end
	return true
end


return Table
