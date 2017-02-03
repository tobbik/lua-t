/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t.c
 * \brief     Global wrapper and packer for lua-t library.
 *            Exports sub libraries. Defines global helper functions.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>
#include <string.h>     // strerror,strrchr
#include <errno.h>      // errno

#include "t.h"

/** -------------------------------------------------------------------------
 * Formats an item on the stack.  Pushes formatted string to last position.
 * \param  L        The Lua intepretter object.
 * \param  int      position stack element to format.
 * \param  int/bool use_tostring if 1 use tostring( ), else .
 *-------------------------------------------------------------------------*/
void
t_fmtStackItem( lua_State *L, int idx, int no_tostring )
{
	int t;
	if (no_tostring || ! luaL_callmeta( L, idx, "__tostring" ))
	{
		t = lua_type( L, idx );
		switch (t)
		{
			case LUA_TNIL:       // nil
				lua_pushliteral( L, "nil" );
				break;

			case LUA_TSTRING:    // strings
				lua_pushfstring( L, "`%s`", lua_tostring( L, idx ) );
				break;

			case LUA_TBOOLEAN:   // booleans
				lua_pushstring( L, lua_toboolean( L, idx ) ? "true" : "false" );
				break;

			case LUA_TNUMBER:    // numbers
				if (lua_isinteger( L, idx ))
					lua_pushfstring( L, "%I", lua_tointeger( L, idx ) );
				else
					lua_pushfstring( L, "%f", lua_tonumber( L, idx ) );
				break;

			case LUA_TUSERDATA:  // userdata
				if (luaL_getmetafield( L, idx, "__name" ))  // does it have a metatable?
				{
					lua_pushfstring( L, "u.%s", lua_tostring( L, -1 ) );
					lua_remove( L, -2 );
				}
				else
					lua_pushliteral( L, "ud" );
				break;

			case LUA_TTABLE:    // tables
				if (luaL_getmetafield( L, idx, "__name" ))  // does it have a metatable?
				{
					lua_pushfstring( L, "t.%s", lua_tostring( L, -1 ) );
					lua_remove( L, -2 );
				}
				else
					lua_pushliteral( L, "table" );
				break;

			default:            // other values
				lua_pushfstring( L, "%s", lua_typename( L, t ) );
				break;
		}
	}
}


/** -------------------------------------------------------------------------
 * Formats the elements of the stack.
 * \param  L     The Lua intepretter object.
 * \param  int   first stack element to format.
 * \param  int   last stack element to format.
 *-------------------------------------------------------------------------*/
void
t_stackPrint( lua_State *L, int idx, int last, int no_tostring )
{
	for ( ;idx <= last; idx++)
	{
		t_fmtStackItem( L, idx, no_tostring );
		printf( "%s  ", lua_tostring( L, -1 ) ); // print serialized item and separator
		lua_pop( L, 1 );                        // pop serialized item
	}
}

/** -------------------------------------------------------------------------
 * Prints a list of items on the lua stack.
 * \param  L The Lua state.
 *-------------------------------------------------------------------------*/
void
t_stackDump ( lua_State *L )
{
	int top = lua_gettop( L );
	printf( "STACK[%d]:   ", top );
	t_stackPrint( L, 1, top, 1 );
	printf( "\n" );  // end the listing
}


/** -------------------------------------------------------------------------
 * Returns an error string to the LUA script.
 * Expands luaL_error by errno support.
 * \param  L The Lua intepretter object.
 * \param  info  Error string.
 * \param  ...   variable arguments to fmt
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_push_error( lua_State *L, const char *fmt, ... )
{
	va_list argp;
	if (NULL==fmt)
	{
		if (0==errno) return luaL_error( L, strerror( errno ) );
		else          return luaL_error( L, "Unknown Error" );
	}
	else
	{
		va_start( argp, fmt );
		luaL_where( L, 1 );
		lua_pushvfstring( L, fmt, argp );
		va_end( argp );
		if (0==errno) lua_pushstring( L, "\n" );
		else          lua_pushfstring( L, ": %s\n", strerror( errno ) );
		lua_concat( L, 3 );
		return lua_error( L );
	}
}


/** -------------------------------------------------------------------------
 * Extended require patches the path and cpath with current path of file.
 * \param   L     Lua state.
 *-------------------------------------------------------------------------*/
static int
lt_require( lua_State *L )
{
	lua_Debug ar;
	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( L, -1, "package" );

	lua_getstack( L, 1, &ar );
	lua_getinfo( L, "S", &ar );
	strrchr( ar.short_src, '/' )[0] = 0x00;

	lua_getfield( L, -1, "path" );
	lua_pushvalue( L, -1 );
	lua_pushfstring(L, ";%s/?.lua;%s/?/init.lua", ar.short_src, ar.short_src );
	lua_concat( L, 2 );
	lua_setfield( L, -3, "path" );               //S: nme LOD pck pth

	lua_getfield( L, -2, "cpath" );
	lua_pushvalue( L, -1 );
	lua_pushfstring(L, ";%s/?.so", ar.short_src );
	lua_concat( L, 2 );
	lua_setfield( L, -4, "cpath" );              //S: nme LOD pck pth cpt

	lua_getglobal( L, "require" );
	lua_pushvalue( L, 1 );
	lua_call( L, 1, 1 );                        //S: nme LOD pck pth cpt mod
	lua_insert( L, 2 );
	lua_setfield( L, -3, "cpath" );
	lua_setfield( L, -2, "path" );
	lua_pop( L, 2 );
	t_stackDump( L );

	return 0;
}


/**--------------------------------------------------------------------------
 * Object methods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg l_t_lib [] =
{
	// t-global methods
	  { "require"     ,   lt_require }
	, { NULL          ,   NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t( lua_State *L )
{
	luaL_newlib( L, l_t_lib );
	luaopen_t_ael( L );
	lua_setfield( L, -2, T_AEL_NAME );
	luaopen_t_tim( L );
	lua_setfield( L, -2, T_TIM_NAME );
	luaopen_t_net( L );
	lua_setfield( L, -2, T_NET_NAME );
	luaopen_t_buf( L );
	lua_setfield( L, -2, T_BUF_NAME );
	luaopen_t_pck( L );
	lua_setfield( L, -2, T_PCK_NAME );
	luaopen_t_enc( L );
	lua_setfield( L, -2, T_ENC_NAME );
	luaopen_t_tst( L );
	lua_setfield( L, -2, T_TST_NAME );
	luaopen_t_oht( L );
	lua_setfield( L, -2, T_OHT_NAME );
	luaopen_t_set( L );
	lua_setfield( L, -2, T_SET_NAME );
	luaopen_t_wsk( L );
	lua_setfield( L, -2, T_WSK_NAME );
	luaopen_t_htp( L );
	lua_setfield( L, -2, T_HTP_NAME );
#ifdef T_NRY
	luaopen_t_nry( L );
	lua_setfield( L, -2, T_NRY_NAME );
#endif
	return 1;
}

