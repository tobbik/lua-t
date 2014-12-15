/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_wsk.h
 * \brief     OOP wrapper for WebSocket opertaion
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset

#include "t.h"
#include "t_wsk.h"
#include "t_buf.h"



/** ---------------------------------------------------------------------------
 * Creates a WebSocket.
 * \param    luaVM    lua state.
 * \lparam   subp     sub-protocol.
 * \return integer # of elements pushed to stack.
 *  -------------------------------------------------------------------------*/
int lt_wsk_New( lua_State *luaVM )
{
	struct t_wsk  *ws;
	//size_t lp;
	//char         *subp = luaL_checklstring( luaVM, 1, &lp );
	ws = t_wsk_create_ud( luaVM );
	return 1;
}


/**--------------------------------------------------------------------------
 * construct a WebSocket
 * \param   luaVM  The lua state.
 * \lparam  CLASS  table WebSocket
 * \lparam  string sub protocol
 * \lreturn struct t_ws userdata.
 * \return  int    # of elements pushed to stack.
 * --------------------------------------------------------------------------*/
static int lt_wsk__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_wsk_New( luaVM );
}


/**--------------------------------------------------------------------------
 * create a t_ws and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_ws*  pointer to the  xt_pack struct
 * --------------------------------------------------------------------------*/
struct t_wsk *t_wsk_create_ud( lua_State *luaVM )
{
	struct t_wsk  *ws;
	ws = (struct t_wsk *) lua_newuserdata( luaVM, sizeof( struct t_wsk ));

	luaL_getmetatable( luaVM, "T.Websocket" );
	lua_setmetatable( luaVM, -2 );
	return ws;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an xt_ws struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct xt_ws* pointer to xt_ws struct
 * --------------------------------------------------------------------------*/
struct t_wsk *t_wsk_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Websocket" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Websocket` expected" );
	return (struct t_wsk *) ud;
}


/**--------------------------------------------------------------------------
 * reads a message
 * \param   luaVM lua Virtual Machine.
 * \param   struct xt_ws.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
int t_wsk_rmsg( lua_State *luaVM, struct t_wsk *ws )
{
	char  buffer[ BUFSIZ ];
	int   rcvd;

	// TODO: Idea
	// WS is in a state -> empty, receiving, sending
	// negotiate to read into the buffer initially or into the luaL_Buffer
	rcvd = t_sck_recv_tdp( luaVM, ws->sck, &(buffer[ 0 ]), BUFSIZ );
	return rcvd;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a packer instance.
 * \param   luaVM      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_wsk__tostring( lua_State *luaVM )
{
	struct t_wsk *ws = t_wsk_check_ud( luaVM, 1, 1 );

	lua_pushfstring( luaVM, "T.Websocket: %p", ws );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   luaVM      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_wsk__len( lua_State *luaVM )
{
	//struct t_wsk *wsk = t_wsk_check_ud( luaVM, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( luaVM, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_wsk_fm [] = {
	{"__call",        lt_wsk__Call},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_wsk_cf [] = {
	{"new",           lt_wsk_New},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_wsk_m [] = {
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_t_wsk (lua_State *luaVM)
{
	// T.Buffer instance metatable
	luaL_newmetatable( luaVM, "T.Websocket" );
	luaL_newlib( luaVM, t_wsk_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_wsk__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_wsk__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	// T.Websocket class
	luaL_newlib( luaVM, t_wsk_cf );
	luaL_newlib( luaVM, t_wsk_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

