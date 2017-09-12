-- \file      lua/Http/Connection.lua
-- \brief     Http Connection implementation
-- \detail    References an Http Connection to a Single Client
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local prxTblIdx,Table  = require( "t" ).proxyTableIndex, require( "t.Table" )
local t_insert     , getmetatable, setmetatable, pairs, assert, next, type =
      table.insert , getmetatable, setmetatable, pairs, assert, next, type
local t_merge,     t_complement,     t_contains,     t_count,     t_keys,     t_asstring =
      Table.merge, Table.complement, Table.contains, Table.count, Table.keys, Table.asstring

local Loop, T, Table, Buffer, Segment = require't.Loop', require't', require't.Table', require't.Buffer', require't.Buffer.Segment'

local _mt

-- ---------------------------- general helpers  --------------------
-- assert Set type and return the proxy table
local chkCon  = function( self )
	T.assert( _mt == getmetatable( self ), "Expected `%s`, got %s", _mt.__name, T.type( self ) )
	return self[ T.prxTblIdx ]
end

local getStream = function( con )
	-- if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a stream identifier ... TODO:
	local stream = cons.streams[ #con.streams ]
	if not stream then
		stream = Http.Stream( con )
		t_insert( con.streams, stream )
	end
	return stream
end

local recv    = function( con )
	local segm = Buffer.Segment( con.buf, con.rcvd+1 )
	local rcvd = con.cli:recv( segm )
	t.print( "RCVD: %d bytes\n", rcvd );
	if not rcvd then
		--dispose of itself ... clear streams, buffer etc...
	else
		segm:setSize( rcvd );
		local stream = getStream( con )
	end
end
--[[
int
t_htp_con_rcv( lua_State *L )
{
	struct t_htp_con *c    = t_htp_con_check_ud( L, 1, 1 );
	struct t_htp_str *s;
	int               rcvd;
	int               res;   // return result

	// read
	rcvd = p_net_sck_recv( L, c->sck, NULL, &(c->buf[ c->read ]), BUFSIZ - c->read );
	printf( "RCVD: %d bytes\n", rcvd );

	if (! rcvd)    // peer has closed
		return lt_htp_con__gc( L );
	// negotiate which stream object is responsible
	// if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a stream identifier
	lua_rawgeti( L, LUA_REGISTRYINDEX, c->sR );
	lua_rawgeti( L, -1, c->cnt );         // S:c,sR,s
	if (lua_isnoneornil( L, -1 ))
	{          // create new stream and put into stream table
		lua_pop( L, 1 );                   // pop nil( failed stream )
		s = t_htp_str_create_ud( L, c );   // S:c,sR,str
		lua_rawgeti( L, LUA_REGISTRYINDEX, s->pR );
		lua_pushstring( L, "connection" );
		lua_pushvalue( L, -1 );            // S:c,sR.str,pR,'connection',c
		lua_rawset( L, -3 );
		lua_pop( L, 1 );                   // remove s->pR
		lua_rawseti( L, -2, c->cnt );      // S:c,sR
		lua_rawgeti( L, -1, c->cnt );      // S:c,sR,str
	}
	else
	{
		s = t_htp_str_check_ud( L, -1, 0 );
	}
	lua_remove( L, -2 );       // pop the stream table

	//printf( "Received %d  \n'%s'\n", rcvd, &(m->buf[ m->read ]) );
	// TODO: set or reset c-read

	c->b = &( c->buf[ 0 ] );

	res = t_htp_str_rcv( L, s, c->read + rcvd );
	switch (res)
	{
		case 0:
			c->read = 0;
		default:
			break;
	}

	return 0;
}
--]]


-- ---------------------------- Instance metatable --------------------
_mt = {       -- local _mt at top of file
	-- essentials
	__name     = "t.Http.Connection"
}

return setmetatable( {
	toString = function( srv ) return _mt.__name end
}, {
	__call   = function( self, srv, cli, adr )
		assert( T.type( srv ) == 't.Http.Server',  "`t.Http.Server` is required" )
		assert( T.type( cli ) == 'T.Net.Socket',   "`T.Net.Socket` is required" )
		assert( T.type( adr ) == 'T.Net.Address',  "`T.Net.Address` is required" )

		local con  = {
			  srv     = srv
			, cli     = cli
			, adr     = adr
			, buf     = Buffer( Buffer.Size ) -- the read buffer
			, rcvd    = 0
			, streams = { }
		}

		srv.ael:addHandle( cli, 'read',  recv, con )
		srv.ael:addHandle( cli, 'write', resp, con )
		return setmetatable( { [ T.prxTblIdx ] = con }, _mt )
	end
} )

