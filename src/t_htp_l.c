/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.c
 * \brief     OOP wrapper for HTTP operation
 * \detail    t_htp namespace is a bit different from normal lua-t namespaces.
 *            There is no T.Http that it relates to.  Instead there are meta
 *            methods, parsers, status codes etc. Mainly helpers.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset

#include "t_htp_l.h"
#include "t_buf.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/// State of the HTTP reader; defines the current read situation apart from
/// content
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
 * Set start point of Buffer.Segment
 * \param   L     Lua state.
 * \param   struct t_buf_seg seg.
 * \param   mv    move start point.
 * --------------------------------------------------------------------------*/
void
t_buf_seg_moveIndex( struct t_buf_seg *seg, int mv )
{
	seg->b    = seg->b + mv;
	seg->idx += mv;
	seg->len -= mv;
}


// ########################## REWRITE
// TODO: use this to adjust large incoming chnks for headers upto BUFSIZ per
// line
/**--------------------------------------------------------------------------
 * Rewrite unparsed bytes from end of buffer to front.
 * \param  L                  the Lua State
 * \param  struct t_htp_str*  pointer to t_htp_str.
 * \param  const char*        pointer to buffer to process.
 * \param  size_t             How many bytes are safe to be processed?
 *
 * \return const char*        pointer to buffer after processing the first line.
 * --------------------------------------------------------------------------*/
void t_htp_adjustBuffer( struct t_buf *buf, size_t index )
{
	memcpy( &(buf->b[0]), (const void *) (buf->b + index ), buf->len - index );
}


/**--------------------------------------------------------------------------
 * Parse Method from the request.
 * \param  L                  the Lua State
 * \param  struct t_htp_str*  pointer to t_htp_str.
 * \param  const char*        pointer to buffer to process.
 * \param  size_t             How many bytes are safe to be processed?
 *
 * \return const char*        pointer to buffer after processing the first line.
 * --------------------------------------------------------------------------*/
static int
lt_htp_parseMethod( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	size_t            n   = 0;                 ///< offset of whitespace after method
	int               m   = T_HTP_MTH_ILLEGAL; ///< HTTP.Method index
	const char       *r   = seg->b;            ///< runner char

	// Determine HTTP Verb
	if (seg->len < 11)
		lua_pushnil( L );
	else
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
	}
	// adjust the buffer segment
	t_buf_seg_moveIndex( seg, n );
	return 1;
}


static int
lt_htp_parseUrl( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	const char       *r   = eat_lws( seg->b );  ///< runner char
	const char       *e   = seg->b + seg->len;  ///< ending char
	const char       *u   = r;                  ///< start of URI
	const char       *q   = NULL;               ///< runner for query
	const char       *v   = r;                  ///< value start marker

	lua_newtable( L );               ///< parsed and decoded query parameters

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
				lua_pushlstring( L, q, r-q ); // push key
				//TODO: if key exists, create table
				v = r+1;
				break;
			case '&':
				lua_pushlstring( L, v, r-v ); // push value
				lua_rawset( L, -3 );
				q = r+1;
				break;
			case ' ':      // last value
				if (NULL != q)
				{
					lua_pushlstring( L, v, r-v ); // push value
					lua_rawset( L, -3 );
				}
				lua_pushlstring( L, u, r-u );    // push full query string
				t_buf_seg_moveIndex( seg, r - seg->b );
				return 2;
				break;
			default:           break;
		}
		r++;
	}
	// expensive recovery because we are optimistic to read the entire
	// header at once
	// TODO: adjust buffer; clean up stack; return nil
	return 0;
}


static int
lt_htp_parseHttpVersion( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	const char       *r   = eat_lws( seg->b );  ///< runner char
	int               v   = T_HTP_VER_ILL;

	t_buf_seg_moveIndex( seg, r - seg->b );
	//TODO: set values based on version default behaviour (eg, KeepAlive for 1.1 etc)
	//TODO: check for n being big enoughp
	if ('H'==*(r) && ('\r'==*(r+8 )))
		switch (*(r+7))
		{
			case '1': v = T_HTP_VER_11; break;
			case '0': v = T_HTP_VER_10; break;
			case '9': v = T_HTP_VER_09; break;
			default: luaL_error( L, "ILLEGAL HTTP version in message" ); break;
		}
	lua_pushinteger( L, v );
	t_buf_seg_moveIndex( seg, 8 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Process HTTP Headers for this request.
 * \param  L                  the Lua State
 * \param  struct t_htp_str*  pointer to t_htp_str.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing the headers.
 * --------------------------------------------------------------------------*/
static int
lt_htp_parseHeaders( lua_State *L )
{
	struct t_buf_seg *seg = t_buf_seg_check_ud( L, 1, 1 );
	const char       *r   = eat_lws( seg->b );  ///< runner char
	const char       *e   = seg->b + seg->len;  ///< ending char
	const char       *s   = r;                  ///< marks start of string
	enum t_htp_rs     rs  = T_HTP_R_KY;         ///< local parse state = New Line Beginning

	lua_newtable( L );                          ///< parsed headers

	// since exit condition is based on r+1 compare for (r+1)
	while (r+1 < e)
	{
		switch (*r)
		{
			case '\n':
				if (' ' == *(r+1)) // Value Continuation
					break;

				lua_pushlstring( L, s, ('\r' == *(r-1)) ? r-s-1 : r-s );   // push value
				lua_rawset( L, -3 );
				rs = T_HTP_R_KY;
				if ('\n' == *(r+1) || '\r' == *(r+1))  // double newLine -> END OF HEADER
				{
					t_buf_seg_moveIndex( seg, (r + (('\n'==*(r+1))? 1 : 2) - seg->b) );
					return 1;
				}
				s  = r+1;
				break;
			case  ':':
				if (T_HTP_R_KY == rs)
				{
					lua_pushlstring( L, s, r-s );                           // push key
					r  = eat_lws( ++r );
					s  = r;
					rs = T_HTP_R_VL;
				}
				break;
			default:
				break;
		}
		r++;
	}
	// expensive recovery because we are optimistic to read the entire
	// header at once
	// TODO: adjust buffer; clean up stack; return nil
	return 0;
}
/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_lib [ ] =
{
	  { "parseMethod"      , lt_htp_parseMethod }
	, { "parseUrl"         , lt_htp_parseUrl }
	, { "parseVersion"     , lt_htp_parseHttpVersion }
	, { "parseHeaders"     , lt_htp_parseHeaders }
	, { NULL               , NULL }
};


/**--------------------------------------------------------------------------
 * Export the t_htp libray to Lua
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_htp( lua_State *L )
{
	luaL_newlib( L, t_htp_lib );
	return 1;
}

