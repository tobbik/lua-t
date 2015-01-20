/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_htp_srv.c
 * \brief     OOP wrapper for HTTP Server operation
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include <string.h>               // memset
#include <time.h>                 // gmtime

#include "t.h"
#include "t_htp.h"


/** ---------------------------------------------------------------------------
 * Creates an T.Http.Server.
 * \param    luaVM    lua state.
 * \lparam   Loop for the Server.
 * \lparam   function WSAPI style request handler.
 * \return integer # of elements pushed to stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_New( lua_State *luaVM )
{
	struct t_htp_srv *s;
	struct t_ael     *l;

	if (lua_isfunction( luaVM, -1 ) && (l = t_ael_check_ud( luaVM, -2, 1 )))
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
	s->nw = time( NULL );
	t_htp_srv_setnow( s, 1 );

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
	void *ud = luaL_testudata( luaVM, pos, "T.Http.Server" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Http.Server` expected" );
	return (struct t_htp_srv *) ud;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_srv struct and return it
 * \param  luaVM    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_srv*  pointer to the struct.
 * --------------------------------------------------------------------------*/
void
t_htp_srv_setnow( struct t_htp_srv *s, int force )
{
	time_t     nw = time( NULL );
	struct tm *tm_struct;

	if ((nw - s->nw) > 0 || force )
	{
		s->nw = nw;
		tm_struct = gmtime( &(s->nw) );
		/* Sun, 06 Nov 1994 08:49:37 GMT */
		strftime( s->fnow, 30, "%a, %d %b %Y %H:%M:%S %Z", tm_struct );
	}
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
	struct t_htp_srv   *s     = (struct t_htp_srv *) lua_touserdata( luaVM, 1 );
	struct sockaddr_in *si_cli;
	struct t_sck       *c_sck;
	struct t_sck       *s_sck;
	struct t_ael       *ael;    // AELoop
	struct t_htp_msg   *m;      // new message userdata

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->sR );
	s_sck = t_sck_check_ud( luaVM, -1, 1 );

	t_sck_accept( luaVM, 2 );   //S: srv,ssck,csck,cip
	c_sck  = t_sck_check_ud( luaVM, -2, 1 );
	si_cli = t_ipx_check_ud( luaVM, -1, 1 );
	t_sck_reuseaddr( luaVM, c_sck );

	//prepare the ael_fd->wR table on stack
	lua_newtable( luaVM );
	lua_pushcfunction( luaVM, t_htp_msg_rsp );    //S: s,ss,cs,ip,rt,rsp
	lua_rawseti( luaVM, -2, 1 );

	lua_pushcfunction( luaVM, lt_ael_addhandle );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->lR );
	ael = t_ael_check_ud( luaVM, -1, 1 );      //S: s,ss,cs,ip,rt,add(),ael
	lua_pushvalue( luaVM, -5 );                // push client socket
	lua_pushboolean( luaVM, 1 );               // yepp, that's for reading
	lua_pushcfunction( luaVM, t_htp_msg_rcv ); //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv
	m = t_htp_msg_create_ud( luaVM, s );
	lua_pushvalue( luaVM, -1 );                // preserve to put onto rsp function
	lua_rawseti( luaVM, -8, 2 );
	lua_newtable( luaVM );                     // create msg proxy table
	lua_pushstring( luaVM, "socket" );
	lua_pushvalue( luaVM, -11 ); //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg,proxy,"socket",cs
	lua_rawset( luaVM, -3 );
	lua_pushstring( luaVM, "ip" );
	lua_pushvalue( luaVM, -10 );  //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg,proxy,"ip",ip
	lua_rawset( luaVM, -3 );
	m->pR  = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	m->sck = c_sck;

	// actually put it onto the loop  //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg
	t_stackDump(luaVM);
	lua_call( luaVM, 5, 0 );          // execute ael:addhandle(cli,tread,rcv,msg)
	t_stackDump(luaVM);
	// Here the t_ael_fd is all allocated and set up for reading.  This will put
	// the response writer method on the ->wR reference for faster processing
	// since an HTTP msg will bounce back and forth between reading and writing.
	//S: s,ss,cs,ip,rt,(true/false)
	//lua_pop( luaVM, 1 );                           // TODO: pop true or false
	ael->fd_set[ m->sck->fd ]->wR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
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
	lua_pushcfunction( luaVM, lt_ael_addhandle ); //S: srv,sc,addhandle
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, s->lR );
	t_ael_check_ud( luaVM, -1, 1 );
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
 * __gc of a T.Http.Server instance.
 * \param   luaVM      The lua state.
 * \lparam  t_htp_srv  The Server instance user_data.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_htp_srv__gc( lua_State *luaVM )
{
	struct t_htp_srv *s = (struct t_htp_srv *) luaL_checkudata( luaVM, 1, "T.Http.Server" );

	// t_sck_close( luaVM, s->sck );     // segfaults???
	luaL_unref( luaVM, LUA_REGISTRYINDEX, s->sR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, s->aR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, s->lR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, s->rR );

	printf("GC'ed HTTP Server...\n");

	return 0;
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
LUAMOD_API int
luaopen_t_htp_srv( lua_State *luaVM )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( luaVM, "T.Http.Server" );
	luaL_newlib( luaVM, t_htp_srv_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_htp_srv__len );
	lua_setfield( luaVM, -2, "__len");
	lua_pushcfunction( luaVM, lt_htp_srv__gc );
	lua_setfield( luaVM, -2, "__gc");
	lua_pushcfunction( luaVM, lt_htp_srv__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack
	// T.Http.Server class
	luaL_newlib( luaVM, t_htp_srv_cf );
	luaL_newlib( luaVM, t_htp_srv_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

