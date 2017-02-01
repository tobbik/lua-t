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
	for (n=CHAR_BIT*sizeof(lua_Unsigned); n>0; n--)
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

