/*
 * \file    l_net_timer.c
 * \detail  OOP wrapper for Time values. This is a thin wrapper around
 *          struct timeval
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
#endif

#include "l_xt.h"
#include "l_xt_net.h"


/**--------------------------------------------------------------------------
 * create an IP endpoint and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn sockkaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  allow for empty endpoints if it makes sense
 * --------------------------------------------------------------------------*/
static int c_new_timer(lua_State *luaVM)
{
	struct timeval  *tv;
	int              ms = luaL_checkint(luaVM, 2);
	
	tv = create_ud_timer ( luaVM , ms );

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create an IP endpoint userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct timeval *create_ud_timer(lua_State *luaVM, int msec)
{
	struct timeval *tv;
	
	tv = (struct timeval *) lua_newuserdata (luaVM, sizeof(struct timeval) );
	tv->tv_sec  = msec/1000;
	tv->tv_usec = (msec % 1000) * 1000;

	luaL_getmetatable(luaVM, "L.net.Timer");
	lua_setmetatable(luaVM, -2);
	return tv;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct sockaddr_in
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct timeval *check_ud_timer (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.net.Timer");
	luaL_argcheck(luaVM, ud != NULL, pos, "`Timer` expected");
	return (struct timeval *) ud;
}


/**--------------------------------------------------------------------------
 * \brief   set port the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lparam  int      the port number.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_set_time (lua_State *luaVM) {
	struct timeval *tv = check_ud_timer(luaVM, 1);
	int             ms = luaL_checkint(luaVM, 2);

	tv->tv_sec  = ms/1000;
	tv->tv_usec = (ms % 1000) * 1000;

	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   get port the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lreturn string   xxx.xxx.xxx.xxx formatted string representing sockkaddr IP.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_get_time (lua_State *luaVM) {
	struct timeval *tv = check_ud_timer(luaVM, 1);

	lua_pushinteger(luaVM, tv->tv_sec*1000 );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the ip endpoint.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the sockaddr_in userdata.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_timer_tostring (lua_State *luaVM) {
	struct timeval *tv = check_ud_timer(luaVM, 1);
	lua_pushfstring(luaVM, "Timer{%d}: %p",
			tv->tv_sec*1000,
			tv
	);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_net_timer_fm [] = {
	{"__call",      c_new_timer},
	{NULL,   NULL}
};


/**
 * \brief      the net IpEndpoint library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg l_net_timer_m [] = {
	{"setTime",     l_set_time},
	{"getTime",     l_get_time},
	{NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the IpEndpoint library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_net_timer (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable(luaVM, "L.net.Timer");   // stack: functions meta
	luaL_newlib(luaVM, l_net_timer_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_timer_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as net.IpEndpoint.localhost
	lua_createtable(luaVM, 0, 0);
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib(luaVM, l_net_timer_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

