/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_dbg.c
 * \brief     Debug/Development helper functions for lua-t library.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>
#include <string.h>     // strerror,strrchr
#include <errno.h>      // errno
#include <stdint.h>     // printf in helpers

#include "t_dbg.h"


/** -------------------------------------------------------------------------
 * Formats an item on the stack.  Pushes formatted string to last position.
 * \param  L        The Lua intepretter object.
 * \param  int      position stack element to format.
 * \param  int/bool use __tostring if 1 use tostring( )
 *-------------------------------------------------------------------------*/
void
t_fmtStackItem( lua_State *L, int idx, int tostring )
{
	int         t;
	size_t      l;
	const char *str;
	if (tostring && luaL_callmeta( L, idx, "__tostring" ))
	{
		printf( "%s ", lua_tostring( L, -1 ) );
		lua_pop( L, 1 );
	}
	else
	{
		t = lua_type( L, idx );
		switch (t)
		{
			case LUA_TNIL:       // nil
				printf( "nil " );
				break;

			case LUA_TSTRING:    // strings
				str = luaL_checklstring( L, idx, &l );
				if (l > 25)
					printf( "`%.13s ... %s` ", str, str+l-13 );
				else
					printf( "`%s` ", str );
				break;

			case LUA_TBOOLEAN:   // booleans
				printf( "%s ", lua_toboolean( L, idx ) ? "true" : "false" );
				break;

			case LUA_TNUMBER:    // numbers
				if (lua_isinteger( L, idx ))
					printf( "%lld ", lua_tointeger( L, idx ) );
				else
					printf( "%f ", lua_tonumber( L, idx ) );
				break;

			case LUA_TUSERDATA:  // userdata
				if (luaL_getmetafield( L, idx, "__name" ))  // does it have a metatable?
				{
					printf( "u.%s ", lua_tostring( L, -1 ) );
					lua_pop( L, 1 );
				}
				else
					printf( "ud " );
				break;

			case LUA_TTABLE:    // tables
				if (luaL_getmetafield( L, idx, "__name" ))  // does it have a metatable?
				{
					printf( "t.%s ", lua_tostring( L, -1 ) );
					lua_pop( L, 1 );
				}
				else
					printf( "table " );
				break;

			default:            // other values
				printf( "%s ", lua_typename( L, t ) );
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
t_stackPrint( lua_State *L, int pos, int last, int no_tostring )
{
	pos = lua_absindex( L, pos );
	for ( ;pos <= last; pos++)
		t_fmtStackItem( L, pos, no_tostring );
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

