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

