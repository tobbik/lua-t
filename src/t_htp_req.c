/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      src/t_htp_req.c
 * \brief     OOP for HTTP Server incoming Request
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset
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


// taken from Ryan Dahls HTTP parser
static const char tokens[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,      '!',      0,      '#',     '$',     '%',     '&',    '\'',
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        0,       0,      '*',     '+',      0,      '-',     '.',      0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
       '0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
       '8',     '9',      0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
       'x',     'y',     'z',      0,       0,       0,      '^',     '_',
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
       '`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
       'x',     'y',     'z',      0,      '|',      0,      '~',       0 };


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
 * \return  enum   How to parse the heaer value.
 * --------------------------------------------------------------------------*/
static void
t_htp_req_identifyHeader( lua_State *L, const char *k, const char *c,
                                        const char *v, const char *e )
{
	size_t   l  = c-k, lv = ('\r' == *e) ? e-v-2 : e-v-1;
	int      cl, i;     // Content-Length parsing
	char     x;
	switch (tokens[ (unsigned char) *k ])
	{
		case 'a':
			if (6 ==l) {     lua_pushstring( L, "Accept" );                     break; }
			if (13==l) {     lua_pushstring( L, "Authorization" );              break; }
			if (14==l) {     lua_pushstring( L, "Accept-Charset" );             break; }
			if (15==l) {
				x = tokens[ (unsigned char) *(k+7) ];
				if ('e'==x) { lua_pushstring( L, "Accept-Encoding"  );           break; }
				if ('l'==x) { lua_pushstring( L, "Accept-Language"  );           break; }
				if ('d'==x) { lua_pushstring( L, "Accept-Datetime" );            break; }
			}
			if (29==l) { lua_pushstring( L, "Access-Control-Request-Method" );  break; }
			if (30==l) { lua_pushstring( L, "Access-Control-Request-Headers" ); break; }
		case 'c':
			if (6 ==l) {     lua_pushstring( L, "Cookie" );                     break; }
			if (10==l)
			{
				x = tokens[ (unsigned char) *(v) ];
				switch (x)
				{
					case 'c':
						lua_pushstring( L, "keepAlive" );
						lua_pushboolean( L, 0 );
						break;
					case 'k':
						lua_pushstring( L, "keepAlive" );
						lua_pushboolean( L, 1 );
						break;
					case 't':
						lua_pushstring( L, "tls" );
						lua_pushboolean( L, 1 );
						break;
					case 'u':
						lua_pushstring( L, "upgrade" );
						lua_pushboolean( L, 1 );
						break;
					default:
						break;
				}
				lua_rawset( L, -5 );
				lua_pushstring( L, "Connection" );
				break;
			}
			if (11==l) {     lua_pushstring( L, "Content-MD5" );                break; }
			if (12==l) {     lua_pushstring( L, "Content-Type" );               break; }
			if (13==l) {     lua_pushstring( L, "Cache-Control" );              break; }
			if (14==l)
			{
				cl = 0;
				for (i=0; i < (int)lv; ++i)
					cl = cl*10 + (v[i] - '0');
				lua_pushstring( L, "contentLength" );
				lua_pushinteger( L, cl );            //S: req,hed,key,cnl
				lua_rawset( L, -5 );
				lua_pushstring( L, "Content-Length" );
				break;
			}
		case 'd':
			                 lua_pushstring( L, "Date" );                       break;
		case 'e':
			                 lua_pushstring( L, "Expect" );                     break;
		case 'f':
			if (4 ==l) {     lua_pushstring( L, "From" );                       break; }
			if (9 ==l) {     lua_pushstring( L, "Forwarded" );                  break; }
		case 'h':
			                 lua_pushstring( L, "Host" );                       break;
		case 'i':
			if (8 ==l) {
				x = tokens[ (unsigned char) *(k+3) ];
				if ('m'==x) { lua_pushstring( L, "If-Match"  );                  break; }
				if ('r'==x) { lua_pushstring( L, "If-Range"  );                  break; }
			}
			if (13==l) {     lua_pushstring( L, "If-None-Match" );              break; }
			if (17==l) {     lua_pushstring( L, "If-Modified-Since" );          break; }
			if (19==l) {     lua_pushstring( L, "If-Unmodified-Since" );        break; }
		case 'm':
			                 lua_pushstring( L, "Max-Forwards" );               break;
		case 'o':
			                 lua_pushstring( L, "Origin" );                     break;
		case 'p':
			if (6 ==l) {     lua_pushstring( L, "Pragma" );                     break; }
			if (19==l) {     lua_pushstring( L, "Proxy-Authorization" );        break; }
		case 'r':
			if (5 ==l) {     lua_pushstring( L, "Range" );                      break; }
			if (7 ==l) {     lua_pushstring( L, "Referer" );                    break; }
		case 't':
			                 lua_pushstring( L, "TE" );                         break;
		case 'u':
			if (7 ==l) {     lua_pushstring( L, "Upgrade" );                    break; }
			if (10==l) {     lua_pushstring( L, "User-Agent" );                 break; }
		case 'v':
			                 lua_pushstring( L, "Via" );                        break;
		case 'w':
			                 lua_pushstring( L, "Warning" );                    break;
		default:
			lua_pushlstring( L, k, l );
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
 * \return bool        0 means not enough data, 1 means done successfully
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
			case 'N':             m = T_HTP_MTH_NOTIFY     ;  n=6;  break;
			case 'O':             m = T_HTP_MTH_OPTIONS    ;  n=7;  break;
			case 'P':
				if ('O'==*(r+1)) { m = T_HTP_MTH_POST       ;  n=4;  break; }
				if ('U'==*(r+1)) { m = T_HTP_MTH_PUT        ;  n=3;  break; }
				if ('A'==*(r+1)) { m = T_HTP_MTH_PATCH      ;  n=5;  break; }
				if ('R'==*(r+2)) { m = T_HTP_MTH_PURGE      ;  n=5;  break; }
				if ('F'==*(r+4)) { m = T_HTP_MTH_PROPFIND   ;  n=8;  break; }
				if ('P'==*(r+4)) { m = T_HTP_MTH_PROPPATCH  ;  n=9;  break; }
			case 'R':             m = T_HTP_MTH_REPORT     ;  n=6;  break;
			case 'S':
				if ('U'==*(r+1)) { m = T_HTP_MTH_SUBSCRIBE  ;  n=9;  break; }
				if ('E'==*(r+1)) { m = T_HTP_MTH_SEARCH     ;  n=6;  break; }
			case 'T':             m = T_HTP_MTH_TRACE      ;  n=5;  break;
			case 'U':
				if ('L'==*(r+2)) { m = T_HTP_MTH_UNLOCK     ;  n=6;  break; }
				if ('L'==*(r+2)) { m = T_HTP_MTH_UNSUBSCRIBE;  n=11; break; }
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
		// HTTP/1.1rnrn
		// HTTP/1.1nn      -- play nice with naughty implementations
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
 * Stack: requesttable
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

	lua_getfield( L, 1, "headers" ); // get pre-existing header table -> re-entrent

	// since exit condition is based on r+1 compare for (r+1)
	while (r+1 <= end)
	{
		switch (*r)
		{
			case '\n':
				if (' ' == *(r+1)) // Value Continuation
					break;
				if (T_HTP_R_KY == rs)
				{
					lua_pushlstring( L, k, r-k-1 );            // didn't find colon; push entire line
					lua_rawseti( L, -2, lua_rawlen( L, -2 ) ); // push entire line as enumerated value
				}
				else
					t_htp_req_identifyHeader( L, k, c, v, r );
				rs = T_HTP_R_KY;
				if ('\n' == *(r+1) || '\r' == *(r+1))         // double newLine -> END OF HEADER
				{
					(*data) = r + (('\n'==*(r+1))? 1 : 3);
					lua_pop( L, 1 );  // pop header-table from stack
					lua_pushstring( L, "contentLength" );
					lua_rawget( L, 1 );
					if (lua_isnil( L, -1 ) || (lua_isinteger( L, -1 ) && 0 == lua_tointeger( L, -1 )))
						lua_pushinteger( L, T_HTP_REQ_DONE );
					else
						lua_pushinteger( L, T_HTP_REQ_BODY );
					lua_remove( L, -2 );
					lua_setfield( L, 1, "state" );
					return 1;
				}
				k = r+1;
				break;
			case  ':':
				if (T_HTP_R_KY == rs)
				{
					c  = r;
					r  = eat_lws( ++r );
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
	const char *data  = luaL_checklstring( L, 2, &d_len );
	const char  *end  = data + d_len-1; // marks the last character
	const char **tail = &data;
	size_t      state = (size_t) luaL_checkinteger( L, 3 );
	lua_pop( L, 1 );  // pop state

	switch (state)
	{
		case T_HTP_REQ_METHOD:
			//printf( "Parsing METHOD\n" );
			if (0 == t_htp_req_parseMethod( L, tail, end ))
				break;
		case T_HTP_REQ_URI:
			//printf( "Parsing URL\n" );
			if (0 == t_htp_req_parseUrl( L, tail, end ))
				break;
		case T_HTP_REQ_VERSION:
			//printf( "Parsing VERSION\n" );
			if (0 == t_htp_req_parseHttpVersion( L, tail, end ))
				break;
		case T_HTP_REQ_HEADERS:
			//printf( "Parsing HEADERS\n" );
			if (0 == t_htp_req_parseHeaders( L, tail, end ))
				break;
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
LUAMOD_API int
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


