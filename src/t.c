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
	pos = lua_absindex( L, pos );
	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	va_start( argp, pos );
	for (argc=0; argc<len; argc++)
		lua_getfield( L, -1, va_arg( argp, const char* ) );
	va_end( argp );
	lua_pushvalue( L, pos );   //S:… key … _LD ___ ___ ___ key
	lua_rawget( L, -2 );       //S:… key … _LD ___ ___ ___ val
	lua_replace( L, pos );     //S:… val … _LD ___ ___ ___
	lua_pop( L, argc+1 );      //S:… fml …
	return (! lua_isnil( L, pos ));
}


/** -------------------------------------------------------------------------
 * Returns an error string to the Lua script.
 * Expands luaL_error by errno support which is useful when system functions
 * are called, eg. surrounding network functionality and others.
 * \param  L     The Lua intepretter object.
 * \param  fail  bool. Raise LuaError.
 * \param  ops   bool. Operation failed -> return false; else resource failed -> * return nil.
 * \param  fmt   Error string.
 * \param  ...   variable arguments to fmt
 * \return int   # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_push_error( lua_State *L, int fail, int ops, const char *fmt, ... )
{
	va_list argp;
	if (fail)
		luaL_where( L, 1 );
	else
	{
		if (ops)    // semantically an operation failed
			lua_pushboolean( L, 0==1 );
		else        // semantically the creation of a resource failed
			lua_pushnil( L );
	}
	if (NULL == fmt)
		lua_pushstring( L, (errno) ? "" : "Unknown Error" );
	else
	{
		va_start( argp, fmt );
		lua_pushvfstring( L, fmt, argp );
		va_end( argp );
	}
	if (errno) lua_pushfstring( L, " (%s)", strerror( errno ));
	lua_concat( L, (fail && errno) ? 3 : (fail || errno) ? 2 : 1 );
	return ((fail) ? lua_error( L ) : 2);
}


/** -------------------------------------------------------------------------
 * Type Error formatter, adapted from lauxlib.c
 * \param  L      Lua state.
 * \param  pos    int; position of wrong typed object on stack.
 * \param  tname  Name of expected type.
 * \return int    # of values pushed onto the stack.
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

