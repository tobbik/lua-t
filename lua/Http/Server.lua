-- \file      lua/Http/Server.lua
-- \brief     Http Server implementation
-- \detail    As simple callback based HTTP server.  Requires to be controlled
--            via t.Loop asynchronously.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_concat     , getmetatable, setmetatable, pairs, assert, next, type =
      table.concat , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table = require't.Loop', require't', require't.Table'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkSrv  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local setNow  = function( srv, force )
	local now = os.time( )
	-- only update the string if a second or more has passed
	if now - srv.now > 0 or force then
		srv.date = os.date( "%a, %d %b %Y %H:%M:%S %Z", now )
	end
end

local accept = function( srv )
	local cli, adr  = srv.sck:accept( )
	local con       = Http.Connection( srv, cli, adr )
end

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Http.Server"
}

return setmetatable( {
	toString = function( srv ) return _mt.__name end
}, {
	__call   = function( self, ael, cb )
		assert( T.type( ael ) == 'T.Loop',   "`T.Loop` is required" )
		assert( type( cb )    == 'function', "Callback function required" )
		local srv  = {
			  ael = ael
			, cb  = cb
		}
		setNow( srv, true )
		return setmetatable( { [ T.prxTblIdx ] = srv }, _mt )
	end
} )

