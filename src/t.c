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


/**--------------------------------------------------------------------------
 * Read type name from stack and overwrite it with type value.
 * \param   L        Lua state.
 * \param   pos      Position of name on Lua stack.
 * \param   dft      char*; Default type name if pos is empty.
 * \param   types    array of t_typ*;
 * \lparam  string   Name of type.
 * \lreturn integer  Value of type.
 * --------------------------------------------------------------------------*/
void
t_getTypeByName( lua_State *L, int pos, const char *dft, const struct t_typ *types )
{
	const char *name  = (NULL == dft )
	                     ? luaL_checkstring( L, pos )
	                     : luaL_optstring( L, pos, dft );
	int         i     = 0;
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	while (NULL != types[i].name)
	{
		if (0 == strcmp( name, types[i].name ))
			break;
		i++;
	}
	if (NULL == types[i].name)
		lua_pushnil( L );
		//return luaL_error( L, "illegal type `%s` in argument %d", name, pos );
	else
		lua_pushinteger( L, types[i].value );
	if (lua_gettop( L ) > pos)
		lua_replace( L, pos );
}


/**--------------------------------------------------------------------------
 * Read type value from stack and overwrite it with type name.
 * \param   L        Lua state.
 * \param   pos      Position of name on Lua stack.
 * \param   dft      int; Default type value if pos is empty.
 * \param   types    array of t_typ*;
 * \lparam  string   Value of type.
 * \lreturn integer  Name of type.
 * --------------------------------------------------------------------------*/
void
t_getTypeByValue( lua_State *L, int pos, const int dft, const struct t_typ *types )
{
	const int   val = (dft < 1)
	                     ? luaL_checkinteger( L, pos )
	                     : luaL_optinteger( L, pos, dft );
	int         i   = 0;
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;

	while (NULL != types[i].name)
	{
		if (types[i].value == val)
			break;
		i++;
	}
	if (NULL == types[i].name)
		lua_pushnil( L );
		//luaL_error( L, "illegal value %d", value );
	else
		lua_pushstring( L, types[i].name );
	if (lua_gettop( L ) > pos)
		lua_replace( L, pos );
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
		else          lua_pushfstring( L, " (%s)\n", strerror( errno ) );
		lua_concat( L, 3 );
		return lua_error( L );
	}
}

