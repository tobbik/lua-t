/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim.c
 * \brief     OOP wrapper for time values(T.Time)
 *            This is a thin wrapper around struct timeval
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t_tim_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////


static int lt_tim_get( lua_State *L );  // forward declaration

/**--------------------------------------------------------------------------
 * Construct a timer and return it.
 * \param   L      The Lua state.
 * \lparam  CLASS  T.Time
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 *       OR
 * \lparam  ud     T.Time userdata instance to clone
 * \lreturn ud     T.Time userdata instance.
 * \return         int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__Call( lua_State *L )
{
	int  ms     = 0;

	lua_remove( L, 1 );
	if (t_tim_is( L, 1 ))
		lt_tim_get( L );         // pull the ms value of t_tim on the stack
	if (lua_isnumber( L, -1 ))
		ms = luaL_checkinteger( L, -1 );

	t_tim_create_ud( L, ms );

	return 1;
}


/**--------------------------------------------------------------------------
 * Create an timer userdata and push to LuaStack.
 * \param   L      Lua state.
 * \return  struct timeval* pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_create_ud( lua_State *L, long ms )
{
	struct timeval *tv;

	tv = (struct timeval *) lua_newuserdata( L, sizeof( struct timeval ) );
	if (ms > 0)
	{
		tv->tv_sec  = ms/1000;
		tv->tv_usec = (ms % 1000) * 1000;
	}
	else
		t_tim_now( tv, 0 );

	luaL_getmetatable( L, T_TIM_TYPE );
	lua_setmetatable( L, -2 );
	return tv;
}


/**--------------------------------------------------------------------------
 * Set the the value of timer.
 * \param   L        Lua state.
 * \lparam  timeval  The timeval userdata.
 * \lparam  int      Time to set in milliseconds (optional).  If not passed
 *                   creates T.Time with time for now since epoch.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_set( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	int             ms;

	if (lua_isnumber( L, 2 ))
	{
		ms = luaL_checkinteger( L, 2 );
		tv->tv_sec  = ms/1000;
		tv->tv_usec = (ms % 1000) * 1000;
	}
	else
	{
		t_tim_now( tv, 0 );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Get the value of the timer.
 * \param   L        Lua state.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_tim_get( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	lua_pushinteger( L, t_tim_getms( tv ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Reset the value of a timer to the difference between it's value and now.
 * It interprets the value of the time object as time passed since epoch and
 * rewrites it's value as difference between now and the previous value of the
 * object.  Useful to measure wall clock time in Lua.
 * \param   L        Lua state.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_since( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	t_tim_since( tv );
	return 0;
}


/**--------------------------------------------------------------------------
 * Reset the value of a timer to the difference between epoch and now.
 * \param   L      Lua state.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_now( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	t_tim_now( tv, 0 );
	return 0;
}


/**--------------------------------------------------------------------------
 * Get a string represents the timer.
 * \param   L         Lua state.
 * \lparam  ud        T.Time userdata instance.
 * \lreturn string    Formatted string T.Time{sec:microsecs}: mem_addr.
 * \return  int       # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__tostring( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	lua_pushfstring( L,
			T_TIM_TYPE"{%d:%d}: %p",
			//tv->tv_sec*1000 + tv->tv_usec/1000,
			tv->tv_sec, tv->tv_usec,
			tv
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * Compares two timer values.
 * \param   L         Lua state.
 * \lparam  ud        T.Time userdata instance.
 * \lparam  ud        T.Time userdata instance to compare to.
 * \lreturn boolean   true if equal otherwise false.
 * \return  int       # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__eq( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );

	lua_pushboolean( L, t_tim_cmp( tA, tB, == ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Adds two timer values.
 * \param   L         Lua state.
 * \lparam  ud        T.Time userdata instance.
 * \lparam  ud        T.Time userdata instance to add to.
 * \lreturn ud        T.Time userdata instance representing the sum.
 * \return  int       # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__add( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );
	struct timeval *tC = t_tim_create_ud( L, 0 );

	t_tim_add( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * Substract two time values.
 * \param   L         Lua state.
 * \lparam  ud        T.Time userdata instance.
 * \lparam  ud        T.Time userdata instance to substract from.
 * \lreturn ud        T.Time userdata instance representing the difference.
 * \return  int       # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__sub( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );
	struct timeval *tC = t_tim_create_ud( L, 0 );

	t_tim_sub( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * System call wrapper to sleep (Lua lacks that)
 *             Lua has no build in sleep method.
 * \param   L    Lua state.
 * \lparam  int  milliseconds to sleep
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_Sleep( lua_State *L )
{
#ifdef _WIN32
	fd_set dummy;
	int s;
#endif
	struct timeval *tv;
	long  sec, usec;
	if (lua_isnumber( L, -1 ))  t_tim_create_ud( L, luaL_checkinteger( L, -1 ) );
	tv  = t_tim_check_ud( L, -1, 1 );
	sec = tv->tv_sec; usec=tv->tv_usec;
#ifdef _WIN32
	s = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
	FD_ZERO( &dummy );
	FD_SET( s, &dummy );
	select( 0, 0, 0, &dummy, tv );
#else
	select( 0, 0, 0, 0, tv );
#endif
	tv->tv_sec=sec; tv->tv_usec=usec;
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tim_fm [] = {
	  { "__call"     , lt_tim__Call }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tim_cf [] = {
	  { "sleep"      , lt_tim_Sleep }     // can work on class or instance
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tim_m [] = {
	// metamethods
	  { "__tostring" , lt_tim__tostring }
	, { "__eq"       , lt_tim__eq }
	, { "__add"      , lt_tim__add }
	, { "__sub"      , lt_tim__sub }
	// instance methods
	, { "set"        , lt_tim_set }
	, { "get"        , lt_tim_get }
	, { "sleep"      , lt_tim_Sleep }
	, { "now"        , lt_tim_now }
	, { "since"      , lt_tim_since }
	, { NULL         , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Timer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      Lua state.
 * \lreturn table  The library.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_tim( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_TIM_TYPE );
	luaL_setfuncs( L, t_tim_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	luaL_newlib( L, t_tim_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_tim_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
