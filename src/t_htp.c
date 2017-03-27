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

#include "t.h"
#include "t_htp.h"


/// State of the HTTP reader; defines the current read situation apart from
/// content
enum t_htp_rs {
	T_HTP_R_XX,         ///< End of read or end of buffer
	T_HTP_R_CR,         ///< Carriage return, expect LF next
	T_HTP_R_LF,         ///< Line Feed, guaranteed end of line
	T_HTP_R_LB,         ///< Line Feed, guaranteed end of line
	T_HTP_R_KS,         ///< Reading Key Start
	T_HTP_R_KY,         ///< Read Key
	T_HTP_R_VL,         ///< Read value
	T_HTP_R_ES,         ///< Eat space
	// exit state from here
	T_HTP_R_BD,         ///< Empty line (\r\n\r\n) -> end of headers
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
	while( ' ' == *s ||  '\r' == *s ||   '\n' == *s)
		s++;
	return s;
}


/**--------------------------------------------------------------------------
 * Parse the entire first line of the request.
 * \param  L                  the Lua State
 * \param  struct t_htp_str*  pointer to t_htp_str.
 * \param  const char*        pointer to buffer to process.
 * \param  size_t             How many bytes are safe to be processed?
 *
 * \return const char*        pointer to buffer after processing the first line.
 * --------------------------------------------------------------------------*/
const char
*t_htp_pReqFirstLine( lua_State *L, struct t_htp_str *s, size_t n )
{
	const char *r   = s->con->b;  ///< runner char
	const char *me  = s->con->b;  ///< HTTP Method end
	const char *u   = NULL;       ///< URL start
	const char *ue  = NULL;       ///< URL end
	const char *q   = NULL,
	           *v   = NULL;       // query, value (query reused as key)
	int         run = 1;

	// Determine HTTP Verb
	if (n<11)
		return NULL;             // don't change anything
	r = s->con->b;
	switch (*r)
	{
		case 'C':
			if ('O'==*(r+1) && ' '==*(r+7 )) { s->mth=T_HTP_MTH_CONNECT;     me+=7;  }
			if ('H'==*(r+1) && ' '==*(r+8 )) { s->mth=T_HTP_MTH_CHECKOUT;    me+=8;  }
			if ('O'==*(r+1) && ' '==*(r+4 )) { s->mth=T_HTP_MTH_COPY;        me+=4;  }
			break;
		case 'D':
			if ('E'==*(r+1) && ' '==*(r+6 )) { s->mth=T_HTP_MTH_DELETE;      me+=6;  }
			break;
		case 'G':
			if ('E'==*(r+1) && ' '==*(r+3 )) { s->mth=T_HTP_MTH_GET;         me+=3;  }
			break;
		case 'H':
			if ('E'==*(r+1) && ' '==*(r+4 )) { s->mth=T_HTP_MTH_HEAD;        me+=4;  }
			break;
		case 'L':
			if ('O'==*(r+1) && ' '==*(r+4 )) { s->mth=T_HTP_MTH_LOCK;        me+=4;  }
			break;
		case 'M':
			if ('K'==*(r+1) && ' '==*(r+5 )) { s->mth=T_HTP_MTH_MKCOL;       me+=5;  }
			if ('K'==*(r+1) && ' '==*(r+10)) { s->mth=T_HTP_MTH_MKACTIVITY;  me+=10; }
			if ('C'==*(r+2) && ' '==*(r+10)) { s->mth=T_HTP_MTH_MKCALENDAR;  me+=10; }
			if ('-'==*(r+1) && ' '==*(r+8 )) { s->mth=T_HTP_MTH_MSEARCH;     me+=8;  }
			if ('E'==*(r+1) && ' '==*(r+5 )) { s->mth=T_HTP_MTH_MERGE;       me+=5;  }
			if ('O'==*(r+1) && ' '==*(r+4 )) { s->mth=T_HTP_MTH_MOVE;        me+=4;  }
			break;
		case 'N':
			if ('O'==*(r+1) && ' '==*(r+6 )) { s->mth=T_HTP_MTH_NOTIFY;      me+=6;  }
			break;
		case 'O':
			if ('P'==*(r+1) && ' '==*(r+7 )) { s->mth=T_HTP_MTH_OPTIONS;     me+=7;  }
			break;
		case 'P':
			if ('O'==*(r+1) && ' '==*(r+4 )) { s->mth=T_HTP_MTH_POST;        me+=4;  }
			if ('U'==*(r+1) && ' '==*(r+3 )) { s->mth=T_HTP_MTH_PUT;         me+=3;  }
			if ('A'==*(r+1) && ' '==*(r+5 )) { s->mth=T_HTP_MTH_PATCH;       me+=5;  }
			if ('U'==*(r+1) && ' '==*(r+5 )) { s->mth=T_HTP_MTH_PURGE;       me+=5;  }
			if ('R'==*(r+1) && ' '==*(r+8 )) { s->mth=T_HTP_MTH_PROPFIND;    me+=8;  }
			if ('R'==*(r+1) && ' '==*(r+9 )) { s->mth=T_HTP_MTH_PROPPATCH;   me+=9;  }
			break;
		case 'R':
			if ('E'==*(r+1) && ' '==*(r+6 )) { s->mth=T_HTP_MTH_REPORT;      me+=6;  }
			break;
		case 'S':
			if ('U'==*(r+1) && ' '==*(r+9 )) { s->mth=T_HTP_MTH_SUBSCRIBE;   me+=9;  }
			if ('E'==*(r+1) && ' '==*(r+6 )) { s->mth=T_HTP_MTH_SEARCH;      me+=6;  }
			break;
		case 'T':
			if ('R'==*(r+1) && ' '==*(r+5 )) { s->mth=T_HTP_MTH_TRACE;       me+=5;  }
			break;
		case 'U':
			if ('N'==*(r+1) && ' '==*(r+6 )) { s->mth=T_HTP_MTH_UNLOCK;      me+=6;  }
			if ('N'==*(r+2) && ' '==*(r+11)) { s->mth=T_HTP_MTH_UNSUBSCRIBE; me+=11; }
			break;
		default:
			luaL_error( L, "Illegal HTTP header: Unknown HTTP Method" );
	}
	// That means no verb was recognized and the switch fell entirely through
	if (T_HTP_MTH_ILLEGAL == s->mth)
	{
		luaL_error( L, "Illegal HTTP header: Unknown HTTP Method" );
		return NULL;
	}
	r = eat_lws( me );

	//  _   _ ____  _                            _
	// | | | |  _ \| |      _ __   __ _ _ __ ___(_)_ __   __ _
	// | | | | |_) | |     | '_ \ / _` | '__/ __| | '_ \ / _` |
	// | |_| |  _ <| |___  | |_) | (_| | |  \__ \ | | | | (_| |
	//  \___/|_| \_\_____| | .__/ \__,_|_|  |___/_|_| |_|\__, |
	//                     |_|                           |___/
	u = r;
	// TODO: create query table only when all of url is received
	while (1 == run)
	{
		switch (*r)
		{
			case '/':
				break;
			case '?':
				lua_pushstring( L, "query" );
				lua_newtable( L );
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
					lua_rawset( L, -3 );          // set proxy.query
				}
				ue = r;
				run = 0;
				break;
			default:           break;
		}
		if ((size_t) (r - s->con->b) > n) // run out of text before parsing is done
			return NULL;
		else
			r++;
	}
	r = eat_lws( r );


	//  _   _ _____ _____ ____    __     __            _
	// | | | |_   _|_   _|  _ \   \ \   / /__ _ __ ___(_) ___  _ __
	// | |_| | | |   | | | |_) |___\ \ / / _ \ '__/ __| |/ _ \| '_ \
	// |  _  | | |   | | |  __/_____\ V /  __/ |  \__ \ | (_) | | | |
	// |_| |_| |_|   |_| |_|         \_/ \___|_|  |___/_|\___/|_| |_|

	//TODO: set values based on version default behaviour (eg, KeepAlive for 1.1 etc)
	//TODO: check for n being big enough
	switch (*(r+7))
	{
		case '1': s->con->ver=T_HTP_VER_11; s->con->kpAlv=200; break;
		case '0': s->con->ver=T_HTP_VER_10; s->con->kpAlv=0  ; break;
		case '9': s->con->ver=T_HTP_VER_09; s->con->kpAlv=0  ; break;
		default: luaL_error( L, "ILLEGAL HTTP version in message" ); break;
	}

	lua_pushstring( L, "method" );
	lua_pushlstring( L, s->con->b, me - s->con->b );
	lua_rawset( L, -3 );

	lua_pushstring( L, "url" );
	lua_pushlstring( L, u, ue-u );
	lua_rawset( L, -3 );

	lua_pushstring( L, "version" );
	lua_pushlstring( L, r, 8 );
	lua_rawset( L, -3 );

	s->state = T_HTP_STR_FLINE;     // indicate first line is done
	// prepare for the header to be parsed by creating the header table on stack
	lua_pushstring( L, "header" );           //S:P,h,"header"
	lua_newtable( L );                       //S:P,h
	lua_rawset( L, -3 );                     //S:P,h
	s->con->b = eat_lws( r+8 );
	return s->con->b;
}


/**--------------------------------------------------------------------------
 * Process HTTP Headers for this request.
 * \param  L                  the Lua State
 * \param  struct t_htp_str*  pointer to t_htp_str.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing the headers.
 * --------------------------------------------------------------------------*/
const char
*t_htp_pHeaderLine( lua_State *L, struct t_htp_str *s, const size_t n )
{
	enum t_htp_rs rs = T_HTP_R_KS;     // local parse state = keystart
	const char *v    = s->con->b;      ///< marks start of value string
	const char *k    = s->con->b;      ///< marks start of key string
	const char *ke   = s->con->b;      ///< marks end of key string
	const char *r    = s->con->b;      ///< runner char
	//size_t      run  = 200;
	lua_pushstring( L, "header" );           //S:P,"header"
	lua_rawget( L, -2 );                     //S:P,h

	//while (rs && rs < T_HTP_R_BD)
	while ((size_t)(r - s->con->b) < n) // run out of text before parsing is done
	{
		// TODO: check that r+1 exists
		switch (*r)
		{
			case '\0':
				rs=T_HTP_R_XX;
				break;
			case '\r':
				if (T_HTP_R_LB != rs) rs=T_HTP_R_CR;
				break;
			case '\n':
				if (' ' == *(r+1))
					;// Handle continous value
				else
				{
					lua_pushlstring( L, k, ke-k );                            // push key
					lua_pushlstring( L, v, (rs==T_HTP_R_LB)? r-v : r-v-1 );   // push value
					lua_rawset( L, -3 );
					k  = r+1;
					rs = T_HTP_R_KS;         // Set Start of key processing
				}
				if ('\r' == *(r+1) || '\n' == *(r+1))
				{
					rs        = T_HTP_R_BD;   // End of Header; leave while loop
					s->state  = T_HTP_STR_HEADDONE;
					s->con->b = eat_lws( r );
				}
				break;
			case  ':':
				if (T_HTP_R_KY == rs)
				{
					ke = r;
					r  = eat_lws( ++r );
					v  = r;
					rs = T_HTP_R_VL;
				}
				break;
			default:
				// look for special Headers
				if (T_HTP_R_KS == rs)
				{
					rs = T_HTP_R_KY;
					switch (tokens[ (size_t) *r ])
					{
						// Content-Length, Connection
						case 'c':
							// Content-Length
							if (':' == *(r+14))
							{
								ke = r+14;
								v  = eat_lws( r+15 );
								r  = v;
								while ('\n' != *r && '\r' != *r)
								{
									s->rqCl = s->rqCl*10 + (*r - '0');
									r++;
								}
								r--;
								rs = T_HTP_R_VL;
								break;
							}
							// Connection: Keep-alive, Close, Upgrade
							else if (':' == *(r+10))
							{
								ke = r+10;
								v  = eat_lws( r+11 );
								r  = v;
								// Keep-Alive
								if ('k' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+9) ] ) s->con->kpAlv   = 200;
								// Close
								if ('c' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+4) ] ) s->con->kpAlv   = 0;
								// Upgrade
								if ('u' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+6) ] ) s->con->upgrade = 1;
								rs = T_HTP_R_VL;
								break;
							}
							break;
						// Expect
						case 'e':
							if (':' == *(r+6))
							{
								ke = r+6;
								v  = eat_lws( r+7 );
								r  = v;
								s->expect = 1;
								rs = T_HTP_R_VL;
								break;
							}
							break;
							// Upgrade
						case 'u':
							if (':' == *(r+7))
							{
								ke = r+7;
								v  = eat_lws( r+8 );
								r  = v;
								s->con->upgrade = 1;
								rs = T_HTP_R_VL;
								break;
							}
							break;
						default:
							rs = T_HTP_R_KY;
							break;
					}  // END SWITCH on First character
				}  // End test for T_HTP_R_KS
				break;
		}
		//if ((size_t)(r - s->con->b) > n) // run out of text before parsing is done
		//	return NULL;
		//else
			r++;
	}

	lua_pop( L, 1 );   // pop the header table
	return s->con->b;
}


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_lib [ ] =
{
	{ NULL,  NULL }
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
	luaopen_t_htp_srv( L );
	lua_setfield( L, -2, T_HTP_SRV_NAME );
	luaopen_t_htp_con( L );
	luaopen_t_htp_str( L );
	return 1;
}

const char
*t_htp_status( int status )
{
	switch (status)
	{
		case 100: return "Continue";                      break;
		case 101: return "Switching Protocols";           break;
		case 200: return "OK";                            break;
		case 201: return "Created";                       break;
		case 202: return "Accepted";                      break;
		case 203: return "Non-Authoritative Information"; break;
		case 204: return "No Content";                    break;
		case 205: return "Reset Content";                 break;
		case 206: return "Partial Content";               break;
		case 300: return "Multiple Choices";              break;
		case 301: return "Moved Permanently";             break;
		case 302: return "Found";                         break;
		case 303: return "See Other";                     break;
		case 304: return "Not Modified";                  break;
		case 305: return "Use Proxy";                     break;
		case 307: return "Temporary Redirect";            break;
		case 400: return "Bad Request";                   break;
		case 401: return "Unauthorized";                  break;
		case 402: return "Payment Required";              break;
		case 403: return "Forbidden";                     break;
		case 404: return "Not Found";                     break;
		case 405: return "Method Not Allowed";            break;
		case 406: return "Not Acceptable";                break;
		case 407: return "Proxy Authentication Required"; break;
		case 408: return "Request Timeout";               break;
		case 409: return "Conflict";                      break;
		case 410: return "Gone";                          break;
		case 411: return "Length Required";               break;
		case 412: return "Precondition Failed";           break;
		case 413: return "Payload Too Large";             break;
		case 414: return "URI Too Long";                  break;
		case 415: return "Unsupported Media Type";        break;
		case 416: return "Range Not Satisfiable";         break;
		case 417: return "Expectation Failed";            break;
		case 426: return "Upgrade Required";              break;
		case 500: return "Internal Server Error";         break;
		case 501: return "Not Implemented";               break;
		case 502: return "Bad Gateway";                   break;
		case 503: return "Service Unavailable";           break;
		case 504: return "Gateway Timeout";               break;
		case 505: return "HTTP Version Not Supported";    break;
	}
	return NULL;
}

