/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt.c
 * \brief     Global wrapper and packer for xt library.
 *            Exports sub libraries. Defines global helper functions.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include <stdio.h>
#include <string.h>     // strerror
#include <errno.h>      // errno

#include "xt.h"
#include "xt_sck.h"



/**
 * \brief  Prints a list of items on the lua stack.
 * \param  luaVM The Lua state.
 */
void stackDump (lua_State *luaVM) {
	int i;
	int top = lua_gettop(luaVM);
	printf("LUA STACK[%d]:\t", top);
	for (i = 1; i <= top; i++) {     /* repeat for each level */
		int t = lua_type(luaVM, i);
		switch (t) {

			case LUA_TSTRING:    /* strings */
				printf("`%s'", lua_tostring(luaVM, i));
				break;

			case LUA_TBOOLEAN:   /* booleans */
				printf(lua_toboolean(luaVM, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:    /* numbers */
				printf("%g", lua_tonumber(luaVM, i));
				break;

			default:	            /* other values */
				printf("%s", lua_typename(luaVM, t));
				break;
		}
		printf("\t");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}


/**
 * \brief  Return an error string to the LUA script.
 *         Basically luaL_error extended by errno support
 * \param  luaVM The Lua intepretter object.
 * \param  info  Error string.
 * \param  ...   variable arguments to fmt
 */
int xt_push_error(lua_State *luaVM, const char *fmt, ...)
{
	va_list argp;
	if (NULL==fmt) {
		if (0==errno) return luaL_error (luaVM, strerror(errno));
		else          return luaL_error (luaVM, "Unknown Error");
	}
	else {
		va_start(argp, fmt);
		luaL_where(luaVM, 1);
		lua_pushvfstring(luaVM, fmt, argp);
		va_end(argp);
		if (0==errno) lua_pushstring (luaVM, "\n");
		else          lua_pushfstring (luaVM, ": %s\n", strerror(errno) );
		lua_concat(luaVM, 3);
		return lua_error(luaVM);
	}
}


/**
 * \brief     Used to reverse the order of bytes for a 16 bit unsigned integer
 *
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 */
inline uint16_t Reverse2Bytes(uint16_t value)
{
	return (
		(value & 0xFFU) << 8 |
		(value & 0xFF00U) >> 8
	);
}


/**
 * \brief     Used to reverse the order of bytes for a 32 bit unsigned integer
 *
 * \param     value Unsigned 32 bit integer
 * \return    integer with the opposite Endianness
 */
 inline uint32_t Reverse4Bytes(uint32_t value)
{
	return (value & 0x000000FFU) << 24 |
			 (value & 0x0000FF00U) << 8  |
			 (value & 0x00FF0000U) >> 8  |
			 (value & 0xFF000000U) >> 24;
}


/**
 * \brief   Used to reverse the order of bytes for a 64 bit unsigned integer
 *
 * \param   value Unsigned 64 bit integer
 * \return  Integer with the opposite Endianness
 */
 inline uint64_t Reverse8Bytes(uint64_t value)
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
 * \brief   Helper to take sockets from Lua tables to FD_SET
 *          Itertates over the table puls out the socket structs and adds the
 *          actual sockets to the fd_set
 * \param   luaVM  The lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
void make_fdset (lua_State *luaVM, int stack_pos, fd_set *collection, int *max_hndl)
{
	struct xt_sck  *hndl;

	FD_ZERO (collection);
	// empty table == nil
	if (lua_isnil (luaVM, stack_pos)) {
		return;
	}
	// only accept tables
	luaL_checktype (luaVM, stack_pos, LUA_TTABLE);
	// TODO: check table for len==0 and return

	// adding fh to FD_SETs
	lua_pushnil(luaVM);
	while (lua_next(luaVM, stack_pos))
	{
		hndl = (struct xt_sck *) lua_touserdata(luaVM, -1);
		FD_SET( hndl->fd, collection );
		*max_hndl = (hndl->fd > *max_hndl) ? hndl->fd : *max_hndl;
		lua_pop(luaVM, 1);   // remove the socket, keep key for next()
	}
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   luaVM  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int l_select_handle(lua_State *luaVM)
{
	fd_set           rfds, wfds;
	struct xt_sck   *hndl;
	int              rnum, wnum, readsocks, i/*, rp*/;
	int              rset=1;//, wset=1;

	wnum = -1;
	rnum = -1;
	make_fdset (luaVM, 1, &rfds, &rnum);
	make_fdset (luaVM, 2, &wfds, &wnum);

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable(luaVM, 0, 0);     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}
		hndl = (struct xt_sck *) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds)
		{
			lua_rawseti(luaVM, -2, rset++);
			readsocks--;
			if (0 == readsocks) {
				break;
			}
		}
		else {
			lua_pop(luaVM, 1);
		}
	}
	return (1);
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   luaVM  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int l_select_handle_k(lua_State *luaVM)
{
	fd_set           rfds, wfds;
	struct xt_sck   *hndl;
	int              rnum, wnum, readsocks /*, rp*/;

	wnum = -1;
	rnum = -1;
	make_fdset (luaVM, 1, &rfds, &rnum);
	make_fdset (luaVM, 2, &wfds, &wnum);

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable(luaVM, 0, 0);     // create result table
	//lua_createtable(luaVM, 0, 0);
	lua_pushnil(luaVM);
	while (lua_next(luaVM, 1))
	{
		hndl = (struct xt_sck *) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds)
		{
			//TODO: Find better way to preserve key for next iteration
			lua_pushvalue(luaVM, -2);
			lua_pushvalue(luaVM, -2);
			lua_rawset(luaVM, -5);
			if (0 == --readsocks) {
				lua_pop(luaVM, 2);
				break;
			}
		}
		lua_pop(luaVM, 1);
	}
	return (1);
}


/**
 * \brief      the (empty) xt library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_xt_lib [] =
{
	// xt-global methods
	{"select",      l_select_handle},
	{"selectK",     l_select_handle_k},
	{NULL,        NULL}
};


/**
 * \brief     Export the xt library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_xt( lua_State *luaVM )
{
	luaL_newlib( luaVM, l_xt_lib );
	luaopen_xt_lp( luaVM );
	lua_setfield( luaVM, -2, "Loop" );
	luaopen_xt_time( luaVM );
	lua_setfield( luaVM, -2, "Time" );
	luaopen_xt_sck( luaVM );
	lua_setfield( luaVM, -2, "Socket" );
	luaopen_xt_ip( luaVM );
	lua_setfield( luaVM, -2, "Ip" );
	luaopen_xt_buf( luaVM );
	lua_setfield( luaVM, -2, "Buffer" );
	luaopen_xt_pck( luaVM );
	lua_setfield( luaVM, -2, "Packer" );
	luaopen_xt_enc( luaVM );
	lua_setfield( luaVM, -2, "Encode" );
	luaopen_xt_test( luaVM );
	lua_setfield( luaVM, -2, "Test" );
	return 1;
}

