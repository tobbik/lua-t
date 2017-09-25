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
 * Adjust start point of Buffer.Segment
 * This is a dangerous function which doesn't check bounderies!
 * \param   L                Lua state.
 * \param   struct t_buf_seg seg.
 * \param   mv               move start point mv bytes to the right.
 * --------------------------------------------------------------------------*/
static void
t_buf_seg_moveIndex( struct t_buf_seg *seg, int mv )
{
	//printf( "%d  ->  %ld(%ld)   %ld(%ld)\n", mv, seg->idx, seg->idx+mv, seg->len, seg->len-mv );
	seg->b   += mv;
	seg->idx += mv;
	seg->len -= mv;
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
 * \param  L                  the Lua State
 * \param  struct t_buf_seg*  pointer to t_buf_seg.
 * \return n                  buffer offset after parsing. 0 means fail.
 * --------------------------------------------------------------------------*/
static size_t
t_htp_req_parseMethod( lua_State *L, struct t_buf_seg *seg )
{
	size_t      n = 0;                 ///< offset of whitespace after method
	int         m = T_HTP_MTH_ILLEGAL; ///< HTTP.Method index
	const char *r = seg->b;            ///< runner char

	// Determine HTTP Verb
	if (seg->len > 10)
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
		t_buf_seg_moveIndex( seg, n );
	}
	return n;
}


/**--------------------------------------------------------------------------
 * Parse Uri and Query from the request.
 * Stack: requesttable
 * \param  L                  the Lua State
 * \param  struct t_buf_seg*  pointer to t_buf_seg.
 * \return n                  buffer offset after parsing. 0 means fail.
 * --------------------------------------------------------------------------*/
static size_t
t_htp_req_parseUrl( lua_State *L, struct t_buf_seg *seg )
{
	const char *r = eat_lws( seg->b );  ///< runner char
	const char *e = seg->b + seg->len;  ///< ending char
	const char *u = r;                  ///< start of URI
	const char *q = NULL;               ///< runner for query
	const char *v = r;                  ///< value start marker

	lua_newtable( L );                  ///< parsed and decoded query parameters

	while (r < e)
	{
		switch (*r)
		{
			case '/':
				break;
			case '?':
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
				lua_pushlstring( L, u, r-u );     // push full query string
				lua_setfield( L, 1, "url" );
				if (NULL != q)
				{
					lua_pushlstring( L, v, r-v );  // push value
					lua_rawset( L, -3 );
					lua_setfield( L, 1, "query" );
				}
				else
					lua_pop( L, 1 );               // pop empty table

				lua_pushinteger( L, T_HTP_REQ_VERSION );
				lua_setfield( L, 1, "state" );
				t_buf_seg_moveIndex( seg, r - seg->b );
				return r - u + 1;  //+1 on for spece before URL
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
 * \param  L                  the Lua State
 * \param  struct t_buf_seg*  pointer to t_buf_seg.
 * \return n                  buffer offset after parsing. 0 means fail.
 * --------------------------------------------------------------------------*/
static int
t_htp_req_parseHttpVersion( lua_State *L, struct t_buf_seg *seg )
{
	const char       *r   = eat_lws( seg->b );  ///< runner char
	int               v   = T_HTP_VER_ILL;

	//TODO: set values based on version default behaviour (eg, KeepAlive for 1.1 etc)
	//TODO: check for n being big enough
	if ('H'==*(r) && ('\r'==*(r+8 ) || '\n'==*(r+8)))
	{
		switch (*(r+7))
		{
			case '1': v = T_HTP_VER_11; break;
			case '0': v = T_HTP_VER_10; break;
			case '9': v = T_HTP_VER_09; break;
			default: luaL_error( L, "ILLEGAL HTTP version in message" ); break;
		}
		lua_pushinteger( L, v );
		if (v == T_HTP_VER_11)
		{
			lua_pushboolean( L, 1 );
			lua_setfield( L, 1, "keepAlive" );
		}
		lua_pushinteger( L, v );
		lua_setfield( L, 1, "version" );
		lua_pushinteger( L, T_HTP_REQ_HEADERS );
		lua_setfield( L, 1, "state" );
		t_buf_seg_moveIndex( seg, r - seg->b + 8 );  // relocate to \r or\n after first line

		return 8;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Parse HTTP Headers from the request.
 * Stack: requesttable
 * \param  L                  the Lua State
 * \param  struct t_buf_seg*  pointer to t_buf_seg.
 * \return n                  buffer offset after parsing. 0 means fail.
 * --------------------------------------------------------------------------*/
static int
t_htp_req_parseHeaders( lua_State *L, struct t_buf_seg *seg )
{
	const char       *r   = eat_lws( seg->b );  ///< runner char
	const char       *e   = seg->b + seg->len;  ///< ending char
	const char       *k   = r;                  ///< marks start of key
	const char       *c   = r;                  ///< marks colon after key
	const char       *v   = r;                  ///< marks start of value
	const char       *s   = r;                  ///< marks start of string
	enum t_htp_rs     rs  = T_HTP_R_KY;         ///< local parse state = New Line Beginning

	lua_getfield( L, 1, "headers" ); // get pre-existing header table -> re-entrent

	// since exit condition is based on r+1 compare for (r+1)
	while (r+1 < e)
	{
		switch (*r)
		{
			case '\n':
				if (' ' == *(r+1)) // Value Continuation
					break;
				if (T_HTP_R_KY == rs)
				{
					lua_pushlstring( L, k, r-k-1 );   // didn't find colon; push entire line
					lua_rawseti( L, -2, lua_rawlen( L, -2 ) ); // push "key" as enumerated value
				}
				else
				{
					lua_pushlstring( L, k, c-k );   // push real key
					lua_pushlstring( L, v, ('\r' == *(r-1)) ? r-v-1 : r-v );   // push value
				}
				rs = T_HTP_R_KY;
				if ('\n' == *(r+1) || '\r' == *(r+1))  // double newLine -> END OF HEADER
				{
					t_buf_seg_moveIndex( seg, (r + (('\n'==*(r+1))? 0 : 1) - seg->b) );
					lua_pop( L, 1 );  // pop header-table from stack
					lua_pushinteger( L, T_HTP_REQ_BODY );
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
 * object.
 * \param   L      Lua state.
 * \lparam  table  t.Http.Request userdata.
 * \lparam  ud     Buffer.Segment user data.
 * \lparam  status current parsing status.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_req_receive( lua_State *L )
{
	struct t_buf     *buf;
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 2, 1 );
	size_t          state;
	//luaL_getmetafield( L, 1, "__name"), "t.Http.Request" );
	//lua_getfield( L, 1, "state" );
	state = (size_t) luaL_checkinteger( L, 3 );
	lua_pop( L, 1 );  // pop state
	// check if a buffer exist?
	/*
	lua_getfield( L, 1, "buf" );
	if (! lua_isnil( L, -1 ))
	{
		buf = t_buf_check_ud( L, -1, 1 );
	}
	lua_pop( L, 2 );  // pop state and buffer/nil
	*/

	switch (state)
	{
		case T_HTP_REQ_METHOD:
			if (0 == t_htp_req_parseMethod( L, seg ))
				break;
		case T_HTP_REQ_URI:
			if (0 == t_htp_req_parseUrl( L, seg ))
				break;
		case T_HTP_REQ_VERSION:
			if (0 == t_htp_req_parseHttpVersion( L, seg ))
				break;
		case T_HTP_REQ_HEADERS:
			if (0 == t_htp_req_parseHeaders( L, seg ))
				break;
		default:
			break;
	}
	/*
	if (seg->len > 0)
	{
		// create a t.Buffer object
		buf = (struct t_buf *) lua_newuserdata( L, sizeof( struct t_buf ) + (seg->len - 1) * sizeof( char ) );
		memcpy( &(buf->b[0]), &(seg->b[0]), seg->len );
		buf->len = seg->len;
		luaL_getmetatable( L, T_BUF_TYPE );
		lua_setmetatable( L, -2 );
	}
	else
		lua_pushnil( L );
	lua_setfield( L, 1, "buf" );
	*/
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_req_fm [] = {
	  { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_req_cf [] = {
	  { NULL           , NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_req_m [] = {
	  { "parse"      , lt_htp_req_receive }
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


