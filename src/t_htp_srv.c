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
 * \param   L    lua state.
 * \lparam  Loop for the Server.
 * \lparam  function WSAPI style request handler.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_New( lua_State *L )
{
	struct t_htp_srv *s;
	struct t_ael     *l;

	if (lua_isfunction( L, -1 ) && (l = t_ael_check_ud( L, -2, 1 )))
	{
		s     = t_htp_srv_create_ud( L );
		lua_insert( L, -3 );
		s->rR = luaL_ref( L, LUA_REGISTRYINDEX );

		s->ael = l;
		s->lR  = luaL_ref( L, LUA_REGISTRYINDEX );
	}
	else
		return t_push_error( L, "T.Http.Server( func ) requires a function as parameter" );
	return 1;
}


/**--------------------------------------------------------------------------
 * construct an HTTP Server
 * \param   L    The lua state.
 * \lparam  CLASS    table Http.Server
 * \lparam  T.Socket sub protocol
 * \lreturn userdata struct t_htp_srv* ref.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_htp_srv_New( L );
}


/**--------------------------------------------------------------------------
 * create a t_htp_srv and push to LuaStack.
 * \param   L  The lua state.
 *
 * \return  struct t_htp_srv*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_srv
*t_htp_srv_create_ud( lua_State *L )
{
	struct t_htp_srv *s;
	s = (struct t_htp_srv *) lua_newuserdata( L, sizeof( struct t_htp_srv ));
	s->nw = time( NULL );
	t_htp_srv_setnow( s, 1 );

	luaL_getmetatable( L, "T.Http.Server" );
	lua_setmetatable( L, -2 );
	return s;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_srv struct and return it
 * \param  L    the Lua State
 * \param  pos      position on the stack
 *
 * \return  struct t_htp_srv*  pointer to the struct.
 * --------------------------------------------------------------------------*/
struct t_htp_srv
*t_htp_srv_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Http.Server" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Http.Server` expected" );
	return (struct t_htp_srv *) ud;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_htp_srv struct and return it
 * \param  L    the Lua State
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
		strftime( s->fnw, 30, "%a, %d %b %Y %H:%M:%S %Z", tm_struct );
	}
}


/**--------------------------------------------------------------------------
 * Accept a connection from a Http.Server listener.
 * Called anytime a new connection gets established.
 * \param   L     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_srv.
 * \param   pointer to the buffer to read from(already positioned).
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_accept( lua_State *L )
{
	struct t_htp_srv   *s     = (struct t_htp_srv *) lua_touserdata( L, 1 );
	struct sockaddr_in *si_cli;
	struct t_sck       *c_sck;
	struct t_sck       *s_sck;
	struct t_ael       *ael;    // AELoop
	struct t_htp_con   *c;      // new message userdata

	lua_rawgeti( L, LUA_REGISTRYINDEX, s->sR );
	s_sck = t_sck_check_ud( L, -1, 1 );

	t_sck_accept( L, 2 );   //S: srv,ssck,csck,cip
	c_sck  = t_sck_check_ud( L, -2, 1 );
	si_cli = t_ipx_check_ud( L, -1, 1 );
	t_sck_reuseaddr( L, c_sck );

	// prepare the ael_fd->wR table on stack
	lua_newtable( L );
	lua_pushcfunction( L, t_htp_con_rsp );    //S: s,ss,cs,ip,rt,rsp
	lua_rawseti( L, -2, 1 );

	lua_pushcfunction( L, lt_ael_addhandle );
	lua_rawgeti( L, LUA_REGISTRYINDEX, s->lR );
	ael = t_ael_check_ud( L, -1, 1 );      //S: s,ss,cs,ip,rt,add(),ael
	lua_pushvalue( L, -5 );                // push client socket
	lua_pushboolean( L, 1 );               // yepp, that's for reading
	lua_pushcfunction( L, t_htp_con_rcv ); //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv
	c = t_htp_con_create_ud( L, s );
	lua_pushvalue( L, -1 );                // preserve to put onto rsp function
	lua_rawseti( L, -8, 2 );
	lua_newtable( L );                     // create connection proxy table
	lua_pushstring( L, "socket" );
	lua_pushvalue( L, -11 );  //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg,proxy,"socket",cs
	lua_rawset( L, -3 );
	lua_pushstring( L, "ip" );
	lua_pushvalue( L, -10 );  //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg,proxy,"ip",ip
	lua_rawset( L, -3 );
	c->pR  = luaL_ref( L, LUA_REGISTRYINDEX );
	c->sck = c_sck;

	// actually put it onto the loop  //S: s,ss,cs,ip,rt,add(),ael,cs,true,rcv,msg
	lua_call( L, 5, 0 );          // execute ael:addhandle(cli,tread,rcv,msg)
	// Here the t_ael_fd is all allocated and set up for reading.  This will put
	// the response writer method on the ->wR reference for faster processing
	// since an HTTP msg will bounce back and forth between reading and writing.
	//S: s,ss,cs,ip,rt,(true/false)
	//lua_pop( L, 1 );                           // TODO: pop true or false
	ael->fd_set[ c->sck->fd ]->wR = luaL_ref( L, LUA_REGISTRYINDEX );
	return 0;
}


/**--------------------------------------------------------------------------
 * Puts the http server on a T.Loop to listen to incoming requests.
 * \param   L     lua Virtual Machine.
 * \lparam  userdata  struct t_htp_srv.
 * \lreturn value from the buffer a packers position according to packer format.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_htp_srv_listen( lua_State *L )
{
	struct t_htp_srv   *s   = t_htp_srv_check_ud( L, 1, 1 );
	struct t_sck       *sc  = NULL;
	struct sockaddr_in *ip  = NULL;

	// reuse socket:listen()
	t_sck_listen( L, 2 );

	sc     = t_sck_check_ud( L, -2, 1 );
	ip     = t_ipx_check_ud( L, -1, 1 );
	s->aR  = luaL_ref( L, LUA_REGISTRYINDEX );
	s->sck = sc;
	s->sR  = luaL_ref( L, LUA_REGISTRYINDEX );

	// TODO: cheaper to reimplement functionality -> less overhead?
	lua_pushcfunction( L, lt_ael_addhandle ); //S: srv,sc,addhandle
	lua_rawgeti( L, LUA_REGISTRYINDEX, s->lR );
	t_ael_check_ud( L, -1, 1 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, s->sR ); // get socket back on stack
	lua_pushboolean( L, 1 );                 //S: srv,sc,addhandle,loop,sck,true
	lua_pushcfunction( L, lt_htp_srv_accept );
	lua_pushvalue( L, 1 );                  /// push server instance

	lua_call( L, 5, 0 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, s->aR );
	//TODO: Check if that returns tru or false; if false resize loop
	return  2;
}


/**--------------------------------------------------------------------------
 * __tostring (print) representation of an T.Http.Server  instance.
 * \param   L      The lua state.
 * \lparam  xt_pack    the packer instance user_data.
 * \lreturn string     formatted string representing packer.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__tostring( lua_State *L )
{
	struct t_htp_srv *s = t_htp_srv_check_ud( L, 1, 1 );

	lua_pushfstring( L, "T.Http.Server: %p", s );
	return 1;
}


/**--------------------------------------------------------------------------
 * __len (#) representation of an instance.
 * \param   L      The lua state.
 * \lparam  userdata   the instance user_data.
 * \lreturn string     formatted string representing the instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_htp_srv__len( lua_State *L )
{
	//struct t_wsk *wsk = t_wsk_check_ud( L, 1, 1 );
	//TODO: something meaningful here?
	lua_pushinteger( L, 4 );
	return 1;
}

/**--------------------------------------------------------------------------
 * __gc of a T.Http.Server instance.
 * \param   L      The lua state.
 * \lparam  t_htp_srv  The Server instance user_data.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_htp_srv__gc( lua_State *L )
{
	struct t_htp_srv *s = (struct t_htp_srv *) luaL_checkudata( L, 1, "T.Http.Server" );

	// t_sck_close( L, s->sck );     // segfaults???
	luaL_unref( L, LUA_REGISTRYINDEX, s->sR );
	luaL_unref( L, LUA_REGISTRYINDEX, s->aR );
	luaL_unref( L, LUA_REGISTRYINDEX, s->lR );
	luaL_unref( L, LUA_REGISTRYINDEX, s->rR );

	printf("GC'ed HTTP Server...\n");

	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_srv_fm [] = {
	{ "__call",        lt_htp_srv__Call },
	{ NULL,            NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_htp_srv_cf [] = {
	{ "new",           lt_htp_srv_New },
	{ NULL,   NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_htp_srv_m [] = {
	{ "__len",         lt_htp_srv__len },
	{ "__gc",          lt_htp_srv__gc },
	{ "__tostring",    lt_htp_srv__tostring },
	{ "listen",        lt_htp_srv_listen },
	{ NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * \brief   pushes this library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_htp_srv( lua_State *L )
{
	// T.Http.Server instance metatable
	luaL_newmetatable( L, "T.Http.Server" );
	luaL_setfuncs( L, t_htp_srv_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Http.Server class
	luaL_newlib( L, t_htp_srv_cf );
	luaL_newlib( L, t_htp_srv_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

