/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_srv.h
 * \brief     OOP wrapper for HTTP operation
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset

#include "t.h"
#include "t_htp.h"
#include "t_buf.h"


static int lt_htp_msg_read( lua_State *luaVM );


/** ---------------------------------------------------------------------------
 * Creates an T.Http.Server.
 * \param    luaVM    lua state.
 * \lparam   function WSAPI style request handler.
 * \return integer # of elements pushed to stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_New( lua_State *luaVM )
{
	struct t_htp_srv *s;
	struct t_elp     *l;

	if (lua_isfunction( luaVM, -1 ) && (l = t_elp_check_ud( luaVM, -2, 1 )))
	{
		s     = t_htp_srv_create_ud( luaVM );
		lua_insert( luaVM, -3 );
		s->rR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
		s->lR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	}
	else
		return t_push_error( luaVM, "T.Http.Server( func ) requires a function as parameter" );
	return 1;
}


/**--------------------------------------------------------------------------
 * construct an HTTP Server
 * \param   luaVM    The lua state.
 * \lparam  CLASS    table Http.Server
 * \lparam  T.Socket sub protocol
 * \lreturn userdata struct t_htp_srv* ref.
 * \return  int    # of elements pushed to stack.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_htp_srv_New( luaVM );
}


/**--------------------------------------------------------------------------
 * create a t_htp_srv and push to LuaStack.
 * \param   luaVM  The lua state.
 *
 * \return  struct t_htp_srv*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_srv
*t_htp_srv_create_ud( lua_State *luaVM )
{
	struct t_htp_srv *s;
	s = (struct t_htp_srv *) lua_newuserdata( luaVM, sizeof( struct t_htp_srv ));

	luaL_getmetatable( luaVM, "T.Http.Server" );
	lua_setmetatable( luaVM, -2 );
	return s;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_srv struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_srv*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_srv
*t_htp_srv_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Http.Server" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Server` expected" );
	return (struct t_htp_srv *) ud;
}


/**--------------------------------------------------------------------------
 * Accept a connection from a Http.Server listener.
 * Called anytime a new connection gets established.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_srv.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_accept( lua_State *luaVM )
{
	struct t_htp_srv   *s     = lua_touserdata( luaVM, 1 );
	struct sockaddr_in *si_cli;
	struct t_sck       *c_sck;
	struct t_sck       *s_sck;
	struct t_htp_msg   *c;      // new connection userdata

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->sR );
	s_sck = t_sck_check_ud( luaVM, -1, 1 );

	t_sck_accept( luaVM, 2 );   //S: srv,ssck,csck,cip
	c_sck  = t_sck_check_ud( luaVM, -2, 1 );
	si_cli = t_ipx_check_ud( luaVM, -1, 1 );

	lua_pushcfunction( luaVM, lt_elp_addhandle );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->lR );
	t_elp_check_ud( luaVM, -1, 1 );               //S: srv,ssck,csck,cip,addhandle,elp
	lua_pushvalue( luaVM, -4 );                   // push client socket
	lua_pushboolean( luaVM, 1 );                  // yepp, that's for reading
	lua_pushcfunction( luaVM, lt_htp_msg_read );  //S: srv,ssck,csck,cip,addhandle,elp,csck,true,read
	c      = (struct t_htp_msg *) lua_newuserdata( luaVM, sizeof( struct t_htp_msg ) );
	lua_pushvalue( luaVM, -8);   // repush csck and cip
	lua_pushvalue( luaVM, -8);   //S: srv,ssck,csck,cip,addhandle,elp,csck,true,read,cli,csk,cip
	c->aR  = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	c->sR  = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	c->rR  = s->rR;       // copy function reference
	c->fd  = c_sck->fd;
	c->hR  = LUA_NOREF;   // header is not received/parsed

	luaL_getmetatable( luaVM, "T.Http.Message" );
	lua_setmetatable( luaVM, -2 );
	lua_call( luaVM, 5, 0 );
	//TODO: Check if that returns true or false; if false resize loop
	return 0;
}


/**--------------------------------------------------------------------------
 * Puts the http server on a T.Loop to listen to incoming requests.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_srv.
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_listen( lua_State *luaVM )
{
	struct t_htp_srv   *s   = t_htp_srv_check_ud( luaVM, 1, 1 );
	struct t_sck       *sc  = NULL;
	struct sockaddr_in *ip  = NULL;

	// reuse socket:listen()
	t_sck_listen( luaVM, 2 );

	sc = t_sck_check_ud( luaVM, -2, 1 );
	ip = t_ipx_check_ud( luaVM, -1, 1 );
	s->aR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	lua_pushvalue( luaVM, -1 );
	s->sR = luaL_ref( luaVM, LUA_REGISTRYINDEX );

	// TODO: cheaper to reimplement functionality -> less overhead?
	lua_pushcfunction( luaVM, lt_elp_addhandle ); //S: srv,sc,addhandle
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->lR );
	t_elp_check_ud( luaVM, -1, 1 );
	lua_pushvalue( luaVM, -3 );                  /// push socket
	lua_pushboolean( luaVM, 1 );                 //S: srv,sc,addhandle,loop,sck,true
	lua_pushcfunction( luaVM, lt_htp_srv_accept );
	lua_pushvalue( luaVM, 1 );                  /// push server instance

	lua_call( luaVM, 5, 0 );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->aR );
	//TODO: Check if that returns tru or false; if false resize loop
	return  2;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of an T.Http.Server  instance.
 * \param   luaVM      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__tostring( lua_State *luaVM )
{
	struct t_htp_srv *s = t_htp_srv_check_ud( luaVM, 1, 1 );

	lua_pushfstring( luaVM, "T.Http.Server: %p", s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   luaVM      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__len( lua_State *luaVM )
{
	//struct t_wsk *wsk = t_wsk_check_ud( luaVM, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( luaVM, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of a T.Http.Message instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_msg  The Message instance user_data.
 * \lreturn string     formatted string representing T.Http.Message.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_htp_msg__tostring( lua_State *luaVM )
{
	struct t_htp_msg *c = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );

	lua_pushfstring( luaVM, "T.Http.Message: %p", c );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   luaVM      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lt_htp_msg__len( lua_State *luaVM )
{
	//struct t_wsk *wsk = t_wsk_check_ud( luaVM, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( luaVM, 5 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Handle incoming chunks from T.Http.Message socket.
 * Called if socket comes back from accept and anytime it returns from read.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_msg.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  integer number of values left on the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_msg_read( lua_State *luaVM )
{
	struct t_htp_msg *c = (struct t_htp_msg *) luaL_checkudata( luaVM, 1, "T.Http.Message" );
	char              buffer[ BUFSIZ ];
	int               rcvd;
	struct t_sck     *c_sck;

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->sR );
	c_sck = t_sck_check_ud( luaVM, -1, 1 );

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, c->rR );
	// TODO: Idea
	// WS is in a state -> empty, receiving, sending
	// negotiate to read into the buffer initially or into the luaL_Buffer
	rcvd = t_sck_recv_tdp( luaVM, c_sck, &(buffer[ 0 ]), BUFSIZ );
	printf( "%s\n", buffer );
	return rcvd;
}


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_srv_fm [] = {
	{"__call",        lt_htp_srv__Call},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief    the metatble for the module
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_srv_cf [] = {
	{"new",           lt_htp_srv_New},
	{NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_srv_m [] = {
	{"listen",        lt_htp_srv_listen},
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
LUAMOD_API int luaopen_t_htp_srv( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Server" );
	luaL_newlib( luaVM, t_htp_srv_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_srv__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_srv__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	// T.Http.Server class
	luaL_newlib( luaVM, t_htp_srv_cf );
	luaL_newlib( luaVM, t_htp_srv_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}



/**--------------------------------------------------------------------------
 * \brief      the buffer library definition
 *             assigns Lua available names to C-functions
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_msg_m [] = {
	{"read",        lt_htp_msg_read},
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
LUAMOD_API int luaopen_t_htp_msg( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Connection" );
	luaL_newlib( luaVM, t_htp_msg_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_msg__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_msg__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}


/**
 * \brief      the (empty) t library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_htp_lib [] =
{
	//{"crypt",     t_enc_crypt},
	{NULL,        NULL}
};


/**
 * \brief     Export the t_htp libray to Lua
 * \param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int
luaopen_t_htp( lua_State *luaVM )
{
	luaL_newlib( luaVM, t_htp_lib );
	luaopen_t_htp_srv( luaVM );
	lua_setfield( luaVM, -2, "Server" );
	luaopen_t_htp_msg( luaVM );
	return 1;
}

