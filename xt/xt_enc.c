/**
 * \file   xt_enc.c
 *         an umbrella for en/decoding routines
 */
#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno

#include "l_xt.h"
#include "xt_enc.h"


/**
 * \brief      the (empty) xt library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_enc_lib [] =
{
	{NULL,        NULL}
};


/**
 * \brief     Export the xti enc libray to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_enc (lua_State *luaVM)
{
	luaL_newlib (luaVM, xt_enc_lib);
	luaopen_enc_arc4(luaVM);
	lua_setfield(luaVM, -2, "Arc4");
	return 1;
}

