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
 * Get the proxy table index onto the stack.
 * Objects that use a proxy table such as T.Test or T.OrderedHashTable have an
 * internal tracking or proxy table which holds the data so that __index and
 * __newindex operations can be done without hassle.  Within the object, the
 * tracking table is indexed by an empty table:
 * T.proxyTableIndex = {}    -- a globally (to T) defined empty table
 * oht = Oht()               -- oht is the table instance with the metamethods
 * oht[ T.proxyTableIndex ]  -- this is the table that actually contains the
 *                           -- values which are accessd and controlled by oht
 *                              __index and __newindex metamethods
 * \param   L    Lua state.
 * \lreturn {}   The empty table used as index for proxy tables.
 *-------------------------------------------------------------------------*/
void
t_getProxyTableIndex( lua_State *L )
{
	luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
	lua_getfield( L, -1, "t.core" );
	lua_getfield( L, -1, T_PROXYTABLEINDEX );      //S: … loaded t {}
	lua_insert( L, -3 );                           //S: … {} loaded t
	lua_pop( L, 2 );                               //S: … {}
}


/** -------------------------------------------------------------------------
 * For objects utilizing a proxy table replace object on stack with table.
 * \param  L        The Lua intepretter object.
 * \param  pos      int; position stack element to format.
 *-------------------------------------------------------------------------*/
void
t_getProxyTable( lua_State *L, int pos )
{
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;  // get absolute stack position
	t_getProxyTableIndex( L );           //S: … obj … {}
	lua_rawget( L, pos );                //S: … obj … tbl
	lua_replace( L, pos );               //S: … tbl …
}


/** -------------------------------------------------------------------------
 * For objects utilizing a proxy table replace object on stack with table.
 * \param   L        The Lua intepretter object.
 * \lparam  obj      Object to retrieve value from.
 * \lparam  key      key for __index.
 *-------------------------------------------------------------------------*/
int
t_getFromProxyTable( lua_State *L )
{
	t_getProxyTable( L, -2 );            //S: … tbl key
	lua_rawget( L, -2 );                 //S: … tbl val
	lua_replace( L, -2 );                //S: … val
	return 1;
}


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


/**--------------------------------------------------------------------------
 * Check a table on position pos of the stack for being of type "T_TYP_TYPE"
 * \param   L     Lua state.
 * \param   int   position on the stack.
 * \param   int   hard check; error out if not a T_TYP_TYPE.
 * \lparam  table T.* table to be tested.
 * --------------------------------------------------------------------------*/
int
t_checkTableType( lua_State *L, int pos, int check, const char *type )
{
	int isOfType = 0;
	if (! check && ! lua_istable( L, pos ))
		return isOfType;
	else
		luaL_checktype( L, pos, LUA_TTABLE );
	if (lua_getmetatable( L, pos ))              // does it have a metatable?
	{
		luaL_getmetatable( L, type );             // get correct metatable
		if (! lua_rawequal( L, -1, -2 ))          // not the same?
		{
			if (check)
				luaL_error( L, "wrong argument, `%s` expected", type );
		}
		else
			isOfType = 1;
		lua_pop( L, 2 );
	}
	else
	{
		if (check)
			luaL_error( L, "wrong argument, `%s` expected", type );
	}
	return isOfType;
}


/** -------------------------------------------------------------------------
 * Extended searchpath with current path of file, the run require.
 * \param    L     Lua state.
 * \lreturn  table imported library.
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

	return 1;
}


/** -------------------------------------------------------------------------
 * Return T_TYP_TYPE of a table/userdata or fallback to lua type.
 * \param   L      Lua state.
 * \lreturn string Name of type.
 *-------------------------------------------------------------------------*/
static int
lt_type( lua_State *L )
{
	int tt = luaL_getmetafield(L, 1, "__name");  // try name
	lua_pushfstring(L, "%s", (tt == LUA_TSTRING)
		? lua_tostring( L, -1 ) : luaL_typename( L, 1 ) );
	if (tt != LUA_TNIL)
		lua_remove(L, -2);  // remove '__name'
	return 1;
}


/**--------------------------------------------------------------------------
 * Object methods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg l_t_lib [] =
{
	// t-global methods
	  { "require"     ,   lt_require }
	, { "type"        ,   lt_type }
	, { NULL          ,   NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_core( lua_State *L )
{
	luaL_newlib( L, l_t_lib );
	luaopen_t_ael( L );
	lua_setfield( L, -2, T_AEL_IDNT );
	luaopen_t_tim( L );
	lua_setfield( L, -2, T_TIM_IDNT );
	luaopen_t_net( L );
	lua_setfield( L, -2, T_NET_IDNT );
	luaopen_t_buf( L );
	lua_setfield( L, -2, T_BUF_IDNT );
	luaopen_t_pck( L );
	lua_setfield( L, -2, T_PCK_IDNT );
	luaopen_t_enc( L );
	lua_setfield( L, -2, T_ENC_IDNT );
	luaopen_t_tst( L );
	lua_setfield( L, -2, T_TST_IDNT );
	luaopen_t_oht( L );
	lua_setfield( L, -2, T_OHT_IDNT );
	luaopen_t_htp( L );
	lua_setfield( L, -2, T_HTP_IDNT );
	luaopen_t_utl( L );
	lua_setfield( L, -2, T_UTL_IDNT );
#ifdef T_NRY
	luaopen_t_nry( L );
	lua_setfield( L, -2, T_NRY_IDNT );
#endif
	lua_newtable( L );
	lua_setfield( L, -2, T_PROXYTABLEINDEX );
	return 1;
}

