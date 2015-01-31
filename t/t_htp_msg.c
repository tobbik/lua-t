/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_msg.c
 * \brief     OOP wrapper for HTTP Message (incoming or outgoing)
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>               // malloc, free
#include <string.h>               // strchr, ...

#include "t.h"
#include "t_htp.h"


static int lt_htp_msg__gc( lua_State *luaVM );


/**--------------------------------------------------------------------------
 * create a t_htp_msg and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_msg*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_msg
*t_htp_msg_create_ud( lua_State *luaVM, struct t_htp_srv *srv )
{
	struct t_htp_msg *m;
	m = (struct t_htp_msg *) lua_newuserdata( luaVM, sizeof( struct t_htp_msg ));
	lua_newtable( luaVM );
	m->obR    = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	m->obi    = 0;
	m->obc    = 0;
	m->sent   = 0;
	m->read   = 0;
	m->sent   = 0;
	m->pS     = T_HTP_STA_ZERO;
	m->mth    = T_HTP_MTH_ILLEGAL;
	m->srv    = srv;
	m->length = 0;
	m->expect = 0;

	luaL_getmetatable( luaVM, "T.Http.Message" );
	lua_setmetatable( luaVM, -2 );
	return m;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_msg struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_msg*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_msg
*t_htp_msg_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Http.Message" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Message` expected" );
	return (struct t_htp_msg *) ud;
}


static void t_htp_msg_adjustbuffer( struct t_htp_msg *m, size_t read, const char* rpos )
{
	memcpy( &(m->buf), rpos, (const char*) &(m->buf) + read - rpos );
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Message socket.
 * Called anytime the client socket returns from the poll for read.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_msg.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_msg_rcv( lua_State *luaVM )
{
	struct t_htp_msg *m   = t_htp_msg_check_ud( luaVM, 1, 1 );
	// struct t_ael     *ael;
	int               rcvd;
	const char       *nxt;   // pointer to the buffer where processing must continue

	// read
	rcvd = t_sck_recv( luaVM, m->sck, &(m->buf[ m->read ]), BUFSIZ - m->read );
	if (!rcvd)     // peer has closed
		return lt_htp_msg__gc( luaVM );
	else           // get the proxy on the stack to fill out verb/url/version/headers ...
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR ); //S:m,P

	printf( "Received %d  \n'%s'\n", rcvd, &(m->buf[ m->read ]) );

	nxt = &(m->buf[ m->read ]);

	while (NULL != nxt)
	{
		// TODO: switching should be done on state and function execution
		// never remove reading handle. Eithe keepAlive takes over or the
		// connection gets destroyed on a read of 0 bytes
		switch (m->pS)
		{
			case T_HTP_STA_ZERO:
				nxt = t_htp_pReqFirstLine( luaVM, m, nxt );
			case T_HTP_STA_HEADER:
				nxt = t_htp_pHeaderLine( luaVM, m, nxt );
				break;
			case T_HTP_STA_HEADDONE:
				// execute function from server
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->rR );
				lua_pushvalue( luaVM, 1 );
				lua_call( luaVM, 1,0 );
				// keep reading if body, else stop reading
				if (m->length > 0 )
				{
					m->pS = T_HTP_STA_BODY;
					t_htp_msg_adjustbuffer( m, rcvd, nxt );
					// read body
				}
				else
					nxt = NULL;     // signal while loop to be done
				break;
			case T_HTP_STA_BODY:
				// execute req.onData
				nxt = NULL;
				break;
			case T_HTP_STA_FINISH:
				// ignore
				nxt = NULL;
				break;
			default:
				luaL_error( luaVM, "Illegal state for T.Http.Message %d", (int) m->pS );
		}
	}

	lua_pop( luaVM, 1 );             // pop the proxy table
	return rcvd;
}


/**--------------------------------------------------------------------------
 * Handle outgoing T.Http.Message into it's socket.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_msg.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_msg_rsp( lua_State *luaVM )
{
	struct t_htp_msg *m   = t_htp_msg_check_ud( luaVM, 1, 1 );
	struct t_ael     *ael;
	size_t            len;
	const char       *buf;

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );     // fetch current buffer row
	lua_rawgeti( luaVM, -1, m->obi );
	buf      = luaL_checklstring( luaVM, -1, &len );
	m->sent += t_sck_send( luaVM, m->sck, buf, len );
	printf( "%zu   %zu   %zu   %zu   %zu\n", lua_rawlen( luaVM, -2), m->obc, m->obi, len, m->sent );

	// S:msg, cRow
	if (m->sent == len) // if current buffer row got sent completely
	{

		// done with current buffer row
		m->sent = 0;
		// done with sending
		if (lua_rawlen( luaVM, -2 ) == m->obc)
		{
			if (T_HTP_STA_FINISH == m->pS)
			{
				if (! m->kpAlv)
				{
					lua_pushcfunction( luaVM, lt_htp_msg__gc );
					lua_pushvalue( luaVM, 1 );
					lua_call( luaVM, 1, 0 );
					return 1;
				}
				else       // ready connection read again
				{
					m->pS = T_HTP_STA_ZERO;
					lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
					ael   = t_ael_check_ud( luaVM, -1, 1 );
					t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
					ael->fd_set[ m->sck->fd ]->t = T_AEL_RD;
					lua_pop( luaVM, 1 );        // pop the loop
				}
			}
			luaL_unref( luaVM, LUA_REGISTRYINDEX, m->obR );    // release buffer, allow gc
			lua_newtable( luaVM );
			m->obR = luaL_ref( luaVM, LUA_REGISTRYINDEX );   // new buffer
			m->obc = 0;
			m->obi = 0;
		}
		else   //forward to next row
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
			ael   = t_ael_check_ud( luaVM, -1, 1 );
			t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
			ael->run=0;
			lua_pushstring( luaVM, "" );       // help gc
			lua_rawseti( luaVM, -3, (m->obi)++ );
		}
	}
	return 1;
}


/**-----------------------------------------------------------------------------
 * Form HTTP response Header.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  int      HTTP status code.     // mandatory!
 * \lparam  int      length
 *           or
 * \lparam  string   HTTP message corresponding to HTTP status code.
 * \lparam  table    key:value pairs of HTTP headers.
 * \return  The # of items pushed to the stack.
 * ---------------------------------------------------------------------------*/
static int
t_htp_msg_formHeader( lua_State *luaVM, struct t_htp_msg *m, int code,
	const char *msg, int len, int t )
{
	char             *b;
	size_t            c = 0;
	luaL_Buffer       lB;

	luaL_buffinit( luaVM, &lB );
	b = luaL_prepbuffer( &lB );
	if (len)
	{
		c += sprintf( b,
			"HTTP/1.1 %d %s\r\n"
			"Connection: %s\r\n"
			"Date: %s\r\n"
			"Content-Length: %d\r\n"
			"%s",
			(int) code,                            // HTTP Status code
			msg,                                   // HTTP Status Message
			(m->kpAlv) ? "Keep-Alive" : "Close",   // Keep-Alive or close
			m->srv->fnw,                           // Formatted Date
			len,                                   // Content-Length
			(t) ? "" : "\r\n"
			);
	}
	else
	{
		c += sprintf( b,
			"HTTP/1.1 %d %s\r\n"
			"Connection: %s\r\n"
			"Date: %s\r\n"
			"Transfer-Encoding: chunked\r\n"
			"%s",
			(int) code,                            // HTTP Status code
			msg,                                   // HTTP Status Message
			(m->kpAlv) ? "Keep-Alive" : "Close",   // Keep-Alive or close
			m->srv->fnw,                           // Formatted Date
			(t) ? "" : "\r\n"
			);
	}
	if (t)
	{
		lua_pushnil( luaVM );
		while (lua_next( luaVM, t ))
		{
			// lua_pushfstring( luaVM,
			c += sprintf( b,
				"%s: %s\r\r",
				lua_tostring( luaVM, -2 ),
				lua_tostring( luaVM, -1 )
				);
			lua_pop( luaVM, 1 );      //FIXME:  this can't pop, it must remove
		}
		c += sprintf( b, "\r\r" );
	}
	luaL_pushresultsize( &lB, c );
	return 0;
}


/**-----------------------------------------------------------------------------
 * Set main values for HTTP response.
 * Takes different combinations of arguments.
 *     int                 - code
 *     int,string          - code, msg
 *     int,string,int      - code, msg, length
 *     int,int             - code, length
 * if there is a last argument being a table, it is treated as unordered key
 * value pair of header: value;
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  int      HTTP status code.     // mandatory!
 * \lparam  int      length
 *           or
 * \lparam  string   HTTP message corresponding to HTTP status code.
 * \lparam  table    key:value pairs of HTTP headers.
 * \return  The # of items pushed to the stack.
 * ---------------------------------------------------------------------------*/
static int
lt_htp_msg_writeHead( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	int               i = lua_gettop( luaVM );
	int               t = (LUA_TTABLE == lua_type( luaVM, i )); // processing headers

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
	if (LUA_TNUMBER == lua_type( luaVM, 3 ) || LUA_TNUMBER == lua_type( luaVM, 4 ))
	{
		t_htp_msg_formHeader( luaVM, m,
			(int) luaL_checkinteger( luaVM, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( luaVM, 3))   // HTTP Status message
				? lua_tostring( luaVM, 3 )
				: t_htp_status( luaL_checkinteger( luaVM, 2 ) ),
			(LUA_TNUMBER == lua_type( luaVM, 3))   // Content-Length
				?  (int) luaL_checkinteger( luaVM, 3 )
				:  (int) luaL_checkinteger( luaVM, 4 ),
			(t) ? i : 0
			);
	}
	else
	{
		t_htp_msg_formHeader( luaVM, m,
			(int) luaL_checkinteger( luaVM, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( luaVM, 3))   // HTTP Status message
				? lua_tostring( luaVM, 3 )
				: t_htp_status( luaL_checkinteger( luaVM, 2 ) ),
			0,   // Content-Length
			(t) ? i : 0
			);
	}
	lua_rawseti( luaVM, -2, ++m->obc );
	lua_pop( luaVM, 1 );
	return 0;
}


/**--------------------------------------------------------------------------
 * Write a response to the T.Http.Message.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_write( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	struct t_ael     *ael;
	size_t            sz;

	luaL_checklstring( luaVM, 2, &sz );
	if (T_HTP_STA_SEND != m->pS)
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
		lua_rawseti( luaVM, -1, ++(m->obc) );
		lua_pop( luaVM, 1 );
		lua_pushinteger( luaVM, 200 );
		lt_htp_msg_writeHead( luaVM );
	}
	else
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
		lua_rawseti( luaVM, -1, m->obc++ );
	}

	lua_pushvalue( luaVM, 2 );
	lua_rawseti( luaVM, -2, lua_rawlen( luaVM, -2 ) );

	if (T_HTP_STA_SEND != m->pS)
	{
		m->pS = T_HTP_STA_SEND;
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_addhandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Finish of sending the T.Http.Message response.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_finish( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	struct t_ael     *ael;
	size_t            sz;

	if (T_HTP_STA_SEND != m->pS)
	{
		luaL_checklstring( luaVM, 2, &sz );
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
		t_htp_msg_formHeader( luaVM, m, 200, t_htp_status( 200 ), (int) sz, 0 );
		lua_pushvalue( luaVM, 2 );
		//t_stackDump( luaVM );
		lua_concat( luaVM, 2 );
		lua_rawseti( luaVM, -2, ++(m->obc) );
		lua_rawgeti( luaVM, -1, m->obc );
		m->obi = m->obc;
		lua_pop( luaVM, 2 );  // pop buffer table, size and HTTP code
	}
	else
	{
		if (LUA_TSTRING == lua_type( luaVM, 2 ))
		{
			luaL_checklstring( luaVM, 2, &sz );
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
			lua_rawseti( luaVM, -1, ++(m->obc) );
		}
	}

	//lua_pushvalue( luaVM, 2 );
	//lua_rawseti( luaVM, -2, lua_rawlen( luaVM, -2 ) );

	if (T_HTP_STA_SEND != m->pS)
	{
		m->pS = T_HTP_STA_SEND;
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_addhandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	m->pS = T_HTP_STA_FINISH;

	return 0;
}


/**--------------------------------------------------------------------------
 * Sets the onData method in T.Http.Message.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  function to be executed when body data arrives on connection.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_onbody( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	
	if (lua_isfunction( luaVM, 2 ))
	{
		m->bR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
		return 0;
	}
	if (lua_isnoneornil( luaVM, 2 ))
	{
		m->bR = LUA_NOREF;
		return 0;
	}
	else
		return t_push_error( luaVM, "Argument must be function or nil" );
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Message by accessing proxy table.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__index( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, -2, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // fetch the proxy table
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
lt_htp_msg__newindex( lua_State *luaVM )
{
	t_htp_msg_check_ud( luaVM, -3, 1 );

	return t_push_error( luaVM, "Can't change values in `T.Http.Message`" );
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_msg  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__tostring( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

	lua_pushfstring( luaVM, "T.Http.Message: %p", m );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   luaVM      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__len( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	lua_pushinteger( luaVM, m->length );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_msg  The Message instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg__gc( lua_State *luaVM )
{
	struct t_htp_msg *m = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	struct t_ael     *ael;

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
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_RD );
		t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_NO;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->rR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->wR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ael->fd_set[ m->sck->fd ]->hR );
		free( ael->fd_set[ m->sck->fd ] );
		ael->fd_set[ m->sck->fd ] = NULL;

		t_sck_close( luaVM, m->sck );
		m->sck = NULL;
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	printf("GC'ed HTTP connection\n");

	return 0;
}



/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_msg_prx_m [] = {
	{"write",        lt_htp_msg_write},
	{"finish",       lt_htp_msg_finish},
	{"onBody",       lt_htp_msg_onbody},
	{"writeHead",    lt_htp_msg_writeHead},
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
luaopen_t_htp_msg( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Message" );
	lua_pushcfunction( luaVM, lt_htp_msg__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_msg__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_htp_msg__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_msg__gc );
	lua_setfield( luaVM, -2, "__gc");
	lua_pushcfunction( luaVM, lt_htp_msg__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	luaL_newmetatable( luaVM, "T.Http.Message.Proxy" );
	luaL_newlib( luaVM, t_htp_msg_prx_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

