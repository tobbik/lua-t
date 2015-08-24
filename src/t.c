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
#include "t_sck.h"


/**
 * Formats the elements of the stack.
 * \param  L     The Lua intepretter object.
 * \param  int   first stack element to format.
 * \param  int   last stack element to format.
 */
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

/**
 * Prints a list of items on the lua stack.
 * \param  L The Lua state.
 */
void
t_stackdump ( lua_State *L )
{
	int top = lua_gettop( L );
	printf( "STACK[%d]:   ", top );
	t_stackPrint( L, 1, top );
	printf( "\n" );  // end the listing
}


/**
 * Returns an error string to the LUA script.
 * Expands luaL_error by errno support.
 * \param  L The Lua intepretter object.
 * \param  info  Error string.
 * \param  ...   variable arguments to fmt
 * \return int  # of values pushed onto the stack.
 */
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


/**
 * Reverse the order of bytes for a 16 bit unsigned integer
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 */
inline uint16_t
Reverse2Bytes( uint16_t value )
{
	return (
		(value & 0xFFU)   << 8 |
		(value & 0xFF00U) >> 8
	);
}


/**
 * Reverse the order of bytes for a 32 bit unsigned integer
 * \param     value Unsigned 32 bit integer
 * \return    integer with the opposite Endianness
 */
inline uint32_t
Reverse4Bytes( uint32_t value )
{
	return (value & 0x000000FFU) << 24 |
			 (value & 0x0000FF00U) << 8  |
			 (value & 0x00FF0000U) >> 8  |
			 (value & 0xFF000000U) >> 24;
}


/**
 * Reverse the order of bytes for a 64 bit unsigned integer.
 * \param   value Unsigned 64 bit integer
 * \return  Integer with the opposite Endianness
 */
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


/** -------------------------------------------------------------------------
 * Helper to take sockets from Lua tables to FD_SET.
 * Itertates over the table puls out the socket structs and adds the actual
 * sockets to the fd_set.
 * \param   L  The lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
void
make_fdset( lua_State *L, int stack_pos, fd_set *collection, int *max_hndl )
{
	struct t_sck  *hndl;

	FD_ZERO( collection );
	// empty table == nil
	if (lua_isnil( L, stack_pos) )
	{
		return;
	}
	// only accept tables
	luaL_checktype( L, stack_pos, LUA_TTABLE );
	// TODO: check table for len==0 and return

	// adding fh to FD_SETs
	lua_pushnil( L );
	while (lua_next( L, stack_pos ))
	{
		hndl = (struct t_sck *) lua_touserdata( L, -1 );
		FD_SET( hndl->fd, collection );
		*max_hndl = (hndl->fd > *max_hndl) ? hndl->fd : *max_hndl;
		lua_pop( L, 1 );   // remove the socket, keep key for next()
	}
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   L  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_selecthandle( lua_State *L )
{
	fd_set          rfds, wfds;
	struct t_sck   *hndl;
	int             rnum, wnum, readsocks, i/*, rp*/;
	int             rset=1, wset=1;

	wnum = -1;
	rnum = -1;
	make_fdset( L, 1, &rfds, &rnum );
	make_fdset( L, 2, &wfds, &wnum );

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create result table
	lua_createtable( L, 0, 0 );     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti( L, 1, i );
		// in table this is when the last index is found
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );
			break;
		}
		hndl = (struct t_sck *) lua_touserdata( L, -1 );
		if FD_ISSET( hndl->fd, &rfds )
		{
			lua_rawseti( L, -3, rset++ );
			readsocks--;
			if (0 == readsocks)
				break;
		}
		else
			lua_pop( L, 1 );
	}
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti( L, 2, i );
		// in table this is when the last index is found
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );
			break;
		}
		hndl = (struct t_sck *) lua_touserdata( L, -1 );
		if FD_ISSET( hndl->fd, &wfds )
		{
			lua_rawseti( L, -2, wset++ );
			readsocks--;
			if (0 == readsocks)
				break;
		}
		else
			lua_pop( L, 1 );
	}
	return 2;
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   L  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_selecthandlek( lua_State *L )
{
	fd_set          rfds, wfds;
	struct t_sck   *hndl;
	int              rnum, wnum, readsocks /*, rp*/;

	wnum = -1;
	rnum = -1;
	make_fdset( L, 1, &rfds, &rnum );
	make_fdset( L, 2, &wfds, &wnum );

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create result table
	//lua_createtable(L, 0, 0);
	lua_pushnil( L );
	while (lua_next( L, 1 ))
	{
		hndl = (struct t_sck *) lua_touserdata( L, -1 );
		if FD_ISSET( hndl->fd, &rfds )
		{
			//TODO: Find better way to preserve key for next iteration
			lua_pushvalue( L, -2 );
			lua_pushvalue( L, -2 );
			lua_rawset( L, -5 );
			if (0 == --readsocks) {
				lua_pop( L, 2 );
				break;
			}
		}
		lua_pop( L, 1 );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Object methods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg l_t_lib [] =
{
	// t-global methods
	{ "select",      lt_selecthandle },
	{ "selectK",     lt_selecthandlek },
	{ NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T library onto the stack
 * \param   L      The lua state.
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
	luaopen_t_sck( L );
	lua_setfield( L, -2, "Socket" );
	luaopen_t_ipx( L );
	lua_setfield( L, -2, "Ip" );
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

