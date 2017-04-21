local base = require( "t.bas" )
local package, format = require'package', string.format


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
	equals  = base.equals,
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
