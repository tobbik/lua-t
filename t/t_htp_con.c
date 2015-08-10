/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_con.c
 * \brief     OOP wrapper for HTTP Connection
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>               // malloc, free
#include <string.h>               // strchr, ...

#include "t.h"
#include "t_htp.h"


static int lt_htp_con__gc( lua_State *luaVM );


/**--------------------------------------------------------------------------
 * create a t_htp_con and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_con*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_con
*t_htp_con_create_ud( lua_State *luaVM, struct t_htp_srv *srv )
{
	struct t_htp_con *c;
	c = (struct t_htp_con *) lua_newuserdata( luaVM, sizeof( struct t_htp_con ) );
	c->buf_head  = NULL;   // reference to current output buffer head
	c->buf_tail  = NULL;   // reference to current output buffer head
	c->srv       = srv;
	c->cnt       = 1;
	lua_newtable( luaVM ); // empty table to hold streams inside

	c->sR        = luaL_ref( luaVM, LUA_REGISTRYINDEX );

	luaL_getmetatable( luaVM, "T.Http.Connection" );
	lua_setmetatable( luaVM, -2 );
	return c;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is a struct t_htp_con * and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_con*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_con
*t_htp_con_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Http.Connection" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Connection` expected" );
	return (struct t_htp_con *) ud;
}


// TODO: use this to adjust large incoming chnks for headers upto BUFSIZ per
// line
void t_htp_con_adjustbuffer( struct t_htp_con *c, size_t read, const char* rpos )
{
	memcpy( &(c->buf), rpos, (const char*) &(c->buf) + read - rpos );
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Connection socket.
 * Called anytime the client socket returns from the poll for read event.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_con.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_con_rcv( lua_State *luaVM )
{
	struct t_htp_con *c    = t_htp_con_check_ud( luaVM, 1, 1 );
	struct t_htp_str *s;
	int               rcvd;
	int               res;   // return result

	// read
	rcvd = t_sck_recv( luaVM, c->sck, &(c->buf[ c->read ]), BUFSIZ - c->read );

	if (! rcvd)    // peer has closed
		return lt_htp_con__gc( luaVM );
	// negotiate which stream object is responsible
	// if HTTP1.0 or HTTP1.1 this is the last, HTTP2.0 has a stream identifier
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->sR );
	lua_rawgeti( luaVM, -1, c->cnt );         // S:c,sR,s
	if (lua_isnoneornil( luaVM, -1 ))
	{          // create new stream and put into stream table
		lua_pop( luaVM, 1 );                   // pop nil( failed stream )
		s = t_htp_str_create_ud( luaVM, c );   // S:c,sR,str
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->pR );
		lua_pushstring( luaVM, "connection" );
		lua_pushvalue( luaVM, -1 );            // S:c,sR.str,pR,'connection',c
		lua_rawset( luaVM, -3 );
		lua_pop( luaVM, 1 );                   // remove s->pR
		lua_rawseti( luaVM, -2, c->cnt );      // S:c,sR
		lua_rawgeti( luaVM, -1, c->cnt );      // S:c,sR,str
	}
	else
	{
		s = t_htp_str_check_ud( luaVM, -1, 0 );
	}
	lua_remove( luaVM, -2 );       // pop the stream table

	//printf( "Received %d  \n'%s'\n", rcvd, &(m->buf[ m->read ]) );
	// TODO: set or reset c-read

	c->b = &( c->buf[ 0 ] );

	res = t_htp_str_rcv( luaVM, s, c->read + rcvd );
	switch (res)
	{
		case 0:
			c->read = 0;
		default:
			break;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Handle outgoing T.Http.Connection into it's socket.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_con.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_con_rsp( lua_State *luaVM )
{
	struct t_htp_con *c    = t_htp_con_check_ud( luaVM, 1, 1 );
	size_t            snt;
	const char       *b;
	struct t_htp_buf *buf  = c->buf_head;
	struct t_htp_str *str;

	// TODO: test for NULL
	// get tail buffer turn into char * array
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, buf->bR );
	b = lua_tostring( luaVM, -1 );
	// fetch the currently active stream for this buffer
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, buf->sR );
	str = t_htp_str_check_ud( luaVM, -1, 1 );
	printf( "Send ResponseChunk: %s\n", b );

	snt = t_sck_send( luaVM,
			c->sck,
			&(b[ buf->sl ]),
			buf->bl - buf->sl );
	buf->sl   += snt;  // How much of current buffer is sent -> adjustment
	str->rsSl += snt;  // How much of current stream is sent -> adjustment

	printf( "%zu   %zu  -- %u    %u\n", buf->sl, buf->bl,
	   buf->sl==buf->bl, buf->sl!=buf->bl );

	if (buf->bl == buf->sl)      // current buffer is sent completly
	{
		if ( T_HTP_STR_FINISH ==  str->state )
		{
			printf( "EndOfStream\n" );
			lua_pushcfunction( luaVM, lt_htp_str__gc );
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, buf->sR );
			luaL_unref( luaVM, LUA_REGISTRYINDEX, buf->sR ); // unref stream for gc
			lua_call( luaVM, 1, 0 );
		}
		// free current buffer and go backwards in linked list
		luaL_unref( luaVM, LUA_REGISTRYINDEX, buf->bR ); // unref string for gc
		c->buf_head = buf->nxt;
		free( buf );

		// TODO:  If there is no kpAlv discard connection
		//        If kpAlv and the buffers are empty, set up a timer to discard 
		//        on kpAlv timeout
		if (NULL == c->buf_head)       // current connection has no buffers left
		{
			// remove this connections socket from evLoop
			t_ael_removehandle_impl( c->srv->ael, c->sck->fd, T_AEL_WR );
			c->srv->ael->fd_set[ c->sck->fd ]->t = T_AEL_RD;
			// done with current the stream has overall
			if ( T_HTP_STR_FINISH == str->state || str->rsSl == str->rsBl)
			{
				if (! c->kpAlv)
				{
					lua_pushcfunction( luaVM, lt_htp_con__gc );
					lua_pushvalue( luaVM, 1 );
					lua_call( luaVM, 1, 0 );
					return 1;
				}
			}
		}

	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Connection by accessing proxy table.
 * This allows access to the socket and the address.
 * \param   luaVM    The lua state.
 * \lparam  Http.Connection instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__index( lua_State *luaVM )
{
	struct t_htp_con *c = t_htp_con_check_ud( luaVM, -2, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->pR );  // fetch the proxy table
	lua_pushvalue( luaVM, -2 );                      // repush the key
	lua_gettable( luaVM, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update  NOT ALLOWED.
 * \param   luaVM    The lua state.
 * \lparam  Http.Connection instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__newindex( lua_State *luaVM )
{
	t_htp_con_check_ud( luaVM, -3, 1 );

	return t_push_error( luaVM, "Can't change values in `T.Http.Connection`" );
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Connection instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_con  The Connection instance user_data.
 * \lreturn string     formatted string representing T.Http.Connection.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__tostring( lua_State *luaVM )
{
	struct t_htp_con *c = (struct t_htp_con *) luaL_checkudata( luaVM, 1, "T.Http.Connection" );

	lua_pushfstring( luaVM, "T.Http.Connection: %p", c );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance (How many streams are handled).
 * \param   luaVM      The lua state.
 * \lparam  userdata   The instance user_data.
 * \lreturn in         How many streams in this Connection.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__len( lua_State *luaVM )
{
	struct t_htp_con *c = (struct t_htp_con *) luaL_checkudata( luaVM, 1, "T.Http.Connection" );
	// TODO: return the length of the stream collection
	lua_pushinteger( luaVM, c->cnt );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Connection instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_con  The Connection instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__gc( lua_State *luaVM )
{
	struct t_htp_con *c = (struct t_htp_con *) luaL_checkudata( luaVM, 1, "T.Http.Connection" );
	struct t_htp_buf *b;

	if (LUA_NOREF != c->pR)
	{
		luaL_unref( luaVM, LUA_REGISTRYINDEX, c->pR );
		c->pR = LUA_NOREF;
	}
	// in normal operarion no buffer should still exist, this is only for 
	while (NULL != c->buf_head)
	{
		b = c->buf_head;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, b->sR ); // unref string for gc
		c->buf_head = c->buf_head->nxt;
		free( b );
	}
	if (NULL != c->sck)
	{
		t_ael_removehandle_impl( c->srv->ael, c->sck->fd, T_AEL_RD );
		t_ael_removehandle_impl( c->srv->ael, c->sck->fd, T_AEL_WR );
		c->srv->ael->fd_set[ c->sck->fd ]->t = T_AEL_NO;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, c->srv->ael->fd_set[ c->sck->fd ]->rR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, c->srv->ael->fd_set[ c->sck->fd ]->wR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, c->srv->ael->fd_set[ c->sck->fd ]->hR );
		free( c->srv->ael->fd_set[ c->sck->fd ] );
		c->srv->ael->fd_set[ c->sck->fd ] = NULL;

		t_sck_close( luaVM, c->sck );
		c->sck = NULL;
	}

	printf( "GC'ed HTTP connection: %p\n", c );

	return 0;
}


/**--------------------------------------------------------------------------
 * \brief      the HTTP Connection library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
// static const luaL_Reg t_htp_con_prx_m [] = {
// 	{"write",        lt_htp_con_write},
// 	{"finish",       lt_htp_con_finish},
// 	{"onBody",       lt_htp_con_onbody},
// 	{"writeHead",    lt_htp_con_writeHead},
// 	{NULL,    NULL}
// };



/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_htp_con( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Connection" );
	lua_pushcfunction( luaVM, lt_htp_con__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_con__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_htp_con__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_con__gc );
	lua_setfield( luaVM, -2, "__gc");
	lua_pushcfunction( luaVM, lt_htp_con__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// luaL_newmetatable( luaVM, "T.Http.Connection.Proxy" );
	// luaL_newlib( luaVM, t_htp_con_prx_m );
	// lua_setfield( luaVM, -2, "__index" );
	// lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

