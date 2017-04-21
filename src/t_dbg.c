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

#include "t.h"
#include "t_dbg.h"


/** -------------------------------------------------------------------------
 * Formats an item on the stack.  Pushes formatted string to last position.
 * \param  L        The Lua intepretter object.
 * \param  int      position stack element to format.
 * \param  int/bool use _tostring if 1 use tostring( ), else .
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
	idx = (idx < 0) ? lua_gettop( L ) + idx + 1 : idx;  // get absolute stack position
	for ( ;idx <= last; idx++)
	{
		t_fmtStackItem( L, idx, no_tostring );
		printf( "%s  ", lua_tostring( L, -1 ) ); // print serialized item and separator
		lua_pop( L, 1 );                         // pop serialized item
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

