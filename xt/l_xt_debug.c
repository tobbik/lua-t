#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#include "l_xt.h"
#include "l_xt_hndl.h"
#define STDIN 0

/** -------------------------------------------------------------------------
 * \brief    Takes a lua file handle and wraps it in an xt handle
 * \param    luaVM  The lua state.
 * \lparam   Lua FileHandle
 * \lreturn  xt_hndl
 * \return   leave 1 item on stack
 *-------------------------------------------------------------------------*/
static int l_file2xt_hndl(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	FILE           **fPtr = (FILE **) luaL_checkudata(luaVM, 1, LUA_FILEHANDLE);

	hndl = (struct xt_hndl*) lua_newuserdata(luaVM, sizeof(struct xt_hndl));
	hndl->hd_t = FIL;
#ifdef WIN32
	hndl->fd   = _fileno(*fPtr);
#else
	hndl->fd   = fileno(*fPtr);
#endif

	luaL_getmetatable(luaVM, "L.net.Socket");
	lua_setmetatable(luaVM, -2);

	return( 1 );
}


/** -------------------------------------------------------------------------
 * \brief    Takes a lua file handle and wraps it in an xt handle
 * \param    luaVM  The lua state.
 * \lparam   Lua FileHandle
 * \lreturn  xt_hndl
 * \return   leave 1 item on stack
 *-------------------------------------------------------------------------*/
static int process_stdin ( lua_State *luaVM ) {
	char            buff[256];
	int             error;

	fgets(buff, sizeof(buff), stdin);
	error = luaL_loadbuffer(luaVM, buff, strlen(buff), "line") ||
	        lua_pcall(luaVM, 0, 0, 0);
	if (error) {
		fprintf(stderr, "%s", lua_tostring(luaVM, -1));
		lua_pop(luaVM, 1);  /* pop error message from the stack */
	}

	return 0;
}


/** -------------------------------------------------------------------------
 * \brief   select from open handles and trap STDIN.
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
	struct xt_hndl  *hndl;
	struct timeval  *tv;
	int              rnum, wnum, readsocks, i/*, rp*/;
	int              rset=1;//, wset=1;

	wnum = -1;
	rnum = STDIN;
	make_fdset(luaVM, 1, &rfds, &rnum);
	FD_SET(STDIN, &rfds); /* add STDIN to connset */
	//make_fdset(luaVM, 2, &wfds, &wnum);
	FD_ZERO(&wfds);

	if (lua_isuserdata(luaVM, 3)) {
		tv = xt_time_check_ud(luaVM, 3);
	}

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		(lua_isuserdata(luaVM, 3)) ? tv : NULL
	);

	// Process stdin separately
	if FD_ISSET( STDIN, &rfds ) {
		// put into reads
		process_stdin(luaVM);
		readsocks--;
	}
	lua_createtable(luaVM, 0, 0);     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	//lua_createtable(luaVM, 0, 0);
	for ( i=1 ; ; i++ ) {
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last index is found
		if ( lua_isnil(luaVM, -1) ) {
			lua_pop(luaVM, 1);
			break;
		}
		// get the values and clear the stack
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds) {
			// put into reads
			lua_rawseti(luaVM, -2, rset++);
			readsocks--;
		}

		//else if FD_ISSET( sock->socket, &wfds) {
		//	// put into writes
		//	lua_rawseti(luaVM, -2, wset++);
		//	readsocks--;
		//}
		else {
			lua_pop(luaVM, 1);   // remove the socket from the stack
		}
	}
	//lua_pop(luaVM, -1);   // remove the table from the stack

	return (1);
}


/**
 * \brief      the net library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_debug_lib [] =
{
	{"select",      l_select_handle},
	{"wrapFile",    l_file2xt_hndl},
	{NULL,          NULL}
};


/**
 * \brief     Export the net library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_debug (lua_State *luaVM)
{
	luaL_newlib (luaVM, l_debug_lib);
	return 1;
}

