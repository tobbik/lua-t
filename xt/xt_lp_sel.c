// Select specific implementation of Loop features
//
//
//
#include "l_xt.h"
#include "xt_lp.h"
#include "xt_time.h"



/**--------------------------------------------------------------------------
 * construct a xt.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table xt.Loop.
 * \lreturn struct xt_lp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lxt_lp_New( luaVM );
}


/**--------------------------------------------------------------------------
 * create a new xt.Loop and return it.
 * \param   luaVM  The lua state.
 * \lreturn struct xt_lp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
int lxt_lp_New( lua_State *luaVM )
{
	struct xt_lp    *lp;
	lp = xt_lp_create_ud( luaVM );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new xt_lp userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_lp * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct xt_lp *xt_lp_create_ud( lua_State *luaVM )
{
	struct xt_lp    *lp;

	lp = (struct xt_lp *) lua_newuserdata( luaVM, sizeof( struct xt_lp ) );
	lp->mx_sz = 1024;
	lp->mxfd  = -1;
	FD_ZERO( &lp->rfds );
	FD_ZERO( &lp->wfds );
	luaL_getmetatable( luaVM, "xt.Loop" );
	lua_setmetatable( luaVM, -2 );
	return lp;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct xt_lp
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_lp*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct xt_lp *xt_lp_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Loop" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Loop` expected" );
	return (struct xt_lp *) ud;
}



/**--------------------------------------------------------------------------
 * Add an event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.
 * \lparam  userdata timeval or xt_hndl.
 * \lparam  function to be executed when event handler fires.
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_lp_addhandle( lua_State *luaVM )
{
	luaL_Stream    *lS;
	struct xt_sck  *sc;
	struct xt_lp   *lp = xt_lp_check_ud( luaVM, 1);


	lS = (luaL_Stream *) luaL_testudata( luaVM, 2, LUA_FILEHANDLE );
// if (NULL != lS)
// {



	sc = (struct xt_sck *) luaL_testudata( luaVM, 2, "xt.Socket" );
	// if (NULL != sc)
	return 0;
}


/**--------------------------------------------------------------------------
 * Add an event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.
 * \lparam  userdata timeval or xt_hndl.
 * \lparam  function to be executed when event handler fires.
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_lp_addtimer( lua_State *luaVM )
{
	
	struct xt_lp   *lp = xt_lp_check_ud( luaVM, 1 );
	struct timeval *tv = xt_time_check_ud( luaVM, 2 );
	int    repeat      = lua_toboolean( luaVM, 3 );


	return 0;
}


/**--------------------------------------------------------------------------
 * Prints out the Loop.
 * \param   luaVM     The lua state.
 * \lparam  userdata  xt.Loop userdata
 * \lreturn string    formatted string representing xt.Loop.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_lp__tostring( lua_State *luaVM )
{
	struct xt_lp *lp = xt_lp_check_ud( luaVM, 1 );
	lua_pushfstring( luaVM, "xt.Loop(select){%d}: %p", lp->mxfd, lp );
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_lp_fm [] =
{
	{"__call",    lxt_lp__Call},
	{NULL,   NULL}
};

/**
 * \brief      the Time library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_lp_cf [] =
{
	{"new",       lxt_lp_New},
	{NULL,        NULL}
};


/**
 * \brief      the Timer library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_lp_m [] =
{
	{"addTimer",       lxt_lp_addtimer},
	{"addHandle",      lxt_lp_addhandle},
	{NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the Loop library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_xt_lp( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "xt.Loop" );   // stack: functions meta
	luaL_newlib( luaVM, xt_lp_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_lp__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	luaL_newlib( luaVM, xt_lp_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( luaVM, xt_lp_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}
