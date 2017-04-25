-- \file      lua/t.lua
-- \brief     General helpers for lua-t.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local package         , format       , getmetatable, type =
      require'package', string.format, getmetatable, type

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

return {
	require  = function( name )
		local dbg, path, cpath = debug.getinfo( 2, "S" ), package.path, package.cpath
		local dir     = dbg.short_src:match( "^(.*)/")
		package.path  = format( "%s;%s/?.lua;%s/?/init.lua", package.path, dir, dir )
		package.cpath = format( "%s;%s/?.so", package.path, dir )
		local loaded  = require( name )
		package.path, package.cpath = path, cpath
		return loaded
	end,
	equals  = equals,
	type    = function( obj )
		local mt   = getmetatable( obj )
		local name = (nil ~= mt) and mt.__name or nil
		return name and name or type( obj )
	end,

--  -------------------------------------------------------------------------
--  Objects that use a proxy table such as T.Test or T.OrderedHashTable have an
--  internal tracking or proxy table which holds the data so that __index and
--  __newindex operations can be done without hassle.  Within the object, the
--  tracking table is indexed by an empty table:
--  t.proxyTableIndex = {}    -- a globally (to t) defined empty table
--  oht = Oht()               -- oht is the table instance with the metamethods
--  oht[ T.proxyTableIndex ]  -- this is the table that actually contains the
--                            -- values which are accessd and controlled by oht
--                               __index and __newindex metamethods
	proxyTableIndex  = { },
}
