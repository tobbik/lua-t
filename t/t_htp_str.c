/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_str.c
 * \brief     OOP wrapper for HTTP Message (incoming or outgoing)
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <stdlib.h>               // malloc, free
#include <string.h>               // strchr, ...

#include "t.h"
#include "t_htp.h"


static int lt_htp_str__gc( lua_State *luaVM );


/**--------------------------------------------------------------------------
 * create a t_htp_str and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_str*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_str
*t_htp_str_create_ud( lua_State *luaVM, struct t_htp_con *con )
{
	struct t_htp_str *c;
	c = (struct t_htp_str *) lua_newuserdata( luaVM, sizeof( struct t_htp_str ));
	lua_newtable( luaVM );
	c->obR     = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	c->obi     = 0;
	c->obc     = 0;
	c->sent    = 0;
	c->read    = 0;
	c->sent    = 0;
	c->pS      = T_htp_str_S_ZERO;
	c->mth     = T_HTP_MTH_ILLEGAL;
	c->con     = con;
	c->length  = 0;
	c->expect  = 0;
	c->ocl     = 0;
	c->osl     = 0;
	c->obl     = 0;

	luaL_getmetatable( luaVM, "T.Http.Stream" );
	lua_setmetatable( luaVM, -2 );
	return m;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_str struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_str*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_str
*t_htp_str_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Http.Stream" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Stream` expected" );
	return (struct t_htp_str *) ud;
}


// TODO: use this to adjust large incoming chnks for headers upto BUFSIZ per
// line
static void t_htp_str_adjustbuffer( struct t_htp_str *s, size_t read, const char* rpos )
{
	memcpy( &(s->buf), rpos, (const char*) &(s->buf) + read - rpos );
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Message socket.
 * Called anytime the client socket returns from the poll for read.
 * \param  luaVM            lua Virtual Machine.
 * \param  struct t_htp_str struct t_htp_str.
 * \param  const char *     pointer to the buffer (already positioned).
 * \return  integer         success indicator.
 *  -------------------------------------------------------------------------*/
int
t_htp_str_rcv( lua_State *luaVM, struct t_htp_str *s, size_t rcvd )
{
	const char *b = &(s->con->buf[ m->con->read ]);

	while (NULL != b)
	{
		switch (c->msg->state)
		{
			case T_HTP_STR_S_ZERO:
				b = t_htp_pReqFirstLine( luaVM, s, rcvd );
				break;
			case T_HTP_STR_S_FLINE:
				nxt = t_htp_pHeaderLine( luaVM, s, rcvd );
				break;
			case T_HTP_STR_S_HEADDONE:
				// execute function from server
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->srv->rR );
				lua_pushvalue( luaVM, 1 );
				lua_call( luaVM, 1,0 );
				// keep reading if body, else stop reading
				if (m->length > 0 )
				{
					m->pS = T_HTP_STA_BODY;
					t_htp_con_adjustbuffer( m, rcvd, nxt );
					// read body
				}
				else
					nxt = NULL;     // signal while loop to be done
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
				luaL_error( luaVM, "Illegal state for T.Http.Message %d", (int) m->pS );
		}
		if (NULL == b)
		{
	}
}


/**--------------------------------------------------------------------------
 * Handle outgoing T.Http.Message into it's socket.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_str.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int
t_htp_str_rsp( lua_State *luaVM )
{
	struct t_htp_str *m   = t_htp_str_check_ud( luaVM, 1, 1 );
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

	// S:msg, cRow
	if (m->sent == len) // if current buffer row got sent completely
	{
		// done with current buffer row
		m->sent = 0;
		// done with sending what the buffer table currently has
		if (m->obc == m->obi)
		{
			//printf( "%zu   %zu   %zu   %d  %d DONE\n", lua_rawlen( luaVM, -2), m->obc, m->obi, m->pS );
			if (T_htp_str_S_FINISH == m->pS || m->osl == m->obl)
			{
				printf( "EndMessage\n" );
				if (! m->kpAlv)
				{
					lua_pushcfunction( luaVM, lt_htp_str__gc );
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
 * TODO: refactor to accept luaL_Buffer instead of char* buffer
 * ---------------------------------------------------------------------------*/
static size_t
t_htp_str_formHeader( lua_State *luaVM, char *b, struct t_htp_str *m,
	int code, const char *msg, int len, int t )
{
	size_t            c;

	if (len)
	{
		c = sprintf( b,
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
		m->ocl = len;
	}
	else
	{
		c = sprintf( b,
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
		m->ocl = 0;
	}
	if (t)
	{
		lua_pushnil( luaVM );
		while (lua_next( luaVM, t ))
		{
			c += sprintf( b,
				"%s: %s\r\r",
				lua_tostring( luaVM, -2 ),
				lua_tostring( luaVM, -1 )
				);
			lua_pop( luaVM, 1 );      //FIXME:  this can't pop, it must remove
		}
		c += sprintf( b, "\r\r" );
	}
	m->obl = (len) ? c + len : 0;
	return c;
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
lt_htp_str_writeHead( lua_State *luaVM )
{
	struct t_htp_str *m = t_htp_str_check_ud( luaVM, 1, 1 );
	int               i = lua_gettop( luaVM );
	int               t = (LUA_TTABLE == lua_type( luaVM, i )); // processing headers
	char             *b;
	size_t            c = 0;
	luaL_Buffer       lB;

	luaL_buffinit( luaVM, &lB );
	b = luaL_prepbuffer( &lB );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
	if (LUA_TNUMBER == lua_type( luaVM, 3 ) || LUA_TNUMBER == lua_type( luaVM, 4 ))
	{
		c = t_htp_str_formHeader( luaVM, b, m,
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
		c = t_htp_str_formHeader( luaVM, b, m,
			(int) luaL_checkinteger( luaVM, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( luaVM, 3))   // HTTP Status message
				? lua_tostring( luaVM, 3 )
				: t_htp_status( luaL_checkinteger( luaVM, 2 ) ),
			0,                                     // Content-Length 0 -> chunked
			(t) ? i : 0
			);
	}
	luaL_pushresultsize( &lB, c );
	lua_rawseti( luaVM, -2, ++(m->obc) );
	m->obi = m->obc;

	m->pS = T_HTP_STA_SEND;
	t_ael_addhandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
	m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
	lua_pop( luaVM, 2 );  // pop buffer table and loop
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
lt_htp_str_write( lua_State *luaVM )
{
	struct t_htp_str *m   = t_htp_str_check_ud( luaVM, 1, 1 );
	size_t            sz;
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;

	luaL_checklstring( luaVM, 2, &sz );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );

	// this assumes first ever call is write -> chunked
	if (T_HTP_STA_SEND != m->pS)
	{
		luaL_buffinit( luaVM, &lB );
		b = luaL_prepbuffer( &lB );
		c = t_htp_str_formHeader( luaVM, b, m, 200, t_htp_status( 200 ), 0, 0 );
		c += sprintf( b+c, "%zx\r\n", sz );
		luaL_addsize( &lB, c );
		lua_pushvalue( luaVM, 2 );
		luaL_addvalue( &lB );
		luaL_addlstring( &lB, "\r\n", 2 );
		luaL_pushresult( &lB );
		lua_rawseti( luaVM, -2, ++(m->obc) );
		m->obi = m->obc;

		m->pS = T_HTP_STA_SEND;
	}
	else
	{
		if (! m->ocl)
		{
			luaL_buffinit( luaVM, &lB );
			b = luaL_prepbuffer( &lB );
			c = sprintf( b, "%zx\r\n", sz );
			luaL_addsize( &lB, c );
			lua_pushvalue( luaVM, 2 );
			luaL_addvalue( &lB );
			luaL_addlstring( &lB, "\r\n", 2 );
			luaL_pushresult( &lB );
		}
		else
			lua_pushvalue( luaVM, 2 );
		lua_rawseti( luaVM, -2, ++(m->obc) );
	}
	if ( 1 == m->obc )  // wrote the first line to the buffer, can also happen if
	{                   // current buffer is flushed but response is incomplete
		t_ael_addhandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
		m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
		m->obi =1;
	}
	lua_pop( luaVM, 1 );                    // pop buffer table and loop

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
lt_htp_str_finish( lua_State *luaVM )
{
	struct t_htp_str *m = t_htp_str_check_ud( luaVM, 1, 1 );
	size_t            sz;
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;


	if (T_HTP_STA_SEND != m->pS)
	{
		luaL_checklstring( luaVM, 2, &sz );
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
		luaL_buffinit( luaVM, &lB );
		b = luaL_prepbuffer( &lB );
		c = t_htp_str_formHeader( luaVM, b, m, 200, t_htp_status( 200 ), (int) sz, 0 );
		luaL_addsize( &lB, c );
		lua_pushvalue( luaVM, 2 );
		luaL_addvalue( &lB );
		luaL_pushresult( &lB );
		lua_rawseti( luaVM, -2, ++(m->obc) );
		m->obi = m->obc;

		t_ael_addhandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
		m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
	}
	else
	{
		if (LUA_TSTRING == lua_type( luaVM, 2 ))
		{
			luaL_checklstring( luaVM, 2, &sz );
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->obR );
			if (! m->ocl)   // chunked
			{
				luaL_buffinit( luaVM, &lB );
				b = luaL_prepbuffer( &lB );
				c = sprintf( b, "%zx\r\n", sz );
				luaL_addsize( &lB, c );
				lua_pushvalue( luaVM, 2 );
				luaL_addvalue( &lB );
				luaL_addlstring( &lB, "\r\n0\r\n\r\n", 7 );
				luaL_pushresult( &lB );
			}
			else  // TODO: check that  size + buffer sz does not exceed m->ocl
				lua_pushvalue( luaVM, 2 );
			lua_rawseti( luaVM, -2, ++(m->obc) );
		}
	}
	if ( 1 == m->obc )  // wrote the first line to the buffer
	{
		t_ael_addhandle_impl( m->srv->ael, m->sck->fd, T_AEL_WR );
		m->srv->ael->fd_set[ m->sck->fd ]->t = T_AEL_RW;
		m->obi =1;
	}
	if ( 0 == m->obc )
	{
		if ( ! m->kpAlv)
		{
			lua_pushcfunction( luaVM, lt_htp_str__gc );
			lua_pushvalue( luaVM, 1 );
			lua_call( luaVM, 1, 0 );
		}
		else
			m->pS = T_htp_str_S_ZERO;
	}

	lua_pop( luaVM, 1 );  // pop buffer table
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
lt_htp_str_onbody( lua_State *luaVM )
{
	struct t_htp_str *m = t_htp_str_check_ud( luaVM, 1, 1 );
	
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
lt_htp_str__index( lua_State *luaVM )
{
	struct t_htp_str *m = t_htp_str_check_ud( luaVM, -2, 1 );

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
lt_htp_str__newindex( lua_State *luaVM )
{
	t_htp_str_check_ud( luaVM, -3, 1 );

	return t_push_error( luaVM, "Can't change values in `T.Http.Message`" );
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_str  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__tostring( lua_State *luaVM )
{
	struct t_htp_str *m = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

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
lt_htp_str__len( lua_State *luaVM )
{
	struct t_htp_str *m = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	lua_pushinteger( luaVM, m->length );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_str  The Message instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__gc( lua_State *luaVM )
{
	struct t_htp_str *m = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

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
static const luaL_Reg t_htp_str_prx_m [] = {
	{"write",        lt_htp_str_write},
	{"finish",       lt_htp_str_finish},
	{"onBody",       lt_htp_str_onbody},
	{"writeHead",    lt_htp_str_writeHead},
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
luaopen_t_htp_str( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Stream" );
	lua_pushcfunction( luaVM, lt_htp_str__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_str__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_htp_str__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_str__gc );
	lua_setfield( luaVM, -2, "__gc");
	lua_pushcfunction( luaVM, lt_htp_str__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	luaL_newmetatable( luaVM, "T.Http.Stream.Proxy" );
	luaL_newlib( luaVM, t_htp_str_prx_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
