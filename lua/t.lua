-- \file      lua/t.lua
-- \brief     General helpers for lua-t.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local package, s_format     , getmetatable, type, d_getinfo    =
      package, string.format, getmetatable, type, debug.getinfo

return {
	require  = function( name )
		local dir                                          , path        , cpath =
		      d_getinfo( 2, "S" ).source:match( "^@(.*)/" ), package.path, package.cpath
		if dir then  -- matched means started with @, means it's a path
			package.path  = s_format( "%s;%s/?.lua;%s/?/init.lua", path, dir, dir )
			package.cpath = s_format( "%s;%s/?.so", cpath, dir )
		end
		local loaded  = require( name )
		package.path, package.cpath = path, cpath     --  restore original
		return loaded
	end,
	type    = function( obj )
		local mt   = getmetatable( obj )
		local name = (nil ~= mt) and mt.__name or nil
		return name and name or type( obj )
	end,
	assert  = function( cnd, ... ) assert( cnd, s_format( ... ) ) end,
	print   = function( fmt, ... ) print( s_format( fmt, ... ) ) end,
}
