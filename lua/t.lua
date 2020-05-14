-- \file      lua/t.lua
-- \brief     General helpers for lua-t.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local package, s_format     , getmetatable, type, d_getinfo    =
      package, string.format, getmetatable, type, debug.getinfo

local equals
equals= function( o1, o2 )
	if o1 == o2 then return true end
	local m1,m2,t1,t2 = getmetatable( o1 ),getmetatable( o2 ), type( o1 ), type( o2 )
	if (m1 and m1.__eq) or "table" ~= t1 or t1 ~= t2 then return o1==o2 end
	-- as per condition above only executed for 'table' type
	if #o1 ~= #o2 then return false end
	for k1,v1 in pairs( o1 ) do if not equals( o2[ k1 ], v1 ) then return false end end
	for k2,v2 in pairs( o2 ) do if not equals( o1[ k2 ], v2 ) then return false end end
	return true
end

local proxyTableIndex = { }
debug.getregistry( )[ 'T.ProxyTableIndex' ] = proxyTableIndex

return {
	require  = function( name )
		local dir                                           , path        , cpath =
		      d_getinfo( 2, "S" ).source:match( "^@(.*)/" ), package.path, package.cpath
		if dir then  -- matched means started with @, means it's a path
			package.path  = s_format( "%s;%s/?.lua;%s/?/init.lua", path, dir, dir )
			package.cpath = s_format( "%s;%s/?.so", cpath, dir )
		end
		local loaded  = require( name )
		package.path, package.cpath = path, cpath     --  restore original
		return loaded
	end,
	equals  = equals,
	type    = function( obj )
		local mt   = getmetatable( obj )
		local name = (nil ~= mt) and mt.__name or nil
		return name and name or type( obj )
	end,
	assert  = function( cnd, ... ) assert( cnd, s_format( ... ) ) end,
	print   = function( fmt, ... ) print( s_format( fmt, ... ) ) end,

--  -------------------------------------------------------------------------
--  Objects that use a proxy table such as t.Test or t.OrderedHashTable have an
--  internal tracking or proxy table which holds the data so that __index and
--  __newindex operations can be used without hassle.  Within the object, the
--  tracking table is indexed by an empty table:
--  t.proxyTableIndex = {}    -- a globally (to t) defined empty table
--  oht = OrderedHashTable()  -- oht is the table instance with the metamethods
--  oht[ t.proxyTableIndex ]  -- this is the table that actually contains the
--                            -- values which are accessd and controlled by oht
--                               __index and __newindex metamethods
	proxyTableIndex  = proxyTableIndex,
}
