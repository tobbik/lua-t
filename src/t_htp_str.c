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
 * construct an HTTP Stream
 * \param   L      Lua state.
 * \lparam  CLASS  table Http.Stream
 * \lparam  ud     T.Http.Connection userdata instance.
 * \lreturn ud     T.Http.Stream userdata instances.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_htp_str__Call( lua_State *L )
{
	struct t_htp_str *str;
	struct t_htp_con *con = t_htp_con_check_ud( L, -1, 1 );

	lua_remove( L, 1 );           // remove the CLASS table
	str = t_htp_str_create_ud( L, con );
	return 1;
}
/**--------------------------------------------------------------------------
 * create a t_htp_str and push to LuaStack.
 * \param   L  The lua state.
 *
 * \return  struct t_htp_str*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_str
*t_htp_str_create_ud( lua_State *L, struct t_htp_con *con )
{
	struct t_htp_str *s;
	s = (struct t_htp_str *) lua_newuserdata( L, sizeof( struct t_htp_str ));
	// Proxy contains lua readable items such as headers, length, status code etc
	lua_newtable( L );
	luaL_getmetatable( L, T_HTP_STR_PRX_TYPE );
	lua_setmetatable( L, -2 );
	s->pR      = luaL_ref( L, LUA_REGISTRYINDEX );
	s->rqCl    = 0;                 ///< request  content length
	s->rsCl    = 0;                 ///< response content length
	s->rsBl    = 0;                 ///< response buffer length (headers + rsCl)
	s->bR      = 0;                 ///< Lua registry reference to body handler function
	s->state   = T_HTP_STR_ZERO;    ///< shall the connection return an expected thingy?
	s->mth     = T_HTP_MTH_ILLEGAL; ///< HTTP Message state
	s->ver     = T_HTP_VER_09;      ///< HTTP Method for this request
	s->con     = con;               ///< connection

	luaL_getmetatable( L, T_HTP_STR_TYPE );
	lua_setmetatable( L, -2 );
	return s;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_str struct and return it
 * \param  L    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_str*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_str
*t_htp_str_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_HTP_STR_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_HTP_STR_TYPE"` expected." );
	return (NULL==ud) ? NULL : (struct t_htp_str *) ud;
}


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


/**--------------------------------------------------------------------------
 * Add a new buffer chunk to the Linked List buffer in t_htp_con.
 * General handling of buffers within the connection.  It does expect a Lua
 * string on top of the stack which will be wrapped into a linked list element.
 * It also expects the t_htp_str element on stack position 1. If the current
 * buffer head is null, the connections socket must also be added to the
 * EventLoop for outgoing connections.
 * \param   L        The lua state.
 * \param   integer      The string length of the chunk on stack.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_htp_str_addbuffer( lua_State *L, struct t_htp_str *s, size_t l, int last )
{
	struct t_htp_con *c = s->con;
	struct t_htp_buf *b = malloc( sizeof( struct t_htp_buf ) );

	printf( "Add Buffer: %zu bytes\n", l );
	b->bl   = l;
	b->sl   = 0;
	b->bR   = luaL_ref( L, LUA_REGISTRYINDEX );
	b->nxt  = NULL;
	b->prv  = NULL;
	lua_pushvalue( L, 1 );
	b->sR   = luaL_ref( L, LUA_REGISTRYINDEX );
	b->last = last;

	if (NULL == c->buf_head)
	{
		c->buf_head = b;
		c->buf_tail = b;
		// wrote the first line to the buffer, can also happen if
		// current buffer is flushed but response is incomplete
		t_ael_addhandle_impl( L, c->srv->ael, c->sck->fd, T_AEL_WR );
		c->srv->ael->fdSet[ c->sck->fd ]->msk = T_AEL_RW;
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
 * \param  L        The lua state.
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
t_htp_str_formHeader( lua_State *L, luaL_Buffer *lB, struct t_htp_str *s,
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
		lua_pushnil( L );
		while (lua_next( L, t ))
		{
			bs += sprintf( b,
				"%s: %s\r\n",
				lua_tostring( L, -2 ),
				lua_tostring( L, -1 )
				);
			lua_pop( L, 1 );      //FIXME:  this can't pop, it must remove
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
 * \param   L    The lua state.
 * \lparam  T.Http.Stream instance.
 * \lparam  int      HTTP status code.     // mandatory!
 * \lparam  int      length
 *           or
 * \lparam  string   HTTP message corresponding to HTTP status code.
 * \lparam  table    key:value pairs of HTTP headers.
 * \return  int    # of values pushed onto the stack.
 * ---------------------------------------------------------------------------*/
static int
lt_htp_str_writeHead( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, 1, 1 );
	int               i = lua_gettop( L );
	int               t = (LUA_TTABLE == lua_type( L, i )); // processing headers
	size_t            c = 0;
	luaL_Buffer       lB;

	luaL_buffinit( L, &lB );
	// indicate the Content-Length was provided
	if (LUA_TNUMBER == lua_type( L, 3 ) || LUA_TNUMBER == lua_type( L, 4 ))
	{
		c = t_htp_str_formHeader( L, &lB, s,
			(int) luaL_checkinteger( L, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( L, 3))   // HTTP Status message
				? lua_tostring( L, 3 )
				: t_htp_status( luaL_checkinteger( L, 2 ) ),
			(LUA_TNUMBER == lua_type( L, 3))   // Content-Length
				?  (int) luaL_checkinteger( L, 3 )
				:  (int) luaL_checkinteger( L, 4 ),
			(t) ? i : 0                            // position of optional header table on stack
			);
	}
	else     // Prepare headers for chunked encoding
	{
		c = t_htp_str_formHeader( L, &lB, s,
			(int) luaL_checkinteger( L, 2 ),   // HTTP Status code
			(LUA_TSTRING == lua_type( L, 3))   // HTTP Status message
				? lua_tostring( L, 3 )
				: t_htp_status( luaL_checkinteger( L, 2 ) ),
			0,                                     // Content-Length 0 -> chunked
			(t) ? i : 0                            // position of optional header table on stack
			);
	}
	luaL_pushresult( &lB );
	s->state = T_HTP_STR_SEND;
	t_htp_str_addbuffer( L, s, lB.n, 0 );
	return 0;
}



/**--------------------------------------------------------------------------
 * Write a response to the T.Http.Message.
 * \param   L    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str_write( lua_State *L )
{
	struct t_htp_str *s   = t_htp_str_check_ud( L, 1, 1 );
	size_t            sz;
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;
	luaL_buffinit( L, &lB );

	luaL_checklstring( L, 2, &sz );

	// this assumes first ever call is write -> chunked
	if (T_HTP_STR_SEND != s->state)
	{
		luaL_buffinit( L, &lB );
		c = t_htp_str_formHeader( L, &lB, s, 200, NULL, 0, 0 );
		b = luaL_prepbuffer( &lB );
		c = sprintf( b, "%zx\r\n", sz );
		luaL_addsize( &lB, c );
		lua_pushvalue( L, 2 );
		luaL_addvalue( &lB );
		luaL_addlstring( &lB, "\r\n", 2 );
		luaL_pushresult( &lB );
		s->state = T_HTP_STR_SEND;
	}
	else
	{
		// if the response Content-length is not known when we are sending
		// the encoding must be chunked
		if (! s->rsCl)
		{
			luaL_buffinit( L, &lB );
			b = luaL_prepbuffer( &lB );
			c = sprintf( b, "%zx\r\n", sz );
			luaL_addsize( &lB, c );
			lua_pushvalue( L, 2 );
			luaL_addvalue( &lB );
			luaL_addlstring( &lB, "\r\n", 2 );
			luaL_pushresult( &lB );
		}
		else
			lua_pushvalue( L, 2 );
	}
	// TODO: 
	t_htp_str_addbuffer( L, s, lB.n, 0 );

	return 0;
}


/**--------------------------------------------------------------------------
 * Finish of sending the T.Http.Message response.
 * \param   L    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str_finish( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, 1, 1 );
	size_t            sz;  /// length of optional string handed in
	char             *b;
	size_t            c   = 0;
	luaL_Buffer       lB;

	// the first action ever called on the stream, prep header first
	if (T_HTP_STR_SEND != s->state)
	{
		luaL_checklstring( L, 2, &sz );
		luaL_buffinit( L, &lB );
		c = t_htp_str_formHeader( L, &lB, s, 200, NULL, (int) sz, 0 );
		lua_pushvalue( L, 2 );
		luaL_addvalue( &lB );
		luaL_pushresult( &lB );
		t_htp_str_addbuffer( L, s, lB.n, 1 );
	}
	else
	{
		if (LUA_TSTRING == lua_type( L, 2 ))
		{
			luaL_checklstring( L, 2, &sz );
			if (! s->rsCl)   // chunked
			{
				luaL_buffinit( L, &lB );
				b = luaL_prepbuffer( &lB );
				c = sprintf( b, "%zx\r\n", sz );
				luaL_addsize( &lB, c );
				lua_pushvalue( L, 2 );
				luaL_addvalue( &lB );
				luaL_addlstring( &lB, "\r\n0\r\n\r\n", 7 );
				luaL_pushresult( &lB );
				t_htp_str_addbuffer( L, s, lB.n, 1 );
			}
			else
			{
				lua_pushvalue( L, 2 );
				t_htp_str_addbuffer( L, s, sz, 1 );
			}
		}
		else
		{
			if (! s->rsCl)   // chunked
			{
				lua_pushstring( L, "0\r\n\r\n" );
				t_htp_str_addbuffer( L, s, 5, 1 );
			}
		}
	}
	/*if ( 0 == s->obc )
	{
		if ( ! s->kpAlv)
		{
			lua_pushcfunction( L, lt_htp_str__gc );
			lua_pushvalue( L, 1 );
			lua_call( L, 1, 0 );
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
 * \param   L    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  function to be executed when body data arrives on connection.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str_onbody( lua_State *L )
{
	struct t_htp_str *m = t_htp_str_check_ud( L, 1, 1 );
	
	if (lua_isfunction( L, 2 ))
	{
		m->bR = luaL_ref( L, LUA_REGISTRYINDEX );
		return 0;
	}
	if (lua_isnoneornil( L, 2 ))
	{
		m->bR = LUA_NOREF;
		return 0;
	}
	else
		return luaL_error( L, "Argument must be function or nil" );
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Message by accessing proxy table.
 * \param   L    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__index( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, s->pR );  // fetch the proxy table
	lua_pushvalue( L, -2 );                      // repush the key
	lua_gettable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * update  NOT ALLOWED.
 * \param   L    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__newindex( lua_State *L )
{
	t_htp_str_check_ud( L, -3, 1 );

	return luaL_error( L, "Can't change values in `"T_HTP_STR_TYPE"`." );
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   L      The lua state.
 * \lparam  t_htp_str  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__tostring( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, 1, 1 );

	lua_pushfstring( L, T_HTP_STR_TYPE": %p", s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   L      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_str__len( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, 1, 1 );
	lua_pushinteger( L, s->rqCl );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc of a T.Http.Message instance.
 * \param   L      The lua state.
 * \lparam  t_htp_str  The Message instance user_data.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_htp_str__gc( lua_State *L )
{
	struct t_htp_str *s = t_htp_str_check_ud( L, 1, 1 );

	if (LUA_NOREF != s->pR)
	{
		luaL_unref( L, LUA_REGISTRYINDEX, s->pR );
		s->pR = LUA_NOREF;
	}

	printf( "GC'ed "T_HTP_STR_TYPE": %p\n", s );

	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_str_fm [] = {
	  { "__call",        lt_htp_str__Call }
	, { NULL,            NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_str_cf [] = {
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_str_m [] = {
	  { "__index",      lt_htp_str__index }
	, { "__newindex",   lt_htp_str__newindex }
	, { "__len",        lt_htp_str__len }
	, { "__gc",         lt_htp_str__gc }
	, { "__tostring",   lt_htp_str__tostring }
	, { NULL,    NULL }
};

/**--------------------------------------------------------------------------
 * Proxytable methods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_str_prx_s [] = {
	  { "write",        lt_htp_str_write }
	, { "finish",       lt_htp_str_finish }
	, { "writeHead",    lt_htp_str_writeHead }
	, { "onBody",       lt_htp_str_onbody }
	, { NULL,    NULL }
};



/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_htp_str( lua_State *L )
{
	// T.Http.Stream instance metatable
	luaL_newmetatable( L, T_HTP_STR_TYPE );
	luaL_setfuncs( L, t_htp_str_m, 0 );
	lua_pop( L, 1 );        // remove metatable T.Http.Stream from stack

	luaL_newmetatable( L, T_HTP_STR_PRX_TYPE );
	luaL_setfuncs( L, t_htp_str_prx_s, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Http.Stream class
	luaL_newlib( L, t_htp_str_cf );
	luaL_newlib( L, t_htp_str_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

