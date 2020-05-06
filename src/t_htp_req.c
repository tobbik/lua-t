/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      src/t_htp_req.c
 * \brief     OOP for HTTP Server incoming Request
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset
#include <ctype.h>                // tolower
#include <time.h>                 // gmtime

#include "t_htp_l.h"
#include "t_buf.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/// State of reading HTTP Headers
enum t_htp_rs {
	T_HTP_R_KY,         ///< Read Key
	T_HTP_R_VL,         ///< Read value
};

/**--------------------------------------------------------------------------
 * Read registered Request headers. Standardize Casing.
 * Parses values of connection relevant headers, such as:
 *   - Content-Length
 *   - Connection (close/keepalive/upgrade)
 *   - Upgrade (WebSocket, ... )
 * \param   char*  k Header key start.
 * \param   char*  c Colon after the header key (':').
 * \param   char*  v Header value start.
 * \param   char*  e Header value end ('\r' or '\n' ... sloppy bastards!).
 * \return  void.
 * --------------------------------------------------------------------------*/
static void
t_htp_req_handleHeader( lua_State *L,       char *k, const char *c,
                                      const char *v, const char *e )
{
	size_t   lk = c-k,
	         lv = ('\r' == *e) ? e-v-2 : e-v-1;
	size_t   i, cl;   // Content-Length parsing

	// lowercase the key; push header-key for header table
	for (i=0; i < lk; ++i)
		k[i] = tolower( k[i] );
	lua_pushlstring( L, k, lk );

	switch (*k)
	{
		case 'e':
			lua_pushstring( L, "expect" );
			lua_pushboolean( L, 1 );
			lua_rawset( L, -6 );                    // set value on the `request` object
			break;
		case 'c':
			if (14==lk)     // content-length
			{
				cl = 0;
				for (i=0; i < lv; ++i)
					cl = cl*10 + (v[i] - '0');
				lua_pushstring( L, "contentLength" );
				lua_pushinteger( L, cl );            //S: req str hed key cnl
				lua_rawset( L, -6 );                 // set request.contentLength
			}
			if (10==lk)   //connection
			{
				switch ( tolower( *v ) )
				{
					case 'c':  lua_pushstring( L, "keepAlive" ); lua_pushboolean( L, 0 ); break;
					case 'k':  lua_pushstring( L, "keepAlive" ); lua_pushboolean( L, 1 ); break;
					case 't':  lua_pushstring( L, "tls" );       lua_pushboolean( L, 1 ); break;
					case 'u':  lua_pushstring( L, "upgrade" );   lua_pushboolean( L, 1 ); break;
					default:                                               break;
				}
				lua_rawset( L, -6 );    // set value on the `request` object
			}
			break;  // break 'c'
		default:
			break;
	}
	lua_pushlstring( L, v, lv );   // push value
	lua_rawset( L, -3 );
}


/**
 * Eat Linear White Space
 */
static inline const char
*eat_lws( const char *s )
{
	while (' ' == *s || '\r' == *s || '\n' == *s)
		s++;
	return s;
}


/**--------------------------------------------------------------------------
 * Parse Method from the request.
 * Stack: requesttable
 * \param  lua_State   L.
 * \param  char **data pointer within to data stream.
 * \param  char *end   end of data strem.
 * \return n    position in parse stream after method
 * --------------------------------------------------------------------------*/
static size_t
t_htp_req_parseMethod( lua_State *L, const char **data, const char *end )
{
	size_t      n = 0;                 ///< offset of whitespace after method
	int         m = T_HTP_MTH_ILLEGAL; ///< HTTP.Method index
	const char *r = *data;             ///< runner char

	// Determine HTTP Verb (METHOD)
	if ((end - *data) > 10)
	{
		switch (*r)
		{
			case 'C':
				if ('N'==*(r+2)) { m = T_HTP_MTH_CONNECT    ;  n=7;  break; }
				if ('E'==*(r+2)) { m = T_HTP_MTH_CHECKOUT   ;  n=8;  break; }
				if ('P'==*(r+2)) { m = T_HTP_MTH_COPY       ;  n=4;  break; }
				break;
			case 'D':             m = T_HTP_MTH_DELETE     ;  n=6;  break;
			case 'G':             m = T_HTP_MTH_GET        ;  n=3;  break;
			case 'H':             m = T_HTP_MTH_HEAD       ;  n=4;  break;
			case 'L':             m = T_HTP_MTH_LOCK       ;  n=4;  break;
			case 'M':
				if ('O'==*(r+3)) { m = T_HTP_MTH_MKCOL      ;  n=5;  break; }
				if ('A'==*(r+2)) { m = T_HTP_MTH_MKACTIVITY ;  n=10; break; }
				if ('A'==*(r+3)) { m = T_HTP_MTH_MKCALENDAR ;  n=10; break; }
				if ('-'==*(r+1)) { m = T_HTP_MTH_MSEARCH    ;  n=8;  break; }
				if ('E'==*(r+1)) { m = T_HTP_MTH_MERGE      ;  n=5;  break; }
				if ('O'==*(r+1)) { m = T_HTP_MTH_MOVE       ;  n=4;  break; }
				break;
			case 'N':             m = T_HTP_MTH_NOTIFY     ;  n=6;  break;
			case 'O':             m = T_HTP_MTH_OPTIONS    ;  n=7;  break;
			case 'P':
				if ('O'==*(r+1)) { m = T_HTP_MTH_POST       ;  n=4;  break; }
				if ('U'==*(r+1)) { m = T_HTP_MTH_PUT        ;  n=3;  break; }
				if ('A'==*(r+1)) { m = T_HTP_MTH_PATCH      ;  n=5;  break; }
				if ('R'==*(r+2)) { m = T_HTP_MTH_PURGE      ;  n=5;  break; }
				if ('F'==*(r+4)) { m = T_HTP_MTH_PROPFIND   ;  n=8;  break; }
				if ('P'==*(r+4)) { m = T_HTP_MTH_PROPPATCH  ;  n=9;  break; }
				break;
			case 'R':             m = T_HTP_MTH_REPORT     ;  n=6;  break;
			case 'S':
				if ('U'==*(r+1)) { m = T_HTP_MTH_SUBSCRIBE  ;  n=9;  break; }
				if ('E'==*(r+1)) { m = T_HTP_MTH_SEARCH     ;  n=6;  break; }
				break;
			case 'T':             m = T_HTP_MTH_TRACE      ;  n=5;  break;
			case 'U':
				if ('L'==*(r+2)) { m = T_HTP_MTH_UNLOCK     ;  n=6;  break; }
				if ('L'==*(r+2)) { m = T_HTP_MTH_UNSUBSCRIBE;  n=11; break; }
				break;
			default:
				luaL_error( L, "Illegal HTTP header: Unknown HTTP Method" );
				break;
		}
		lua_pushinteger( L, m );
		lua_setfield( L, 1, "method" );
		lua_pushinteger( L, T_HTP_REQ_URI );
		lua_setfield( L, 1, "state" );
		(*data) += n;
	}
	return n;
}


/**--------------------------------------------------------------------------
 * Parse Uri and Query from the request.
 * Stack: requesttable
 * \param  lua_State   L.
 * \param  char **data pointer within to data stream.
 * \param  char *end   end of data strem.
 * \return bool        0 means not enough data, 1 means done successfully
 * --------------------------------------------------------------------------*/
static size_t
t_htp_req_parseUrl( lua_State *L, const char **data, const char *end )
{
	const char *r = eat_lws( *data );  ///< runner char
	const char *u = r;                 ///< start of URI
	const char *q = NULL;              ///< runner for query
	const char *v = r;                 ///< value start marker

	lua_newtable( L );                 ///< parsed and decoded query parameters

	while (r <= end)
	{
		switch (*r)
		{
			case '/':
				break;
			case '?':
				lua_pushlstring( L, u, r-u );     // push path
				lua_setfield( L, 1, "path" );
				q = r+1;
				break;
			case '=':
				lua_pushlstring( L, q, r-q );     // push key
				//TODO: if key exists, create table
				v = r+1;
				break;
			case '&':
				lua_pushlstring( L, v, r-v );     // push value
				lua_rawset( L, -3 );
				q = r+1;
				break;
			case ' ':                            // last value
				lua_pushlstring( L, u, r-u );     // push full uri string
				lua_setfield( L, 1, "url" );
				if (NULL != q)
				{
					lua_pushlstring( L, v, r-v );  // push value
					lua_rawset( L, -3 );
					lua_setfield( L, 1, "query" );
				}
				else
				{
					lua_pop( L, 1 );               // pop empty table
					lua_pushlstring( L, u, r-u );  // push full uri string as path
					lua_setfield( L, 1, "path" );
				}

				lua_pushinteger( L, T_HTP_REQ_VERSION );
				lua_setfield( L, 1, "state" );
				(*data) = r;
				return r - u + 1;  //+1 on for space before URL
				break;
			default:           break;
		}
		r++;
	}
	lua_pop( L, lua_gettop( L ) - 3 );         // pop empty table and potentially pushed keys
	return 0;
}


/**--------------------------------------------------------------------------
 * Parse HTTP Version from the request.
 * Stack: requesttable
 * \param  lua_State   L.
 * \param  char **data pointer within to data stream.
 * \param  char *end   end of data strem.
 * \return bool        0 means not enough data, 1 means done successfully
 * --------------------------------------------------------------------------*/
static int
t_htp_req_parseHttpVersion( lua_State *L, const char **data, const char *end )
{
	const char *r = eat_lws( *data );  ///< runner char
	int         v = T_HTP_VER_ILL;
	size_t      l = end - r;

	if ((l > 10 && '\r'==*(r+8 )) || (l > 8 && '\n'==*(r+8)))
	{
		switch (*(r+7))
		{
			case '1': v = T_HTP_VER_11; break;
			case '0': v = T_HTP_VER_10; break;
			case '9': v = T_HTP_VER_09; break;
			default: luaL_error( L, "ILLEGAL HTTP version in message" ); break;
		}
		if (v < T_HTP_VER_11)
		{
			lua_pushboolean( L, 0 );
			lua_setfield( L, 1, "keepAlive" );
		}
		lua_pushinteger( L, v );
		lua_setfield( L, 1, "version" );
		// HTTP/1.1/r/n/r/n
		// HTTP/1.1/n/n      -- play nice with sloppy implementations
		// 01234567890
		lua_pushinteger( L, ((l>10 && '\r'==*(r+10)) || ('\n'==*(r+8) && '\n'==*(r+9)))
			? T_HTP_REQ_DONE
			: T_HTP_REQ_HEADERS );
		lua_setfield( L, 1, "state" );
		(*data) = r + 8;  // relocate to \r or\n after first line

		return 8;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Parse HTTP Headers from the request.
 * Stack: requesttable data
 * \param  lua_State   L.
 * \param  char **data pointer within to data stream.
 * \param  char *end   end of data strem.
 * \return bool        0 means not enough data, 1 means done successfully
 * --------------------------------------------------------------------------*/
static int
t_htp_req_parseHeaders( lua_State *L, const char **data, const char *end )
{
	const char    *r  = eat_lws( *data );  ///< runner char
	const char    *k  = r;                 ///< marks start of key
	const char    *c  = r;                 ///< marks colon after key
	const char    *v  = r;                 ///< marks start of value
	enum t_htp_rs  rs = T_HTP_R_KY;        ///< local parse state = New Line Beginning

	lua_getfield( L, 1, "headers" );       // get pre-existing header table -> re-entrent

	// since exit condition is based on r+1 compare for (r+1)
	while (r+1 <= end)
	{
		switch (*r)
		{
			case '\n':
				if (' ' == *(r+1))                            // Value Continuation
					break;
				if (T_HTP_R_KY == rs)
				{
					lua_pushlstring( L, k, r-k-1 );            // didn't find colon; push entire line
					lua_rawseti( L, -2, lua_rawlen( L, -2 ) ); // push entire line as enumerated value
				}
				else
					t_htp_req_handleHeader( L, (char *) k, c, v, r );
				rs = T_HTP_R_KY;
				if ('\n' == *(r+1) || '\r' == *(r+1))         // double newLine -> END OF HEADER
				{
					(*data) = r + (('\n'==*(r+1))? 1 : 3);
					lua_pushstring( L, "content-length" );
					lua_rawget( L, -2 );
					lua_pushstring( L, "state" );              //S: req hdr cl "state"
					if (lua_isnil( L, -2 ) || (lua_isinteger( L, -2 ) && 0 == lua_tointeger( L, -2 )))
						lua_pushinteger( L, T_HTP_REQ_DONE );
					else
						lua_pushinteger( L, T_HTP_REQ_BODY );   //S: req hdr cl "state" ste
					lua_rawset( L, 1 );                        //S: req hdr cl
					lua_pop( L, 2 );                           //S: req
					return 1;
				}
				k = r+1;
				break;
			case  ':':
				if (T_HTP_R_KY == rs)
				{
					c  = r;
					r  = (char *) eat_lws( ++r );
					v  = r;
					rs = T_HTP_R_VL;
				}
				break;
			default:
				break;
		}
		r++;
	}
	lua_pop( L, 1 );  // pop header-table from stack
	return 0;
}


/**--------------------------------------------------------------------------
 * Receive message
 * This is optimistic.  If a big chunk got received don't waste time moving
 * everyting between Lua and C.  Just keep parsing and fill the T.Http.Request
 * object.  Otherwise, preserve the state, store unparsed string in Lua and
 * re-parse upon next incoming data.
 * \param   L      Lua state.
 * \lparam  table  t.Http.Request userdata.
 * \lparam  string Lua string of received data.
 * \lparam  status current parsing status.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_req_parse( lua_State *L )
{
	size_t      d_len;
	const char  *data = luaL_checklstring( L, 2, &d_len );
	const char   *end = data + d_len-1; // marks the last character
	const char **tail = &data;
	size_t      state = (size_t) luaL_checkinteger( L, 3 );
	lua_pop( L, 1 );  // pop state

	switch (state)
	{
		case T_HTP_REQ_METHOD:
			//printf( "Parsing METHOD\n" );
			if (0 == t_htp_req_parseMethod( L, tail, end ))
				break;
			/* FALLTHRU */
		case T_HTP_REQ_URI:
			//printf( "Parsing URL\n" );
			if (0 == t_htp_req_parseUrl( L, tail, end ))
				break;
			/* FALLTHRU */
		case T_HTP_REQ_VERSION:
			//printf( "Parsing VERSION\n" );
			if (0 == t_htp_req_parseHttpVersion( L, tail, end ))
				break;
			/* FALLTHRU */
		case T_HTP_REQ_HEADERS:
			//printf( "Parsing HEADERS\n" );
			if (0 == t_htp_req_parseHeaders( L, tail, end ))
				break;
			/* FALLTHRU */
		default:
			break;
	}
	if (*tail == end)
		lua_pushnil( L );
	else
		lua_pushlstring( L, *tail, end - (*tail) + 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
//static const struct luaL_Reg t_htp_req_fm [] = {
//	  { NULL           , NULL }
//};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
//static const struct luaL_Reg t_htp_req_cf [] = {
//	  { NULL           , NULL }
//};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_req_m [] = {
	  { "parse"      , lt_htp_req_parse }
	, { NULL           , NULL }
};


/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_htp_req( lua_State *L )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( L, T_HTP_REQ_TYPE );
	luaL_setfuncs( L, t_htp_req_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Http.Request class
	//luaL_newlib( L, t_htp_req_cf );
	//luaL_newlib( L, t_htp_req_fm );
	//lua_setmetatable( L, -2 );
	//return 1;
	return 0;
}


