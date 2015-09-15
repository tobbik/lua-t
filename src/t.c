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
#include <string.h>     // strerror
#include <errno.h>      // errno

#include "t.h"


/** -------------------------------------------------------------------------
 * Formats the elements of the stack.
 * \param  L     The Lua intepretter object.
 * \param  int   first stack element to format.
 * \param  int   last stack element to format.
 *-------------------------------------------------------------------------*/
void
t_stackPrint( lua_State *L, int i, int last )
{
	for ( ;i <= last; i++)
	{     // repeat for each level
		int t = lua_type( L, i );
		switch (t)
		{
			case LUA_TSTRING:    // strings
				printf( "`%s`", lua_tostring( L, i ) );
				break;

			case LUA_TBOOLEAN:   // booleans
				printf( lua_toboolean( L, i ) ? "true" : "false" );
				break;

			case LUA_TNUMBER:    // numbers
				printf( "%g", lua_tonumber( L, i ) );
				break;

			case LUA_TUSERDATA:    // userdata
				if (luaL_getmetafield( L, i, "__name" ))  // does it have a metatable?
				{
					printf( "u.%s", lua_tostring( L, -1 ) );
					lua_pop( L, 1 );
				}
				else
					printf( "ud" );
				break;

			case LUA_TTABLE:    // tables
				if (luaL_getmetafield( L, i, "__name" ))  // does it have a metatable?
				{
					printf( "t.%s", lua_tostring( L, -1 ) );
					lua_pop( L, 1);
				}
				else
					printf( "table" );
				break;

			default:             // other values
				printf( "%s", lua_typename( L, t ) );
				break;
		}
		printf( "   " );  // put a separator
	}
}

/** -------------------------------------------------------------------------
 * Prints a list of items on the lua stack.
 * \param  L The Lua state.
 *-------------------------------------------------------------------------*/
void
t_stackdump ( lua_State *L )
{
	int top = lua_gettop( L );
	printf( "STACK[%d]:   ", top );
	t_stackPrint( L, 1, top );
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
 * Reverse the order of bytes for a 16 bit unsigned integer
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 *-------------------------------------------------------------------------*/
inline uint16_t
Reverse2Bytes( uint16_t value )
{
	return (
		(value & 0xFFU)   << 8 |
		(value & 0xFF00U) >> 8
	);
}


/** -------------------------------------------------------------------------
 * Reverse the order of bytes for a 32 bit unsigned integer
 * \param     value Unsigned 32 bit integer
 * \return    integer with the opposite Endianness
 *-------------------------------------------------------------------------*/
inline uint32_t
Reverse4Bytes( uint32_t value )
{
	return (value & 0x000000FFU) << 24 |
			 (value & 0x0000FF00U) << 8  |
			 (value & 0x00FF0000U) >> 8  |
			 (value & 0xFF000000U) >> 24;
}


/** -------------------------------------------------------------------------
 * Reverse the order of bytes for a 64 bit unsigned integer.
 * \param   value Unsigned 64 bit integer
 * \return  Integer with the opposite Endianness
 *-------------------------------------------------------------------------*/
inline uint64_t
Reverse8Bytes( uint64_t value )
{
	return (value & 0x00000000000000FFUL) << 56 |
			 (value & 0x000000000000FF00UL) << 40 |
			 (value & 0x0000000000FF0000UL) << 24 |
			 (value & 0x00000000FF000000UL) << 8  |
			 (value & 0x000000FF00000000UL) >> 8  |
			 (value & 0x0000FF0000000000UL) >> 24 |
			 (value & 0x00FF000000000000UL) >> 40 |
			 (value & 0xFF00000000000000UL) >> 56;
}


/**--------------------------------------------------------------------------
 * Object methods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg l_t_lib [] =
{
	// t-global methods
	{ NULL,   NULL}
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
	lua_setfield( L, -2, "Loop" );
	luaopen_t_tim( L );
	lua_setfield( L, -2, "Time" );
	luaopen_t_net( L );
	lua_setfield( L, -2, "Net" );
	luaopen_t_buf( L );
	lua_setfield( L, -2, "Buffer" );
	luaopen_t_pck( L );
	lua_setfield( L, -2, "Pack" );
	luaopen_t_enc( L );
	lua_setfield( L, -2, "Encode" );
	luaopen_t_tst( L );
	lua_setfield( L, -2, "Test" );
	luaopen_t_wsk( L );
	lua_setfield( L, -2, "Websocket" );
	luaopen_t_htp( L );
	lua_setfield( L, -2, "Http" );
#ifdef T_NRY
	luaopen_t_nry( L );
	lua_setfield( L, -2, "Numarray" );
#endif
	return 1;
}

