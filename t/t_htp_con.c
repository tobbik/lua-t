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
	c->obR       = LUA_NOREF;  // reference to current output buffer table
	c->obidx     = 0;          // current buffer line index
	c->obcnt     = 0;          // current buffer line count
	c->obsnt     = 0;          // current buffer sent
	c->olsnt     = 0;          // current line sent
	c->srv       = srv;
	c->str       = NULL;

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
static void t_htp_con_adjustbuffer( struct t_htp_con *rcm, size_t read, const char* rpos )
{
	memcpy( &(c->buf), rpos, (const char*) &(c->buf) + read - rpos );
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Connection socket.
 * Called anytime the client socket returns from the poll for read.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_con.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_con_rcv( lua_State *luaVM )
{
	struct t_htp_con *c    = t_htp_con_check_ud( luaVM, 1, 1 );
	struct t_htp_srm *s;
	int               rcvd;
	int               res;   // return result
	const char       *nxt;   // pointer to the buffer where parsing must continue

	// read
	rcvd = t_sck_recv( luaVM, c->sck, &(c->buf[ c->read ]), BUFSIZ - c->read );

	// TODO: if HTTP 2.0 figure out current stream

	// negotiate which stream object is responsible
	// if http1.0 or http1.1 this is the last, http2.0 hast a stream identifier
	if (! rcvd)    // peer has closed
		return lt_htp_con__gc( luaVM );
	else           // get the proxy on the stack to fill out verb/url/version/headers ...
	{
		if (NULL == c->srv)   // create new stream and put into stream table
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->srm->sR ); // S:c,str
			c->srm = t_htp_srm_create_ud( luaVM, c );
			lua_rawseti( luaVM, -2, lua_rawlen( luaVM, -2 )+1 );
		}
	}

	//printf( "Received %d  \n'%s'\n", rcvd, &(m->buf[ m->read ]) );

	res = t_htp_str_rcv( luaVM, c->srm, &(c->buf[ c->read ]), c->read + rcvd );
	switch (res)
	{
		case 0:
			c->read = 
		
	return 0;
}


/**--------------------------------------------------------------------------
 * Handle outgoing T.Http.Message into it's socket.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_con.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_con_rsp( lua_State *luaVM )
{
	struct t_htp_con *m   = t_htp_con_check_ud( luaVM, 1, 1 );
	size_t            len;
	size_t            sent;
	const char       *buf;

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );     // fetch current buffer row
	lua_rawgeti( luaVM, -1, m->obi );
	printf( "%zu   %zu   %zu   ", lua_rawlen( luaVM, -2), m->obc, m->obi );
	buf      = luaL_checklstring( luaVM, -1, &len );
	sent     = t_sck_send( luaVM, m->sck, buf + m->sent, len - m->sent );
	m->osl  += sent;
	m->sent += sent;
	printf( "%zu   %zu  -- %zu   %zu   %zu\n", len, m->sent, m->obl, m->osl, sent );

	// S:stream, cRow
	if (m->sent == len) // if current buffer row got sent completely
	{
		// done with current buffer row
		m->sent = 0;
		// done with sending what the buffer table currently has
		if (m->obc == m->obi)
		{
			//printf( "%zu   %zu   %zu   %d  %d DONE\n", lua_rawlen( luaVM, -2), m->obc, m->obi, m->pS );
			if (T_HTP_STA_FINISH == m->pS || m->osl == m->obl)
			{
				printf( "EndMessage\n" );
				if (! m->kpAlv)
				{
					lua_pushcfunction( luaVM, lt_htp_con__gc );
					lua_pushvalue( luaVM, 1 );
					lua_call( luaVM, 1, 0 );
					return 1;
				}
				else       // remove writability of socket
				{
					m->pS = T_HTP_STA_ZERO;
					t_ael_removehandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
					m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RD;
					m->ocl = 0;
					m->obl = 0;
					m->osl = 0;
				}
			}
			else
			{
				t_ael_removehandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
				m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RD;
			}
			luaL_unref( luaVM, LUA_REGISTRYINDEX, m->obR );    // release buffer, allow gc
			lua_newtable( luaVM );
			m->obR = luaL_ref( luaVM, LUA_REGISTRYINDEX );   // new buffer
			m->obc = 0;
			m->obi = 0;
		}
		else   //forward to next row
		{
			//printf( "%zu   %zu   %zu   %d\n", lua_rawlen( luaVM, -2), m->obc, m->obi, m->pS );
			lua_pushstring( luaVM, "" );           // help gc
			lua_rawseti( luaVM, -3, (m->obi)++ );  // set current line as empty string and forward
		}
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Message by accessing proxy table.
 * This allows access to the socket and the address.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
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
 * \lparam  Http.Message instance.
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
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_con  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
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
	lua_pushinteger( luaVM, c->length );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_con  The Message instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_con__gc( lua_State *luaVM )
{
	struct t_htp_con *m = (struct t_htp_con *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

	if (LUA_NOREF != m->pR)
	{
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->pR );
		m->pR = LUA_NOREF;
	}
	if (LUA_NOREF != m->obR)
	{
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->obR );    // release buffer, allow gc
		m->obR = LUA_NOREF;
	}
	if (NULL != m->sck)
	{
		t_ael_removehandle_impl( m->srv->ael, m->sck->fd, T_AEL_RD );
		t_ael_removehandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
		m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_NO;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->srv->ael->fd_set[ m->sck->fd ]->rR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->srv->ael->fd_set[ m->sck->fd ]->wR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, m->srv->ael->fd_set[ m->sck->fd ]->hR );
		free( m->srv->ael->fd_set[ m->sck->fd ] );
		m->srv->ael->fd_set[ m->sck->fd ] = NULL;

		t_sck_close( luaVM, m->sck );
		m->sck = NULL;
	}

	printf("GC'ed HTTP connection: %p\n", m);

	return 0;
}


/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_con_prx_m [] = {
	{"write",        lt_htp_con_write},
	{"finish",       lt_htp_con_finish},
	{"onBody",       lt_htp_con_onbody},
	{"writeHead",    lt_htp_con_writeHead},
	{NULL,    NULL}
};



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

	luaL_newmetatable( luaVM, "T.Http.Connection.Proxy" );
	luaL_newlib( luaVM, t_htp_con_prx_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

