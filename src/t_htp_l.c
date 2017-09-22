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
#include <time.h>                 // gmtime

#include "t_htp_l.h"
#include "t_buf.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


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
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_lib [ ] =
{
	  { NULL               , NULL }
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
	luaopen_t_htp_req( L );
	//lua_setfield( L, -2, T_HTP_REQ_NAME );
	return 1;
}

