/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp.c
 * \brief     OOP wrapper for HTTP operation
 * \detail    Contains meta methods, parsers, status codes etc.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset

#include "t.h"
#include "t_htp.h"


void
t_htp_parse( struct t_htp_msg *m, const char *buf )
{
	char    *n  = buf;
	char    *x  = n;
	size_t   got_get=0, get_cnt=0, slash_cnt=0, error=0;
	size_t   i;

	x = strchr( n, ' ' );
	i = x-n;
	switch (n)
	{
		case 'C': {
			if (7==i && 'O'==n+1)
				m->mth=T_HTP_MTH_CONNECT;
			else if (8==i && 'H'==n+1)
				m->mth=T_HTP_MTH_CHECKOUT;
			else if (4==i && 'O'==n+1)
				m->mth=T_HTP_MTH_COPY;
			else
				m->mth=T_HTP_MTH_ILLEGAL;
			break;
		}
		case 'D': m->mth=T_HTP_MTH_DELETE; break;
		case 'G': m->mth=T_HTP_MTH_GET; break;
		case 'H': m->mth=T_HTP_MTH_HEAD; break;
		case 'L': m->mth=T_HTP_MTH_LOCK; break;
		case 'M': {
			m->mth=T_HTP_MTH_MKCOL; /* or MOVE, MKACTIVITY, MERGE, M-SEARCH, MKCALENDAR */ break;
			if (5==i && 'K'==n+1)
				m->mth=T_HTP_MTH_MKCOL;
			else if (10==i && 'K'==n+1)
				m->mth=T_HTP_MTH_MKACTIVITY;
			else if (10==i && 'C'==n+2)
				m->mth=T_HTP_MTH_MKCALENDAR;
			else if (8==i && '-'==n+1)
				m->mth=T_HTP_MTH_MSEARCH;
			else if (5==i && 'E'==n+1)
				m->mth=T_HTP_MTH_MERGE;
			else
				m->mth=T_HTP_MTH_ILLEGAL;
			break;
		}
		case 'N': m->mth=T_HTP_MTH_NOTIFY; break;
		case 'O': m->mth=T_HTP_MTH_OPTIONS; break;
		case 'P': m->mth=T_HTP_MTH_POST; /* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE */ break;
		case 'R': m->mth=T_HTP_MTH_REPORT; break;
		case 'S': m->mth=T_HTP_MTH_SUBSCRIBE; /* or SEARCH */ break;
		case 'T': m->mth=T_HTP_MTH_TRACE; break;
		case 'U': m->mth=T_HTP_MTH_UNLOCK; /* or UNSUBSCRIBE */ break;
		default:
            SET_ERRNO(HPE_INVALID_METHOD);
            goto error;
        }

enum t_htp_ver {


	if      (0 == memcmp(n, "GET ", 4)) { m->rq_t=RQT_GET; n+=3; }
	else if (0 == memcmp(n, "POST", 4)) { m->rq_t=RQT_PST; n+=4; }
	else if (0 == memcmp(n, "PUT ", 4)) { m->rq_t=RQT_PUT; n+=3; }
	else if (0 == memcmp(n, "DELE", 4)) { m->rq_t=RQT_DLT; n+=6; }
	else if (0 == memcmp(n, "HEAD", 4)) { m->rq_t=RQT_HED; n+=4; }
	else if (0 == memcmp(n, "OPTI", 4)) { m->rq_t=RQT_OPT; n+=7; }
	else if (0 == memcmp(n, "TRAC", 4)) { m->rq_t=RQT_TRC; n+=5; }
	else if (0 == memcmp(n, "PATC", 4)) { m->rq_t=RQT_PAT; n+=5; }
	else if (0 == memcmp(n, "CONN", 4)) { m->rq_t=RQT_CON; n+=7; }
	else m->rq_t=RQT_ILG;
	*next = '\0';
	// URL
	n++; // eat the space
	if ('/' == *n)
	{
		m->url = n;
	}
	else
	{
		/* we are extremely unhappy ... -> malformed url
		   error(400, "URL has to start with a '/'!"); */
		printf("Crying game....\n");
	}
	/* chew through url, find GET, check url sanity */
	while ( !got_get && ' ' != *next ) {
		switch (*next) {
			case ' ':
				*next = '\0';
				break;
			case '?':
				got_get = 1;
				cn->get_str = next+1;
				*next = '\0';
				break;
			case '/':
				slash_cnt++;
				if ('.' == *(next+1) && '.' == *(next+2) && '/' == *(next+3))
					slash_cnt--;
				break;
			case '.':
				if ('/' == *(next-1) && '/' != *(next+1))
					error = 400;  /* trying to reach hidden files */
				break;
			default:
				/* keep chewing */
				break;
		}
		next++;
	}
	/* GET - count get parameters */
	while ( got_get && ' ' != *next ) {
		if( '=' == *next) {
			get_cnt++;
		}
		next++;
	}
	*next = '\0';
	next++;
	/* HTTP protocol version */
	if (0 == strncasecmp(next, "HTTP/", 5))
		next+=5;
	else
		// error(400, "The HTTP protocol type needs to be specified!");
		printf("Crying game....\n");
	if (0 == strncasecmp(next, "0.9", 3)) { cn->http_prot=HTTP_09; }
	if (0 == strncasecmp(next, "1.0", 3)) { cn->http_prot=HTTP_10; }
	if (0 == strncasecmp(next, "1.1", 3)) { cn->http_prot=HTTP_11; }
#if DEBUG_VERBOSE==3
	printf("URL SLASHES: %d -- GET PARAMTERS: %d --ERRORS: %d\n",
		slash_cnt, get_cnt, error);
#endif
}
/**
 * \brief      the (empty) T.Http library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_htp_lib [] =
{
	//{"crypt",     t_enc_crypt},
	{NULL,        NULL}
};


/**
 * \brief     Export the t_htp libray to Lua
 * \param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int
luaopen_t_htp( lua_State *luaVM )
{
	luaL_newlib( luaVM, t_htp_lib );
	luaopen_t_htp_srv( luaVM );
	lua_setfield( luaVM, -2, "Server" );
	luaopen_t_htp_msg( luaVM );
	return 1;
}

