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



static int lt_htp_msg__gc( lua_State *luaVM );
/**
 * Eat Linear White Space
 */
static inline const char
*eat_lws( const char *s )
{
	while( ' ' == *s ||  '\r' == *s ||   '\n' == *s)
		s++;
	return s;
};


/**--------------------------------------------------------------------------
 * Determine the HTTP method used for this request.
 * \param  luaVM              the Lua State
 * \param  struct t_htp_msg*  pointer to t_htp_msg.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing method.
 * --------------------------------------------------------------------------*/
static inline const char
*t_htp_msg_pMethod( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	switch (*b)
	{
		case 'C':
			if ('O'==*(b+1) && ' '==*(b+7 )) { m->mth=T_HTP_MTH_CONNECT;     d+=7;  }
			if ('H'==*(b+1) && ' '==*(b+8 )) { m->mth=T_HTP_MTH_CHECKOUT;    d+=8;  }
			if ('O'==*(b+1) && ' '==*(b+4 )) { m->mth=T_HTP_MTH_COPY;        d+=4;  }
			break;
		case 'D':
			if ('E'==*(b+1) && ' '==*(b+6 )) { m->mth=T_HTP_MTH_DELETE;      d+=6;  }
			break;
		case 'G':
			if ('E'==*(b+1) && ' '==*(b+3 )) { m->mth=T_HTP_MTH_GET;         d+=3;  }
			break;
		case 'H':
			if ('E'==*(b+1) && ' '==*(b+4 )) { m->mth=T_HTP_MTH_HEAD;        d+=4;  }
			break;
		case 'L':
			if ('O'==*(b+1) && ' '==*(b+4 )) { m->mth=T_HTP_MTH_LOCK;        d+=4;  }
			break;
		case 'M':
			if ('K'==*(b+1) && ' '==*(b+5 )) { m->mth=T_HTP_MTH_MKCOL;       d+=5;  }
			if ('K'==*(b+1) && ' '==*(b+10)) { m->mth=T_HTP_MTH_MKACTIVITY;  d+=10; }
			if ('C'==*(b+2) && ' '==*(b+10)) { m->mth=T_HTP_MTH_MKCALENDAR;  d+=10; }
			if ('-'==*(b+1) && ' '==*(b+8 )) { m->mth=T_HTP_MTH_MSEARCH;     d+=8;  }
			if ('E'==*(b+1) && ' '==*(b+5 )) { m->mth=T_HTP_MTH_MERGE;       d+=5;  }
			if ('O'==*(b+1) && ' '==*(b+4 )) { m->mth=T_HTP_MTH_MOVE;        d+=4;  }
			break;
		case 'N':
			if ('O'==*(b+1) && ' '==*(b+6 )) { m->mth=T_HTP_MTH_NOTIFY;      d+=6;  }
			break;
		case 'O':
			if ('P'==*(b+1) && ' '==*(b+7 )) { m->mth=T_HTP_MTH_OPTIONS;     d+=7;  }
			break;
		case 'P':
			if ('O'==*(b+1) && ' '==*(b+4 )) { m->mth=T_HTP_MTH_POST;        d+=4;  }
			if ('U'==*(b+1) && ' '==*(b+3 )) { m->mth=T_HTP_MTH_PUT;         d+=3;  }
			if ('A'==*(b+1) && ' '==*(b+5 )) { m->mth=T_HTP_MTH_PATCH;       d+=5;  }
			if ('U'==*(b+1) && ' '==*(b+5 )) { m->mth=T_HTP_MTH_PURGE;       d+=5;  }
			if ('R'==*(b+1) && ' '==*(b+8 )) { m->mth=T_HTP_MTH_PROPFIND;    d+=8;  }
			if ('R'==*(b+1) && ' '==*(b+9 )) { m->mth=T_HTP_MTH_PROPPATCH;   d+=9;  }
			break;
		case 'R':
			if ('E'==*(b+1) && ' '==*(b+6 )) { m->mth=T_HTP_MTH_REPORT;      d+=6;  }
			break;
		case 'S':
			if ('U'==*(b+1) && ' '==*(b+9 )) { m->mth=T_HTP_MTH_SUBSCRIBE;   d+=9;  }
			if ('E'==*(b+1) && ' '==*(b+6 )) { m->mth=T_HTP_MTH_SEARCH;      d+=6;  }
			break;
		case 'T':
			if ('R'==*(b+1) && ' '==*(b+5 )) { m->mth=T_HTP_MTH_TRACE;       d+=5;  }
			break;
		case 'U':
			if ('N'==*(b+1) && ' '==*(b+6 )) { m->mth=T_HTP_MTH_UNLOCK;      d+=6;  }
			if ('N'==*(b+2) && ' '==*(b+11)) { m->mth=T_HTP_MTH_UNSUBSCRIBE; d+=11; }
			break;
		default:
			luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
	}
	// That means no verb was recognized and the switch fell entirely through
	if (T_HTP_MTH_ILLEGAL == m->mth)
	{
		luaL_error( luaVM, "Illegal HTTP header: Unknown HTTP Method" );
		return NULL;
	}
	lua_pushstring( luaVM, "method" );
	lua_pushlstring( luaVM, b, d-b );
	lua_rawset( luaVM, -3 );
	d = eat_lws( d );
	m->bRead += d-b;
	m->pS     = T_HTP_STA_URL;
	return d;
}


/**--------------------------------------------------------------------------
 * Process URI String for this request. Parse Query if present.
 * \param  luaVM              the Lua State
 * \param  struct t_htp_msg*  pointer to t_htp_msg.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing the URI.
 *
 * TODO: URL decode key and value
 * TODO: Validate proper key/value pairs
 * --------------------------------------------------------------------------*/
static inline const char
*t_htp_msg_pUrl( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	const char *q  = NULL,
	           *v  = NULL;   // query, value (query reused as key)
	while (' ' != *d)
	{
		switch (*d)
		{
			case '/':
				break;
			case '?':
				lua_pushstring( luaVM, "query" );
				lua_newtable( luaVM );
				q = d+1;
				break;
			case '=':
				lua_pushlstring( luaVM, q, d-q ); // push key
				//TODO: if key exists, create table
				v = d+1;
				break;
			case '&':
				lua_pushlstring( luaVM, v, d-v ); // push value
				lua_rawset( luaVM, -3 );
				q = d+1;
				break;
			default:           break;
		}
		d++;
	}
	if (NULL != q)
	{
		lua_pushlstring( luaVM, v, d-v ); // push last value
		lua_rawset( luaVM, -3 );          // set last key
		lua_rawset( luaVM, -3 );          // set proxy.query
	}
	lua_pushstring( luaVM, "url" );
	lua_pushlstring( luaVM, b, d-b );
	lua_rawset( luaVM, -3 );
	d = eat_lws( d );
	m->bRead += d-b;
	m->pS     = T_HTP_STA_VERSION;
	return d;
}


/**--------------------------------------------------------------------------
 * Process HTTP Version for this request.
 * \param  luaVM              the Lua State
 * \param  struct t_htp_msg*  pointer to t_htp_msg.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing the HTTP VERSION.
 * --------------------------------------------------------------------------*/
static inline const char
*t_htp_msg_pVersion( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	const char *d  = b;
	size_t      i;

	d = strchr( b, '\r' );
	i = d-b;
	if (8 != i)
		luaL_error( luaVM, "ILLEGAL HTTP version in HTTP message" );
	//TODO: set values based on version default behaviour (eg, KeepAlive for 1.1 etc)
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
	// prepare for the header to be parsed by creating the header table on stack
	lua_newtable( luaVM );                       //S:P,h
	lua_pushstring( luaVM, "header" );           //S:P,h,"header"
	lua_pushvalue( luaVM, -2 );                  //S:P,h,"header",h
	lua_rawset( luaVM, -4 );                     //S:P,h
	return d;
}


/** --------------------------------------------------------------------------
 * Analyze the header for an T_HTP_CON relevant information.
 * HTTP defines a lot of headers. This fishes only for the ones immediately
 * affecting the connection strategy.  Looking for:
 *      - Expect
 *      - Content-Length
 *      - Host
 *      - Range
 *      - Upgrade
 * \param  lua_State          luaVM the Lua State.
 * \param  struct t_htp_msg*  pointer to t_htp_msg.
 * \param  char *             pointer to the buffer.
 * --------------------------------------------------------------------------*/


/**--------------------------------------------------------------------------
 * Process HTTP Headers for this request.
 * \param  luaVM              the Lua State
 * \param  struct t_htp_msg*  pointer to t_htp_msg.
 * \param  const char*        pointer to buffer to process.
 *
 * \return const char*        pointer to buffer after processing the headers.
 * --------------------------------------------------------------------------*/
static inline const char
*t_htp_msg_pHeader( lua_State *luaVM, struct t_htp_msg *m, const char *b )
{
	enum t_htp_rs rs = T_HTP_R_KS;
	const char *v    = b;             ///< marks start of value string
	const char *k    = b;    ///< marks start of key string
	const char *ke   = b;             ///< marks end of key string
	const char *r    = b;             ///< runner
	//size_t      run  = 200;

	while (rs && rs < T_HTP_R_BD)
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
					lua_pushlstring( luaVM, k, ke-k );   // push key
					lua_pushlstring( luaVM, v, (rs==T_HTP_R_LB)? r-v : r-v-1 );   // push value
					lua_rawset( luaVM, -3 );
					k  = r+1;
					rs = T_HTP_R_KS;         // Set Key to be Capitalized
				}
				if ('\r' == *(r+1) || '\n' == *(r+1))
				{
					rs = T_HTP_R_BD;   // End of Header
					m->pS = T_HTP_STA_HEADDONE;
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
				if (T_HTP_R_KS == rs)
				{
					switch ((size_t) tokens[ (size_t) *r ])
					{
						// Content-Length, Connection
						case 'c':
							// Content-Length
							if (':'==*(b+15))
							{
								v = eat_lws( b+16 );
								r=v;
								while ('\n' != *r && '\r' != *r)
								{
									m->length = m->length*10 + (*r - '0');
									r++;
								}
								rs = T_HTP_R_VL;
								break;
							}
							// Connection: Keep-alive, Close, Upgrade
							if (':'==*(b+11))
							{
								v = eat_lws( b+12 );
								r = v;
								if ('k' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+9) ] ) m->kpAlv   = 200;
								if ('c' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+4) ] ) m->kpAlv   = 0;
								if ('u' == tokens[ (size_t) *v ] && 'e' == tokens[ (size_t) *(v+6) ] ) m->upgrade = 1;
								rs = T_HTP_R_VL;
								break;
							}
							break;
						// Expect
						case 'e':
							if (':'==*(b+7))
							{
								v = eat_lws( b+8 );
								r = v;
								m->expect = 1;
								rs = T_HTP_R_VL;
								break;
							}
							break;
							// Upgrade
						case 'u':
							if (':'==*(b+8))
							{
								v = eat_lws( b+9 );
								r = v;
								m->upgrade = 1;
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
		r++;
	}

	lua_pop( luaVM, 1 );   // pop the header table
	return r;
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
	m->bRead  = 0;
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
	struct t_ael     *ael;
	int               rcvd;
	const char       *nxt;   // pointer to the buffer where processing must continue

	// get the proxy on the stack to fill out verb/url/version/headers ...
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR ); //S:m,P

	// read
	rcvd = t_sck_recv( luaVM, m->sck, &(m->buf[ m->bRead ]), BUFSIZ - m->bRead );
	printf( "%d   %s\n", rcvd, &(m->buf[ m->bRead ]) );

	nxt = &(m->buf[ m->bRead ]);

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
			case T_HTP_STA_HEADDONE:
				// execute function from server
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->rR );
				lua_pushvalue( luaVM, 1 );
				lua_call( luaVM, 1,0 );
				// keep reading if body, else stop reading
				if (m->length > 0)
				{
					m->pS = T_HTP_STA_BODY;
					// read body
				}
				else
				{  // Don't test this socket for reading any more
					lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
					ael = t_ael_check_ud( luaVM, -1, 1 );
					t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_RD );
					ael->fd_set[ m->sck->fd ]->t = ael->fd_set[ m->sck->fd ]-> t & (~T_AEL_RD);
					lua_pop( luaVM, 1 );             // pop the event loop
					nxt = NULL;
				}
				break;
			case T_HTP_STA_BODY:        // now we can process the function and start writing back
				// TODO: if all of Content-Length is read, stop reading on socket
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
				ael = t_ael_check_ud( luaVM, -1, 1);
				t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_RD );
				ael->fd_set[ m->sck->fd ]->t = ael->fd_set[ m->sck->fd ]-> t & (~T_AEL_RD);
				lua_pop( luaVM, 1 );             // pop the event loop
				nxt = NULL;
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

	m->sent += t_sck_send( luaVM, m->sck, &(m->buf[ m->sent ]), m->bRead );

	printf("RESPONSE:  %zu    %zu      %s\n", m->bRead, m->sent,  m->buf);
	// if (T_HTP_STA_DONE == m->pS)
	if (m->sent >= m->bRead)      // if current buffer is empty
	{
		if (T_HTP_STA_FINISH==m->pS)
		{
			// if keepalive reverse socket again and create timeout function
			if (! m->kpAlv)
			{
				lua_pushcfunction( luaVM, lt_htp_msg__gc );
				lua_pushvalue( luaVM, 1 );
				lua_call( luaVM, 1, 0 );
			}
			else     // start reading again
			{
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
				ael = t_ael_check_ud( luaVM, -1, 1 );
				t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
				ael->fd_set[ m->sck->fd ]->t = ael->fd_set[ m->sck->fd ]-> t & (~T_AEL_WR);
				lua_pop( luaVM, 1 );
			}
		}
		else
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
			ael = t_ael_check_ud( luaVM, -1, 1 );
			t_ael_removehandle_impl( ael, m->sck->fd, T_AEL_WR );
			ael->fd_set[ m->sck->fd ]->t = ael->fd_set[ m->sck->fd ]-> t & (~T_AEL_WR);
			lua_pop( luaVM, 1 );
		}
	}

	return 1;
}



static void
t_htp_msg_prepresp( struct t_htp_msg *m )
{
	t_htp_srv_setnow( m->srv, 0 );            // update server time
	m->bRead = (size_t) snprintf( m->buf, BUFSIZ,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 17 \r\n"
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
	struct t_ael     *ael;
	size_t            s;
	const char       *v = luaL_checklstring( luaVM, 2, &s );

	if (T_HTP_STA_SEND != m->pS)
	{
		memset( &(m->buf[0]), 0, BUFSIZ );
		m->sent  = 0;
		t_htp_msg_prepresp( m );
	}

	memcpy( &(m->buf[ m->bRead ]), v, s );
	m->bRead += s;

	if (T_HTP_STA_SEND != m->pS)
	{
		m->pS = T_HTP_STA_SEND;
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->srv->lR );
		ael = t_ael_check_ud( luaVM, -1, 1 );
		t_ael_addhandle_impl( ael, m->sck->fd, T_AEL_WR );
		ael->fd_set[ m->sck->fd ]->t = T_AEL_WR;
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
	m->pS = T_HTP_STA_FINISH;

	return 0;
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
	const char       *k = luaL_checkstring( luaVM, -1 );

	if (0 == strncmp( k, "write", 5 ))
	{
		lua_pushcfunction( luaVM, lt_htp_msg_write );
		return 1;
	}
	else if (0 == strncmp( k, "finish", 6 ))
	{
		lua_pushcfunction( luaVM, lt_htp_msg_finish );
		return 1;
	}
	else
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, m->pR );  // fetch the proxy table
		lua_pushvalue( luaVM, -2 );                      // repush the key
		lua_rawget( luaVM, -2 );

		return 1;
	}
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

	return 0;
}

