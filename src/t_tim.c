/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim.c
 * \brief     OOP wrapper for time values(T.Time)
 *            This is a thin wrapper around struct timeval
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>    // gettimeofday()
#endif

#include "t.h"
#include "t_tim.h"


/**--------------------------------------------------------------------------
 * Adds timeval tB to timeval tA and puts value into tX.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * \param  *tX struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_add( struct timeval *tA, struct timeval *tB, struct timeval *tX )
{
	struct timeval tC;

	tC.tv_sec    = tB->tv_sec  + tA->tv_sec ;  // add seconds
	tC.tv_usec   = tB->tv_usec + tA->tv_usec ; // add microseconds
	tC.tv_sec   += tC.tv_usec / 1000000 ;      // add microsecond overflow to seconds
	tC.tv_usec  %= 1000000 ;                   // subtract overflow from microseconds
	tX->tv_sec   = tC.tv_sec;
	tX->tv_usec  = tC.tv_usec;
}


/**--------------------------------------------------------------------------
 * Substract timeval tB from timeval tA and puts value into tX.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * \param  *tX struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_sub( struct timeval *tA, struct timeval *tB, struct timeval *tX )
{
	struct timeval tC;

	tC.tv_sec    = (tB->tv_usec > tA->tv_usec)
		? tA->tv_sec - tB->tv_sec - 1
		: tA->tv_sec - tB->tv_sec;
		;
	tC.tv_usec   = tA->tv_usec;
	tC.tv_usec   = (tB->tv_usec > tA->tv_usec)
		? 1000000 - (tB->tv_usec - tA->tv_usec)
		: tA->tv_usec - tB->tv_usec;
	tX->tv_sec   = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 0 : tC.tv_sec;
	tX->tv_usec  = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 1 : tC.tv_usec;
}


/**--------------------------------------------------------------------------
 * Sets tA to time different between tA and now
 * \param  *tv struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_since( struct timeval *tA )
{
	struct timeval tC;

	gettimeofday( &tC, 0 );
	t_tim_sub( &tC, tA, tA );
}


/**--------------------------------------------------------------------------
 * Gets milliseconds worth of tm
 * \param  *tv struct timeval pointer
 * \return timeval value in milliseconds
 * --------------------------------------------------------------------------*/
long
t_tim_getms( struct timeval *tA )
{
	return tA->tv_sec*1000 + tA->tv_usec/1000;
}


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////

/**--------------------------------------------------------------------------
 * Create an Timer and return it.
 * \param   L      The lua state.
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_New( lua_State *L )
{
	struct timeval  *tv;
	int              ms;

	tv = t_tim_create_ud( L );
	if (lua_isnumber( L, 1 )) {
		ms          = luaL_checkinteger( L, 1 );
		tv->tv_sec  = ms/1000;
		tv->tv_usec = (ms % 1000) * 1000;
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a timer and return it.
 * \param   L     The lua state.
 * \lparam  CLASS table Time
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_tim_New( L );
}


/**--------------------------------------------------------------------------
 * Create an timer userdata and push to LuaStack.
 * \param   L  The lua state.
 * \return  struct timeval*  pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_create_ud( lua_State *L )
{
	struct timeval *tv;

	tv = (struct timeval *) lua_newuserdata( L, sizeof( struct timeval ) );
	gettimeofday( tv, 0 );
	luaL_getmetatable( L, "T.Time" );
	lua_setmetatable( L, -2 );
	return tv;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct sockaddr_in
 * \param   L    The lua state.
 * \param   int  Position on the stack
 * \param   int  check(boolean): if true error out on fail
 * \return  struct timeval*  pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Time" );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`T.Time` expected" );
	return (NULL==ud) ? NULL : (struct timeval *) ud;
}


/**--------------------------------------------------------------------------
 * Set the the value of timer.
 * \param   L        The lua state.
 * \lparam  timeval  The timeval userdata.
 * \lparam  int      Time to set in milliseconds  (optional).
 * \return  int  # of values pushed onto the stack.
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
		gettimeofday( tv, 0 );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Get the value of the timer.
 * \param   L        The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lreturn string   xxx.xxx.xxx.xxx formatted string representing sockkaddr IP.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_get( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	lua_pushinteger( L, t_tim_getms( tv ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Get a string represents the timer.
 * \param   L         The lua state.
 * \lparam  userdata  T.Timer.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__tostring( lua_State *L )
{
	struct timeval *tv = t_tim_check_ud( L, 1, 1 );
	lua_pushfstring( L,
			"T.Time{%d:%d}: %p",
			//tv->tv_sec*1000 + tv->tv_usec/1000,
			tv->tv_sec, tv->tv_usec,
			tv
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * Compares two timer values.
 * \param   L         The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to compare to.
 * \lreturn boolean   true if equal otherwise false.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__eq( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );
	if (tA->tv_sec == tB->tv_sec && tA->tv_usec == tB->tv_usec)
		lua_pushboolean( L, 1 );
	else
		lua_pushboolean( L, 0 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Adds two timer values.
 * \param   L     The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to add to.
 * \lreturn timeval   the difference in time.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__add( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );
	struct timeval *tC = t_tim_create_ud( L );

	t_tim_add( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * Substract two time values.
 * \param   L     The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to add to.
 * \lreturn timeval   true if equal otherwise false.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim__sub( lua_State *L )
{
	struct timeval *tA = t_tim_check_ud( L, 1, 1 );
	struct timeval *tB = t_tim_check_ud( L, 2, 1 );
	struct timeval *tC = t_tim_create_ud( L );

	t_tim_sub( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * System call wrapper to sleep (Lua lacks that)
 *             Lua has no build in sleep method.
 * \param      The Lua state.
 * \lparam     int  milliseconds to sleep
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tim_sleep( lua_State *L )
{
#ifdef _WIN32
	fd_set dummy;
	int s;
#endif
	struct timeval *tv;
	long  sec, usec;
	if (lua_isnumber( L, -1 ))  lt_tim_New( L );
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
static const struct luaL_Reg t_tim_fm [] =
{
	{ "__call",    lt_tim__Call },
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tim_cf [] =
{
	{ "new",       lt_tim_New },
	{ "sleep",     lt_tim_sleep },     // can work on class or instance
	{ NULL,        NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tim_m [] =
{
	{ "set",       lt_tim_set },
	{ "get",       lt_tim_get },
	{ "sleep",     lt_tim_sleep },
	{ NULL,   NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Timer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_tim( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, "T.Time" );   // stack: functions meta
	luaL_newlib( L, t_tim_m );
	lua_setfield( L, -2, "__index" );
	lua_pushcfunction( L, lt_tim__tostring );
	lua_setfield( L, -2, "__tostring" );
	lua_pushcfunction( L, lt_tim__eq );
	lua_setfield( L, -2, "__eq" );
	lua_pushcfunction( L, lt_tim__add );
	lua_setfield( L, -2, "__add" );
	lua_pushcfunction( L, lt_tim__sub );
	lua_setfield( L, -2, "__sub" );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	luaL_newlib( L, t_tim_cf );
	luaL_newlib( L, t_tim_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

