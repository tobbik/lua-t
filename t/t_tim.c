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
#include <WinSock2.h>
#include <winsock.h>
#include <time.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>    // gettimeofday()
#endif

#include "t.h"
#include "t_tim.h"


/**--------------------------------------------------------------------------
 * \brief  adds timeval B to timeval A.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * --------------------------------------------------------------------------*/
void t_tim_add (struct timeval *tA, struct timeval *tB, struct timeval *tX)
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
 * \brief  substract timeval B from timeval A.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * --------------------------------------------------------------------------*/
void t_tim_sub (struct timeval *tA, struct timeval *tB, struct timeval *tX)
{
	struct timeval tC;

	tC.tv_sec = (tB->tv_usec > tA->tv_usec)
		? tA->tv_sec - tB->tv_sec - 1
		: tA->tv_sec - tB->tv_sec;
		;
	tC.tv_usec   = tA->tv_usec;
	tC.tv_usec = (tB->tv_usec > tA->tv_usec)
		? 1000000 - (tB->tv_usec - tA->tv_usec)
		: tA->tv_usec - tB->tv_usec;
	tX->tv_sec   = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 0 : tC.tv_sec;
	tX->tv_usec  = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 1 : tC.tv_usec;
}


/**--------------------------------------------------------------------------
 * \brief  sets tm to time different between tm and now
 * \param  *tv struct timeval pointer
 * --------------------------------------------------------------------------*/
void t_tim_since (struct timeval *tA)
{
	struct timeval tC;

	gettimeofday( &tC, 0 );
	t_tim_sub( &tC, tA, tA );
}


/**--------------------------------------------------------------------------
 * \brief  gets milliseconds worth of tm
 * \param  *tv struct timeval pointer
 * \return timeval value in milliseconds
 * --------------------------------------------------------------------------*/
long t_tim_getms (struct timeval *tA)
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
 * create an Timer and return it.
 * \param   luaVM  The lua state.
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim_New( lua_State *luaVM )
{
	struct timeval  *tv;
	int              ms;

	tv = t_tim_create_ud (luaVM);
	if (lua_isnumber(luaVM, 1)) {
		ms          = luaL_checkinteger(luaVM, 1);
		tv->tv_sec  = ms/1000;
		tv->tv_usec = (ms % 1000) * 1000;
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * construct a timer an Timer and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table Time
 * \lparam  int    time spam in milliseconds, if omitted time since epoch
 * \lreturn struct timeval userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_tim_New( luaVM );
}


/**--------------------------------------------------------------------------
 * \brief   create an timer userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct timeval*  pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_create_ud( lua_State *luaVM )
{
	struct timeval *tv;

	tv = (struct timeval *) lua_newuserdata( luaVM, sizeof( struct timeval ) );
	gettimeofday( tv, 0 );
	luaL_getmetatable( luaVM, "T.Time" );
	lua_setmetatable( luaVM, -2 );
	return tv;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct sockaddr_in
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \param   int      check(boolean): if true error out on fail
 * \return  struct timeval*  pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Time" );
	luaL_argcheck( luaVM, (ud != NULL  || !check), pos, "`T.Time` expected" );
	return (NULL==ud) ? NULL : (struct timeval *) ud;
}


/**--------------------------------------------------------------------------
 * \brief   set the the value of timer.
 * \param   luaVM    The lua state.
 * \lparam  timeval  The timeval userdata.
 * \lparam  int      time to set in milliseconds  (optional).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim_set( lua_State *luaVM )
{
	struct timeval *tv = t_tim_check_ud( luaVM, 1, 1 );
	int             ms;

	if (lua_isnumber( luaVM, 2 ))
	{
		ms = luaL_checkinteger( luaVM, 2 );
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
 * \brief   get port the timer.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lreturn string   xxx.xxx.xxx.xxx formatted string representing sockkaddr IP.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim_get( lua_State *luaVM )
{
	struct timeval *tv = t_tim_check_ud( luaVM, 1, 1 );
	lua_pushinteger( luaVM, t_tim_getms( tv ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the timer.
 * \param   luaVM     The lua state.
 * \lparam  userdata  T.Timer.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim__tostring( lua_State *luaVM )
{
	struct timeval *tv = t_tim_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM,
			"T.Time{%d:%d}: %p",
			//tv->tv_sec*1000 + tv->tv_usec/1000,
			tv->tv_sec, tv->tv_usec,
			tv
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   compares two time values.
 * \param   luaVM     The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to compare to.
 * \lreturn boolean   true if equal otherwise false.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim__eq( lua_State *luaVM )
{
	struct timeval *tA = t_tim_check_ud( luaVM, 1, 1 );
	struct timeval *tB = t_tim_check_ud( luaVM, 2, 1 );
	if (tA->tv_sec == tB->tv_sec && tA->tv_usec == tB->tv_usec)
		lua_pushboolean( luaVM, 1 );
	else
		lua_pushboolean( luaVM, 0 );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   adds two time values.
 * \param   luaVM     The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to add to.
 * \lreturn timeval   the difference in time.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim__add( lua_State *luaVM )
{
	struct timeval *tA = t_tim_check_ud( luaVM, 1, 1 );
	struct timeval *tB = t_tim_check_ud( luaVM, 2, 1 );
	struct timeval *tC = t_tim_create_ud( luaVM );

	t_tim_add( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   substract two time values.
 * \param   luaVM     The lua state.
 * \lparam  timeval   the timval userdata.
 * \lparam  timeval   timval userdata to add to.
 * \lreturn timeval   true if equal otherwise false.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_tim__sub( lua_State *luaVM )
{
	struct timeval *tA = t_tim_check_ud( luaVM, 1, 1 );
	struct timeval *tB = t_tim_check_ud( luaVM, 2, 1 );
	struct timeval *tC = t_tim_create_ud( luaVM );

	t_tim_sub( tA, tB, tC );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief      a system call to sleep (Lua lacks that)
 *             Lua has no build in sleep method.
 * \param      The Lua state.
 * \lparam     int  milliseconds to sleep
 * \return     0 return values
 * --------------------------------------------------------------------------*/
static int
lt_tim_sleep( lua_State *luaVM )
{
#ifdef _WIN32
	fd_set dummy;
	int s;
#endif
	struct timeval *tv;
	long  sec, usec;
	if (lua_isnumber( luaVM, -1 ))  lt_tim_New( luaVM );
	tv  = t_tim_check_ud( luaVM, -1, 1 );
	sec = tv->tv_sec; usec=tv->tv_usec;
#ifdef _WIN32
	s = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
	FD_ZERO( &dummy );
	FD_SET( s, &dummy );
	select(0, 0, 0, &dummy, tv);
#else
	select(0, 0, 0, 0, tv);
#endif
	tv->tv_sec=sec; tv->tv_usec=usec;
	return 0;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg t_tim_fm [] =
{
	{"__call",    lt_tim__Call},
	{NULL,   NULL}
};

/**
 * \brief      the Time library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_tim_cf [] =
{
	{"new",       lt_tim_New},
	{"sleep",     lt_tim_sleep},     // method can work on class aor instance
	{NULL,        NULL}
};


/**
 * \brief      the Timer library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg t_tim_m [] =
{
	{"set",       lt_tim_set},
	{"get",       lt_tim_get},
	{"sleep",     lt_tim_sleep},
	{NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the Timer library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_tim( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "T.Time" );   // stack: functions meta
	luaL_newlib( luaVM, t_tim_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_tim__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_tim__eq );
	lua_setfield( luaVM, -2, "__eq" );
	lua_pushcfunction( luaVM, lt_tim__add );
	lua_setfield( luaVM, -2, "__add" );
	lua_pushcfunction( luaVM, lt_tim__sub );
	lua_setfield( luaVM, -2, "__sub" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Timer.localhost
	luaL_newlib( luaVM, t_tim_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( luaVM, t_tim_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

