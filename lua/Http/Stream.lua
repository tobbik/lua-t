-- \file      lua/Http/Stream.lua
-- \brief     Http Stream implementation
-- \detail    References an Http Stream object.  It basically references a
--            single Request/Response interaction   Http2.0 introduced the
--            `Stream` terminology. This however implements it for lower
--            versions(1.1) as well.
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table, Buffer, Segment =
      require't.Loop', require't', require't.Table', require't.Buffer', require't.Buffer.Segment'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkStr  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local getStream = function( con )
	-- if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a stream identifier ... TODO:
	local stream = con.streams[ #con.streams ]
	if not stream then
		stream = Http.Stream( con )
		t_insert( con.streams, stream )
	end
	return stream
end

local recv    = function( stream )
	local buf  = stream.con.buf
	local rcvd = con.cli:recv( con.buf, Buffer.Segment( con.buf, con.rcvd+1 ) )
	T.print( "RCVD: %d bytes\n", rcvd );
	if not rcvd then
		--dispose of itself ... clear streams, buffer etc...
	else
		local stream = getStream( con )
	end
end

--[[
/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Connection socket.
 * Called anytime the client socket returns from the poll for read.
 * \param  L            lua Virtual Machine.
 * \param  struct t_htp_str struct t_htp_str.
 * \param  const char *     pointer to the buffer (already positioned).
 * \return  integer         success indicator.
 *  -------------------------------------------------------------------------*/
int
t_htp_str_rcv( lua_State *L, struct t_htp_str *s, size_t rcvd )
{
	const char *b = &(s->con->buf[ s->con->read ]);

	while (NULL != b)
	{
		switch (s->state)
		{
			case T_HTP_STR_ZERO:
				lua_rawgeti( L, LUA_REGISTRYINDEX, s->pR );
				b = t_htp_pReqFirstLine( L, s, rcvd );
				break;
			case T_HTP_STR_FLINE:
				//lua_rawgeti( L, LUA_REGISTRYINDEX, s->pR );
				b = t_htp_pHeaderLine( L, s, rcvd );
				break;
			case T_HTP_STR_HEADDONE:
				s->con->cnt++;
				lua_pop( L, 1 );      // pop s->pR
				// execute function from server
				lua_rawgeti( L, LUA_REGISTRYINDEX, s->con->srv->rR );
				lua_pushvalue( L, 2 );
				lua_call( L, 1, 0 );
				// if request has content length keep reading body, else stop reading
				if (s->rqCl > 0 )
				{
					s->state = T_HTP_STR_BODY;
					t_htp_con_adjustbuffer( s->con, rcvd, b );
					// read body
				}
				else
					b = NULL;     // signal while loop to be done
				break;
		/*
			case T_htp_str_S_BODY:
				// execute req.onData
				nxt = NULL;
				break;
			case T_htp_str_S_FINISH:
				// ignore
				nxt = NULL;
				break;
		*/
			default:
				luaL_error( L, "Illegal state for "T_HTP_STR_TYPE" %d", (int) s->state );
		}
		//if (NULL == b)
		//{
		//}

	}
	return 1;  /// TODO: make sense of this
}
--]]

-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Http.Stream"
}

return setmetatable( {
	toString = function( srv ) return _mt.__name end
}, {
	__call   = function( self, con )
		assert( T.type( con ) == 't.Http.Connection',  "`t.Http.Connection` is required" )
		local stream = {
			  rqCLen     = 0   -- request Length as reported by Header
			, rsCLen     = 0   -- response Length as calculated or set by application
			, rsBLen     = 0   -- size of Response Buffer
			, cb         = nil -- request handler function( callback )
			, state      = Http.Stream.Zero
			, method     = Http.Method.Illegal
			, version    = Http.Version.VER09
			, conn       = con
		}
		return setmetatable( { [ T.prxTblIdx ] = con }, _mt )
	end
} )

