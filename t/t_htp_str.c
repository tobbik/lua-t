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

/**--------------------------------------------------------------------------
 * create a t_htp_str and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_str*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_str
*t_htp_str_create_ud( lua_State *luaVM, struct t_htp_con *con )
{
	struct t_htp_str *s;
	s = (struct t_htp_str *) lua_newuserdata( luaVM, sizeof( struct t_htp_str ));
	// Proxy contains lua readable items such as headers, length, status code etc
	lua_newtable( luaVM );
	luaL_getmetatable( luaVM, "T.Http.Stream.Proxy" );
	lua_setmetatable( luaVM, -2 );
	s->pR      = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	s->rqCl    = 0;                 ///< request  content length
	s->rsCl    = 0;                 ///< response content length
	s->rsBl    = 0;                 ///< response buffer length (headers + rsCl)
	s->bR      = 0;                 ///< Lua registry reference to body handler function
	s->state   = T_HTP_STR_ZERO;    ///< shall the connection return an expected thingy?
	s->mth     = T_HTP_MTH_ILLEGAL; ///< HTTP Message state
	s->ver     = T_HTP_VER_09;      ///< HTTP Method for this request
	s->con     = con;               ///< connection

	luaL_getmetatable( luaVM, "T.Http.Stream" );
	lua_setmetatable( luaVM, -2 );
	return s;
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


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Connection socket.
 * Called anytime the client socket returns from the poll for read.
 * \param  luaVM            lua Virtual Machine.
 * \param  struct t_htp_str struct t_htp_str.
 * \param  const char *     pointer to the buffer (already positioned).
 * \return  integer         success indicator.
 *  -------------------------------------------------------------------------*/
int
t_htp_str_rcv( lua_State *luaVM, struct t_htp_str *s, size_t rcvd )
{
	const char *b = &(s->con->buf[ s->con->read ]);

	while (NULL != b)
	{
		switch (s->state)
		{
			case T_HTP_STR_ZERO:
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->pR );
				b = t_htp_pReqFirstLine( luaVM, s, rcvd );
				break;
			case T_HTP_STR_FLINE:
				//lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->pR );
				b = t_htp_pHeaderLine( luaVM, s, rcvd );
				break;
			case T_HTP_STR_HEADDONE:
				s->con->cnt++;
				lua_pop( luaVM, 1 );      // pop s->pR
				// execute function from server
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->con->srv->rR );
				lua_pushvalue( luaVM, 2 );
				lua_call( luaVM, 1, 0 );
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
				luaL_error( luaVM, "Illegal state for T.Http.Message %d", (int) s->state );
		}
		//if (NULL == b)
		//{
		//}

	}
	return 1;  /// TODO: make sense of this
}


/**--------------------------------------------------------------------------
 * Add a new buffer chunk to the Linked List buffer in t_htp_con.
 * General handling of buffers within the connection.  It does expect a Lua
 * string on top of the stack which will be wrapped into a linked list element.
 * It also expects the t_htp_str element on stack position 1. If the current
 * buffer head is null, the connections socket must also be added to the
 * EventLoop for outgoing connections.
 * \param   luaVM        The lua state.
 * \param   integer      The string length of the chunk on stack.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
t_htp_str_addbuffer( lua_State *luaVM, struct t_htp_str *s, size_t l, int last )
{
	struct t_htp_con *c = s->con;
	struct t_htp_buf *b = malloc( sizeof( struct t_htp_buf ) );

	b->bl   = l;
	b->sl   = 0;
	b->bR   = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	b->nxt  = NULL;
	b->prv  = NULL;
	lua_pushvalue( luaVM, 1 );
	b->sR   = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	b->last = last;

	if (NULL == c->buf_head)
	{
		c->buf_head = b;
		c->buf_tail = b;
		// wrote the first line to the buffer, can also happen if
		// current buffer is flushed but response is incomplete
		t_ael_addhandle_impl( c->srv->ael, c->sck->fd, T_AEL_WR );
		c->srv->ael->fd_set[ c->sck->fd ]->t = T_AEL_RW;
	}
	else
	{
		c->buf_tail->nxt = b;
		b->prv           = c->buf_tail;
		c->buf_tail      = b;
	}
	return 1;
}


/**-----------------------------------------------------------------------------
 * Form HTTP response Header.
 * \param  luaVM        The lua state.
 * \param  luaL_Buffer  Lua Buffer pointer.
 * \param  struct t_htp_str struct pointer.
 * \param  int          the HTTP Status Code to be returned.
 * \param  char*        the HTTP Status Message to be returned.
 * \param  size_t*      length of the HTTP Payload aka. Content-length.
 * \param  int          position of table on stack where headers are present.
 *                      0 means no additional headers.
 * \return  int         size of string added to the buffer.
 * ---------------------------------------------------------------------------*/
static size_t
t_htp_str_formHeader( lua_State *luaVM, luaL_Buffer *lB, struct t_htp_str *s,
	int code, const char *msg, int len, int t )
{
	size_t   c;       ///< count all chars added in this method
	size_t   bs;      ///< chars added currently to buffers
	char    *b = luaL_prepbuffer( lB );

	if (len)
	{
		bs = sprintf( b,
			"HTTP/1.1 %d %s\r\n"
			"Connection: %s\r\n"
			"Date: %s\r\n"
			"Content-Length: %d\r\n"
			"%s",
			(int) code,                               // HTTP Status code
			(NULL == msg) ? t_htp_status( code ) : msg, // HTTP Status Message
			(s->con->kpAlv) ? "Keep-Alive" : "Close", // Keep-Alive or close
			s->con->srv->fnw,                         // Formatted Date
			len,                                      // Content-Length
			(t) ? "" : "\r\n"
			);
		s->rsCl = len;
	}
	else
	{
		bs = sprintf( b,
			"HTTP/1.1 %d %s\r\n"
			"Connection: %s\r\n"
			"Date: %s\r\n"
			"Transfer-Encoding: chunked\r\n"
			"%s",
			(int) code,                               // HTTP Status code
			(NULL == msg) ? t_htp_status( code ) : msg, // HTTP Status Message
			(s->con->kpAlv) ? "Keep-Alive" : "Close", // Keep-Alive or close
			s->con->srv->fnw,                         // Formatted Date
			(t) ? "" : "\r\n"
			);
		s->rsCl = len;
	}
	luaL_addsize( lB, bs );
	// TODO: Find a way to deal more efficiently with that buffer
	b  = luaL_prepbuffer( lB );
	c  = bs;
	bs = 0;
	if (t)
	{
		lua_pushnil( luaVM );
		while (lua_next( luaVM, t ))
		{
			bs += sprintf( b,
				"%s: %s\r\n",
				lua_tostring( luaVM, -2 ),
				lua_tostring( luaVM, -1 )
				);
			lua_pop( luaVM, 1 );      //FIXME:  this can't pop, it must remove
		}
		bs += sprintf( b, "\r\n" );   // finish off the HTTP Headers part
		luaL_addsize( lB, bs );
	}
	c += bs;
	s->rsBl = (len) ? c + len : 0;
	return c;
}


/**-----------------------------------------------------------------------------
 * Set main values for HTTP response.
 * Takes different combinations of arguments.
 *     int                 - HTTP status code
 *     int,string          - HTTP status code, msg
 *     int,string,int      - HTTP status code, msg, length
 *     int,int             - HTTP status code, length
 * if there is a last argument being a table, it is treated as unordered key
 * value pair of header: value;
 * \param   luaVM    The lua state.
 * \lparam  T.Http.Stream instance.
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
	struct t_htp_str *s = t_htp_str_check_ud( luaVM, 1, 1 );
	int               i = lua_gettop( luaVM );
	int               t = (LUA_TTABLE == lua_type( luaVM, i )); // processing headers
	size_t            c = 0;
	luaL_Buffer       lB;

	luaL_buffinit( luaVM, &lB );
	// indicate the Content-Length was provided
	if (LUA_TNUMBER == lua_type( luaVM, 3 ) || LUA_TNUMBER == lua_type( luaVM, 4 ))
	{
		c = t_htp_str_formHeader( luaVM, &lB, s,
			(int) luaL_checkinteger( luaVM, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( luaVM, 3))   // HTTP Status message
				? lua_tostring( luaVM, 3 )
				: t_htp_status( luaL_checkinteger( luaVM, 2 ) ),
			(LUA_TNUMBER == lua_type( luaVM, 3))   // Content-Length
				?  (int) luaL_checkinteger( luaVM, 3 )
				:  (int) luaL_checkinteger( luaVM, 4 ),
			(t) ? i : 0                            // position of optional header table on stack
			);
	}
	else     // Prepare headers for chunked encoding
	{
		c = t_htp_str_formHeader( luaVM, &lB, s,
			(int) luaL_checkinteger( luaVM, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( luaVM, 3))   // HTTP Status message
				? lua_tostring( luaVM, 3 )
				: t_htp_status( luaL_checkinteger( luaVM, 2 ) ),
			0,                                     // Content-Length 0 -> chunked
			(t) ? i : 0                            // position of optional header table on stack
			);
	}
	luaL_pushresult( &lB );
	t_htp_str_addbuffer( luaVM, s, lB.n, 0 );
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
	struct t_htp_str *s   = t_htp_str_check_ud( luaVM, 1, 1 );
	size_t            sz;
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;
	luaL_buffinit( luaVM, &lB );

	luaL_checklstring( luaVM, 2, &sz );

	// this assumes first ever call is write -> chunked
	if (T_HTP_STR_SEND != s->state)
	{
		luaL_buffinit( luaVM, &lB );
		c = t_htp_str_formHeader( luaVM, &lB, s, 200, NULL, 0, 0 );
		b = luaL_prepbuffer( &lB );
		c = sprintf( b, "%zx\r\n", sz );
		luaL_addsize( &lB, c );
		lua_pushvalue( luaVM, 2 );
		luaL_addvalue( &lB );
		luaL_addlstring( &lB, "\r\n", 2 );
		luaL_pushresult( &lB );
		printf( "Header size: %zu    ----  %zu \n", c, lB.n );
		s->state = T_HTP_STR_SEND;
	}
	else
	{
		// if the response Content-length is not known when we are sending
		// the encoding must be chunked
		if (! s->rsCl)
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
	}
	// TODO: 
	t_htp_str_addbuffer( luaVM, s, lB.n, 0 );

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
	struct t_htp_str *s = t_htp_str_check_ud( luaVM, 1, 1 );
	size_t            sz;  /// length of optional string handed in
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;

	// the first action ever called on the stream, prep header first
	if (T_HTP_STR_SEND != s->state)
	{
		luaL_checklstring( luaVM, 2, &sz );
		luaL_buffinit( luaVM, &lB );
		c = t_htp_str_formHeader( luaVM, &lB, s, 200, NULL, (int) sz, 0 );
		lua_pushvalue( luaVM, 2 );
		luaL_addvalue( &lB );
		luaL_pushresult( &lB );
		t_htp_str_addbuffer( luaVM, s, lB.n, 1 );
	}
	else
	{
		if (LUA_TSTRING == lua_type( luaVM, 2 ))
		{
			luaL_checklstring( luaVM, 2, &sz );
			if (! s->rsCl)   // chunked
			{
				luaL_buffinit( luaVM, &lB );
				b = luaL_prepbuffer( &lB );
				c = sprintf( b, "%zx\r\n", sz );
				luaL_addsize( &lB, c );
				lua_pushvalue( luaVM, 2 );
				luaL_addvalue( &lB );
				luaL_addlstring( &lB, "\r\n0\r\n\r\n", 7 );
				luaL_pushresult( &lB );
				t_htp_str_addbuffer( luaVM, s, lB.n, 1 );
			}
			else
			{
				lua_pushvalue( luaVM, 2 );
				t_htp_str_addbuffer( luaVM, s, sz, 1 );
			}
		}
		else
		{
			if (! s->rsCl)   // chunked
			{
				lua_pushstring( luaVM, "0\r\n\r\n" );
				t_htp_str_addbuffer( luaVM, s, 5, 1 );
			}
		}
	}
	/*if ( 0 == s->obc )
	{
		if ( ! s->kpAlv)
		{
			lua_pushcfunction( luaVM, lt_htp_str__gc );
			lua_pushvalue( luaVM, 1 );
			lua_call( luaVM, 1, 0 );
		}
		else
			s->state = T_htp_str_S_ZERO;
	}
	*/

	s->state = T_HTP_STR_FINISH;

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
	struct t_htp_str *s = t_htp_str_check_ud( luaVM, -2, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->pR );  // fetch the proxy table
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

	return t_push_error( luaVM, "Can't change values in `T.Http.Stream`" );
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
	struct t_htp_str *m = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Stream" );

	lua_pushfstring( luaVM, "T.Http.Stream: %p", m );
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
	struct t_htp_str *s = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Stream" );
	lua_pushinteger( luaVM, s->rqCl );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_str  The Message instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int
lt_htp_str__gc( lua_State *luaVM )
{
	struct t_htp_str *s = (struct t_htp_str *) luaL_checkudata( luaVM, 1, "T.Http.Stream" );

	if (LUA_NOREF != s->pR)
	{
		luaL_unref( luaVM, LUA_REGISTRYINDEX, s->pR );
		s->pR = LUA_NOREF;
	}

	printf( "GC'ed HTTP Stream: %p\n", s );

	return 0;
}



/**--------------------------------------------------------------------------
 * \brief      the stream library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_str_prx_s [] = {
	{"write",        lt_htp_str_write},
	{"finish",       lt_htp_str_finish},
	{"writeHead",    lt_htp_str_writeHead},
	{"onBody",       lt_htp_str_onbody},
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
	// T.Http.Stream instance metatable
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
	lua_pop( luaVM, 1 );        // remove metatable T.Http.Stream from stack

	luaL_newmetatable( luaVM, "T.Http.Stream.Proxy" );
	luaL_newlib( luaVM, t_htp_str_prx_s );
	lua_setfield( luaVM, -2, "__index" );
	lua_pop( luaVM, 1 );        // remove metatable T.Http.Stream.Proxy from stack
	return 0;
}

