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
 * Prints a binary representation of a number as in memory.
 * \param  char array to print.
 *-------------------------------------------------------------------------*/
void
t_printCharBin( volatile char *b, size_t sz )
{
	size_t n, x;

	for (n=0; n<sz; n++)
	{
		for (x=CHAR_BIT; x>0; x--)
			printf( "%d", (b[n] >> (x-1) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a binary representation of a number.
 * \param  int integer to print.
 *-------------------------------------------------------------------------*/
void
t_printIntBin( lua_Unsigned i )
{
	size_t n,x;

	for (n=sizeof( lua_Unsigned )*CHAR_BIT; n>0; n-=CHAR_BIT)
	{
		for (x=CHAR_BIT; x>0; x--)
			printf( "%d", ((i >> (n-CHAR_BIT+x-1)) & 0x01) ? 1 : 0 );
		printf( " " );
	}
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a hexadecimal representation of a number as in memory.
 * \param  char array to print.
 *-------------------------------------------------------------------------*/
void
t_printCharHex( volatile char *b, size_t sz )
{
	size_t n;

	for (n=0; n<sz; n++)
		printf( "%02X ", b[n] & 0X00000000000000FF);
	printf( "\n" );
}

/** -------------------------------------------------------------------------
 * Prints a haxadecimal representation of a number.
 * \param  int integer to print.
 *-------------------------------------------------------------------------*/
void
t_printIntHex( lua_Unsigned i )
{
	size_t n;
	for (n=sizeof(lua_Unsigned); n>0; n--)
		printf( "%02llX ", (i >> (n-1)*CHAR_BIT) & 0X00000000000000FF );
	printf("\n");
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
	int is_of_type = 0;
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
		{
			is_of_type = 1;
		}
		lua_pop( L, 2 );
	}
	else
	{
		if (check)
			luaL_error( L, "wrong argument, `%s` expected", type );
	}
	return is_of_type;
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

