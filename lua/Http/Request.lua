-- \file      lua/Http/Request.lua
-- \brief     Http Request implementation
-- \detail    References an Http Request object.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type

local Loop, T, Table, Buffer =
      require't.Loop', require't', require't.Table', require't.Buffer'
local           Http ,                Method ,                Version ,                Response =
      require't.Http', require't.Http.Method', require't.Http.Version', require't.Http.Response'

--local _mt
local _mt = debug.getregistry( )[ "T.Http.Request" ]

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkStr  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local State = {
	  Method   = 1
	, Url      = 2
	, Version  = 3
	, Headers  = 4
	, Body     = 5
	, Done     = 6
}

-- ---------------------------- Instance metatable --------------------
--_mt = {       -- local _mt at top of file
--	-- essentials
--	  __name     = "t.Http.Request"
--	, recv       = recv
--}

--_mt.__index     = _mt
_mt.recv       = recv
_mt.__name     = "t.Http.Request"
for k,v in pairs(_mt) do print(k,v) end

return setmetatable( {
	  State  = State
}, {
	__call   = function( self, callback )
		local request = {
			  rqCLen     = 0        -- request Length as reported by Header
			, rsCLen     = 0        -- response Length as calculated or set by application
			, rsBLen     = 0        -- size of Response Buffer
			, state      = State.Method
			, method     = Method.ILLEGAL
			, version    = Version.ILLEGAL
			, keepAlive  = false
			, callback   = callback
			, headers    = { }      -- pre-existent for re-entrant header parsing
		}
		return setmetatable( request, _mt )
	end
} )

