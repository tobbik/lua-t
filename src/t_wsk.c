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
#include "t_htp.h"
#include "t_buf.h"



/** ---------------------------------------------------------------------------
 * Creates a WebSocket.
 * \param    L    lua state.
 * \lparam   subp     sub-protocol.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
int lt_wsk_New( lua_State *L )
{
	struct t_wsk  *ws;
	//size_t lp;
	//char         *subp = luaL_checklstring( L, 1, &lp );
	ws = t_wsk_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * construct a WebSocket
 * \param   L  The lua state.
 * \lparam  CLASS  table WebSocket
 * \lparam  string sub protocol
 * \lreturn struct t_ws userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_wsk__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_wsk_New( L );
}


/**--------------------------------------------------------------------------
 * create a t_ws and push to LuaStack.
 * \param   L  The lua state.
 *
 * \return  struct t_ws*  pointer to the  xt_pack struct
 * --------------------------------------------------------------------------*/
struct t_wsk *t_wsk_create_ud( lua_State *L )
{
	struct t_wsk  *ws;
	ws = (struct t_wsk *) lua_newuserdata( L, sizeof( struct t_wsk ));

	luaL_getmetatable( L, T_WSK_NAME );
	lua_setmetatable( L, -2 );
	return ws;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an xt_ws struct and return it
 * \param  L    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct xt_ws* pointer to xt_ws struct
 * --------------------------------------------------------------------------*/
struct t_wsk *t_wsk_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_checkudata( L, pos, T_WSK_NAME );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_WSK_NAME"` expected" );
	return (struct t_wsk *) ud;
}


/**--------------------------------------------------------------------------
 * reads a message
 * \param   L lua Virtual Machine.
 * \param   struct xt_ws.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
int t_wsk_rmsg( lua_State *L, struct t_wsk *ws )
{
	char  buffer[ BUFSIZ ];
	int   rcvd;

	// TODO: Idea
	// WS is in a state -> empty, receiving, sending
	// negotiate to read into the buffer initially or into the luaL_Buffer
	rcvd = t_sck_recv( L, ws->sck, &(buffer[ 0 ]), BUFSIZ );
	return rcvd;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a packer instance.
 * \param   L      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_wsk__tostring( lua_State *L )
{
	struct t_wsk *ws = t_wsk_check_ud( L, 1, 1 );

	lua_pushfstring( L, T_WSK_NAME": %p", ws );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   L      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_wsk__len( lua_State *L )
{
	//struct t_wsk *wsk = t_wsk_check_ud( L, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( L, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_wsk_fm [] = {
	{ "__call",        lt_wsk__Call},
	{ NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_wsk_cf [] = {
	{ "new",           lt_wsk_New},
	{ NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_wsk_m [] = {
	{ "__len",           lt_wsk__len },
	{ "__tostring",      lt_wsk__tostring },
	{ NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_t_wsk (lua_State *L)
{
	// T.Websocket instance metatable
	luaL_newmetatable( L, T_WSK_NAME );
	luaL_setfuncs( L, t_wsk_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Websocket class
	luaL_newlib( L, t_wsk_cf );
	luaL_newlib( L, t_wsk_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

