/**
 * \file   library.c
 *         a couple of helper files used throughout all library files
 */
#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno

#include "l_xt.h"



/**
 * \brief  Prints a list of items on the lua stack.
 * \param  luaVM The Lua state.
 */
void stackDump (lua_State *luaVM) {
	int i;
	int top = lua_gettop(luaVM);
	printf("LUA STACK[%d]:\t", top);
	for (i = 1; i <= top; i++) {     /* repeat for each level */
		int t = lua_type(luaVM, i);
		switch (t) {

			case LUA_TSTRING:    /* strings */
				printf("`%s'", lua_tostring(luaVM, i));
				break;

			case LUA_TBOOLEAN:   /* booleans */
				printf(lua_toboolean(luaVM, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:    /* numbers */
				printf("%g", lua_tonumber(luaVM, i));
				break;

			default:	            /* other values */
				printf("%s", lua_typename(luaVM, t));
				break;
		}
		printf("\t");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}


/**
 * \brief  Return an error string to the LUA script.
 *         Pass NULL to use the return value of strerror.
 * \param  luaVM The Lua intepretter object.
 * \param  info  An error string to return to the caller.
 */
int pusherror(lua_State *luaVM, const char *info)
{
	lua_pushnil(luaVM);
	if (info==NULL)
		return luaL_error(luaVM, strerror(errno));
	else
		return luaL_error(luaVM, "%s: %s", info, strerror(errno));
}


/**
 * \brief      the (empty) xt library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_xt_fm [] =
{
	{NULL,        NULL}
};


/**
 * \brief     Export the net library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_xt (lua_State *luaVM)
{
	luaL_newlib (luaVM, l_xt_fm);
	luaopen_net(luaVM);
	lua_setfield(luaVM, -2, "net");
	luaopen_buf(luaVM);
	lua_setfield(luaVM, -2, "buffer");
	luaopen_debug(luaVM);
	lua_setfield(luaVM, -2, "debug");
	return 1;
}

