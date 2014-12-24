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

const char *eat_lws( const char *s )
{
	while( ' ' == *s ||  '\r' == *s ||   '\n' == *s)
		s++;
	return s;
};


static inline char
*t_htp_msg_pMethod( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	size_t      i;

	d = strchr( b, ' ' );
	i = d-b;
	if (i>13)
		luaL_error( luaVM, "Illegal HTTP header: HTTP Method is too long" );
	switch (*b)
	{
		case 'C': {
			if (7==i && 'O'==*(b+1))
				m->mth=T_HTP_MTH_CONNECT;
			else if (8==i && 'H'==*(b+1))
				m->mth=T_HTP_MTH_CHECKOUT;
			else if (4==i && 'O'==*(b+1))
				m->mth=T_HTP_MTH_COPY;
			else
				luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
			break;
		}
		case 'D': m->mth=T_HTP_MTH_DELETE; break;
		case 'G': m->mth=T_HTP_MTH_GET; break;
		case 'H': m->mth=T_HTP_MTH_HEAD; break;
		case 'L': m->mth=T_HTP_MTH_LOCK; break;
		case 'M': {
			if (5==i && 'K'==*(b+1))
				m->mth=T_HTP_MTH_MKCOL;
			else if (10==i && 'K'==*(b+1))
				m->mth=T_HTP_MTH_MKACTIVITY;
			else if (10==i && 'C'==*(b+2))
				m->mth=T_HTP_MTH_MKCALENDAR;
			else if (8==i && '-'==*(b+1))
				m->mth=T_HTP_MTH_MSEARCH;
			else if (5==i && 'E'==*(b+1))
				m->mth=T_HTP_MTH_MERGE;
			else if (4==i && 'O'==*(b+1))
				m->mth=T_HTP_MTH_MOVE;
			else
				luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
			break;
		}
		case 'N': m->mth=T_HTP_MTH_NOTIFY; break;
		case 'O': m->mth=T_HTP_MTH_OPTIONS; break;
		case 'P': {
			if (4==i && 'O'==*(b+1))
				m->mth=T_HTP_MTH_POST;
			else if (3==i && 'U'==*(b+1))
				m->mth=T_HTP_MTH_PUT;
			else if (5==i && 'A'==*(b+1))
				m->mth=T_HTP_MTH_PATCH;
			else if (5==i && 'U'==*(b+1))
				m->mth=T_HTP_MTH_PURGE;
			else if (8==i && 'R'==*(b+1))
				m->mth=T_HTP_MTH_PROPFIND;
			else if (9==i && 'R'==*(b+1))
				m->mth=T_HTP_MTH_PROPPATCH;
			else
				luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
			break;
		}
		case 'R': m->mth=T_HTP_MTH_REPORT; break;
		case 'S': {
			if (9==i && 'U'==*(b+1))
				m->mth=T_HTP_MTH_SUBSCRIBE;
			else if (6==i && 'E'==*(b+1))
				m->mth=T_HTP_MTH_SEARCH;
			else
				luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
			break;
		}
		case 'T': m->mth=T_HTP_MTH_TRACE; break;
		case 'U': {
			if (6==i && 'N'==*(b+1))
				m->mth=T_HTP_MTH_UNLOCK;
			else if (11==i && 'N'==*(b+1))
				m->mth=T_HTP_MTH_UNSUBSCRIBE;
			else
				luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
			break;
		}
		default:
			luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
	}
	lua_pushstring( luaVM, "method" );
	lua_pushlstring( luaVM, b, d-b );
	lua_rawset( luaVM, -3 );
	d = eat_lws( d );
	m->bRead += d-b;
	m->pS     = T_HTP_STA_URL;
	return (char *) d;
}


static inline char
*t_htp_msg_pUrl( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	size_t      i;

	d = strchr( b, ' ' );
	i = d-b;
	if (i>2000)
		luaL_error( luaVM, "illegal URL" );
	lua_pushstring( luaVM, "url" );
	lua_pushlstring( luaVM, b, d-b );
	lua_rawset( luaVM, -3 );
	d = eat_lws( d );
	m->bRead += d-b;
	m->pS     = T_HTP_STA_VERSION;
	return (char *) d;
}


static inline char
*t_htp_msg_pVersion( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	size_t      i;

	d = strchr( b, '\r' );
	i = d-b;
	if (8 != i)
		luaL_error( luaVM, "ILLEGAL HTTP version in HTTP message" );
	switch (*(b+7))
	{
		case '1': m->ver=T_HTP_VER_11; break;
		case '0': m->ver=T_HTP_VER_10; break;
		case '9': m->ver=T_HTP_VER_09; break;
		default: luaL_error( luaVM, "ILLEGAL HTTP version in message" ); break;
	}
	lua_pushstring( luaVM, "version" );
	lua_pushlstring( luaVM, b, d-b );
	lua_rawset( luaVM, -3 );
	d = eat_lws( d );
	m->bRead += d-b;
	m->pS     = T_HTP_STA_HEADER;
	return (char *) d;
}


static inline char
*t_htp_msg_pHeader( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *v  = b;
	const char *k  = b;
	const char *ke = b;
	const char *r  = b;
	size_t      run= 200;
	size_t      is_key = 1;

	lua_pushstring( luaVM, "header" );
	lua_rawget( luaVM, -2 );
	if (lua_isnil(luaVM, -1))                       //S:P,h/nil
	{
		lua_newtable( luaVM );                       //S:P,nil,h
		lua_replace( luaVM, -2);                     //S:P,h
		lua_pushstring( luaVM, "header" );           //S:P,h,"header"
		lua_pushvalue( luaVM, -2 );                  //S:P,h,"header",h
		lua_rawset( luaVM, -4 );                     //S:P,h
	}

	while (run>0)
	{
		//printf("%c   %c   %c   %c   %zu\n", *r, *k, *ke, *v, run-- );
		switch (*r)
		{
			case '\0':
				run=0; break;
			case ':':
				if (is_key)
				{
					ke = r;
					r++;
					r = eat_lws( r );
					v  = r;
					is_key = 0;
				}
				else
					r++;
				break;
			case '\r':
			case '\n':
				if (' ' == *(r+1))
				{
					r++;
				}
				else
				{
					//printf("Got an R   %lu    %lu \n",    ke-k, r-v);
					lua_pushlstring( luaVM, k, ke-k );   // push key
					lua_pushlstring( luaVM, v, r -v );   // push value
					//t_stackDump( luaVM );
					lua_rawset( luaVM, -3 );
					if (0==m->sz && memcmp( k,"Content-Length", ke-k ))
						m->sz = (size_t) atoi( v );
					else
						m->sz = 0;
					while( '\r' == *r ||   '\n' == *r) r++;
					k=r;
					is_key=1;
					m->bRead += k-b;
				}
				break;
			default: r++; break;
		}
	}

	m->pS = (0 == m->sz) ? T_HTP_STA_NOBODY : T_HTP_STA_BODY;
	lua_pop( luaVM, 1 );   // pop the header table
	return (char *) r;
}


static int
t_htp_msg_parse( lua_State *luaVM, struct t_htp_msg *m, const char *buf )
{
	char *nxt = (char *) buf;

	while (NULL != nxt)
	{
		switch(m->pS)
		{
			case T_HTP_STA_ZERO:
				nxt = t_htp_msg_pMethod( luaVM, m, nxt );
				break;
			case T_HTP_STA_URL:
				nxt = t_htp_msg_pUrl( luaVM, m, nxt );
				break;
			case T_HTP_STA_VERSION:
				nxt = t_htp_msg_pVersion( luaVM, m, nxt );
				break;
			case T_HTP_STA_HEADER:
				nxt = t_htp_msg_pHeader( luaVM, m, nxt );
				break;
			case T_HTP_STA_BODY:
			case T_HTP_STA_NOBODY:
				return 1;
			default:
				luaL_error( luaVM, "Illegal state for T.Http.Message" );
		}
	}
	return (NULL == nxt) ? 0 : 1;
}


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
	m->bRead = 0;
	m->pS    = T_HTP_STA_ZERO;
	m->srv   = srv;
	m->sz    = 0;

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
	struct t_htp_msg *c = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

	lua_pushfstring( luaVM, "T.Http.Message: %p", c );
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
	//struct t_wsk *wsk = t_wsk_check_ud( luaVM, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( luaVM, 5 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Message socket.
 * Called if socket comes back from accept and anytime it returns from read.
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
	struct t_elp     *elp;
	int               rcvd;

	// get the proxy on the stack
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR ); //S:m,P

	// read
	rcvd = t_sck_recv_tcp( luaVM, m->sck, &(m->buf[ m->bRead ]), BUFSIZ - m->bRead );
	printf( "%d   %s\n", rcvd, &(m->buf[ m->bRead ]) );

	t_htp_msg_parse( luaVM, m, &(m->buf[ m->bRead ]) );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->rR );    // get function from msg
	lua_pushvalue( luaVM, 1 );
	lua_call( luaVM, 1,0 );


	// TODO: depending on T_HTP_STA state / parse body or deal with incoming data
	if (m->pS > T_HTP_STA_HEADER)
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->lR );
		elp = t_elp_check_ud( luaVM, -1, 1);
		t_elp_removehandle_impl( elp, m->sck->fd );
		t_elp_addhandle_impl( elp, m->sck->fd, 0 );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ m->sck->fd ]->fR );
		lua_createtable( luaVM, 2, 0 );  // create function/parameter table
		lua_pushcfunction( luaVM, t_htp_msg_rsp );
		lua_rawseti( luaVM, -2, 1 );
		lua_pushvalue( luaVM, 1 );
		lua_rawseti( luaVM, -2, 2 );
		elp->fd_set[ m->sck->fd ]->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
		lua_pop( luaVM, 1 );             // pop the event loop
	}

	lua_pop( luaVM, 1 );             // pop the proxy table
	return rcvd;
}


/**--------------------------------------------------------------------------
 * Handle outgoing message to T.Http.Message socket.
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
	struct t_elp     *elp;

	m->sent += t_sck_send_tcp( luaVM, m->sck, &(m->buf[ m->sent ]), m->bRead );

	printf("RESPONSE:  %zu    %zu      %s\n", m->bRead, m->sent,  m->buf);
	//if (T_HTP_STA_DONE == m->pS)
	if (m->sent >= m->bRead)
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->lR );
		elp = t_elp_check_ud( luaVM, -1, 1);
		luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ m->sck->fd ]->fR );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ m->sck->fd ]->hR );
		free( elp->fd_set[ m->sck->fd ] );
		elp->fd_set[ m->sck->fd ] = NULL;
		t_sck_close( luaVM, m->sck );
	}

	return 1;
}



static void
t_htp_msg_prepresp( struct t_htp_msg *m )
{
	t_htp_srv_setnow( m->srv, 0 );            // update server time
	m->bRead = (size_t) snprintf(m->buf, BUFSIZ,
		"HTTP1.1 200 OK\r\n"
		"Date: %s\r\n\r\n",
			m->srv->fnow
	);
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
	size_t            s;
	const char       *v = luaL_checklstring( luaVM, 2, &s );

	if (T_HTP_STA_BODY != m->pS)
	{
		memset( &(m->buf[0]), 0, BUFSIZ );
		m->sent  =0;
		t_htp_msg_prepresp( m );
	}
	memcpy( &(m->buf[ m->bRead ]), v, s );
	m->bRead += s;

	return 0;
}


/**--------------------------------------------------------------------------
 * Send the T.Http.Message.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  string.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_msg_finish( lua_State *luaVM )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, 1, 1 );
	m->pS = T_HTP_STA_DONE;

	return 1;
}


/**--------------------------------------------------------------------------
 * Access Field Values in T.Http.Message by accessing proxy table.
 * \param   luaVM    The lua state.
 * \lparam  Http.Message instance.
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static inline int
t_htp_msg_get( lua_State *luaVM, char *val )
{
	struct t_htp_msg *m = t_htp_msg_check_ud( luaVM, -1, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // fetch the proxy table
	lua_pushstring( luaVM, val );                    // repush the key
	lua_rawget( luaVM, -2 );

	return 1;
}

static int lt_htp_msg_geturl   ( lua_State *luaVM ) { return t_htp_msg_get( luaVM, "url" ); }
static int lt_htp_msg_getheader( lua_State *luaVM ) { return t_htp_msg_get( luaVM, "header" ); }
static int lt_htp_msg_getsocket( lua_State *luaVM ) { return t_htp_msg_get( luaVM, "socket" ); }
static int lt_htp_msg_getip    ( lua_State *luaVM ) { return t_htp_msg_get( luaVM, "ip" ); }


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
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_msg_m [] = {
	{"getHeader",     lt_htp_msg_getheader},
	{"getUrl",        lt_htp_msg_geturl},
	{"getSocket",     lt_htp_msg_getsocket},
	{"getIp",         lt_htp_msg_getip},
	{"write",         lt_htp_msg_write},
	{"finish",        lt_htp_msg_finish},
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
	luaL_newlib( luaVM, t_htp_msg_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_msg__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lt_htp_msg__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_msg__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}

