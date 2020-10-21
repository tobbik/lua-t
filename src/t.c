/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t.c
 * \brief     Global setup for lua-t library.
 *            Exports sub libraries. Defines global helper functions.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdio.h>
#include <string.h>     // strerror,strrchr
#include <errno.h>      // errno
#include <stdint.h>     // printf in helpers

#include "t.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

/** -------------------------------------------------------------------------
 * Get a value from a table loaded in the registry.
 * The key must be on the stack and passed by position on stack.  The key on
 * the stack will be replaced by the value, or nil if nothing was found.
 * Iterates through names as subtables. To access Net.Socket.Protocol pass
 * "Net", "Socket", "Protocol"
 * \param  L    Lua state.
 * \param  len  Int. How many sub-table names have been passed.
 * \param  pos  Position of table key on Lua stack.
 * \param  ...  variable arguments of table names.
 * \return bool Was the value found
 *-------------------------------------------------------------------------*/
int
t_getLoadedValue( lua_State *L, size_t len, int pos, ... )
{
	va_list          argp;
	unsigned int     argc = 0;
	pos = t_getAbsPos( L, pos );
	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	va_start( argp, pos );
	for (argc=0; argc<len; argc++)
	{
		// push next sub-table on stack
		//const char *field = va_arg( argp, const char* );
		//lua_getfield( L, -1, field );
		lua_getfield( L, -1, va_arg( argp, const char* ) );
						/* debug start
						printf("                 ------------------------ NEXT FIELD: %s\n", field );
						lua_pushnil(L);  // first key
						while (lua_next(L, -2) != 0) {
							t_stackDump(L);
						    // removes 'value'; keeps 'key' for next iteration
							lua_pop(L, 1);
						}
						end debug */
	}
	va_end( argp );
	lua_pushvalue( L, pos );   //S:… key … _LD ___ ___ ___ key
	lua_rawget( L, -2 );       //S:… key … _LD ___ ___ ___ val
	lua_replace( L, pos );     //S:… val … _LD ___ ___ ___
	lua_pop( L, argc+1 );      //S:… fml …
	return (! lua_isnil( L, pos ));
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
		if (0 != errno) return luaL_error( L, strerror( errno ) );
		else            return luaL_error( L, "Unknown Error" );
	}
	else
	{
		va_start( argp, fmt );
		luaL_where( L, 1 );
		lua_pushvfstring( L, fmt, argp );
		va_end( argp );
		if (0==errno) lua_pushstring( L, "\n" );
		else          lua_pushfstring( L, " (%s)\n", strerror( errno ) );
		lua_concat( L, 3 );
		return lua_error( L );
	}
}


/** -------------------------------------------------------------------------
 * Type Error formatter, adapted from lauxlib.c
 * \param  L      Lua state.
 * \param  pos    int; position of wrong typed object on stack.
 * \param  tname  Name of expected type.
 * \return  int   # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int t_typeerror( lua_State *L, int pos, const char *tname )
{
	const char *msg;
	const char *typearg;  /* name for the type of the actual argument */
	if (LUA_TSTRING == luaL_getmetafield( L, pos, "__name" ))
		typearg = lua_tostring(L, -1);  /* use the given type name */
	else if (LUA_TLIGHTUSERDATA == lua_type(L, pos))
		typearg = "light userdata";  /* special name for messages */
	else
		typearg = luaL_typename( L, pos );  /* standard name */
	msg = lua_pushfstring( L, "%s expected, got %s", tname, typearg );
	return luaL_argerror( L, pos, msg );
}

