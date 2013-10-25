/*
 * \file    l_net_ip.c
 * \detail  OOP wrapper for IP/Port addresses. These is a thin wrapper around
 *          struct sockaddr
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
static int c_new_ipendpoint(lua_State *luaVM)
{
	struct sockaddr_in  *ip;
	int                  port;

	ip = create_ud_ipendpoint ( luaVM );

	memset( (char *) &(*ip), 0, sizeof(ip) );
	ip->sin_family = AF_INET;

	if (lua_isstring(luaVM, 2)) {
#ifdef _WIN32
		if ( InetPton (AF_INET, luaL_checkstring(luaVM, 2), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "InetPton() failed\n") );
#else
		if ( inet_pton(AF_INET, luaL_checkstring(luaVM, 2), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "inet_aton() failed\n") );
#endif
		if ( lua_isnumber(luaVM, 3) ) {
			port = luaL_checkint(luaVM, 3);
			luaL_argcheck(luaVM, 1 <= port && port <= 65536, 3,
								  "port number out of range");
			ip->sin_port   = htons(port);
		}
	}
	else if ( lua_isnumber(luaVM, 2) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
		port = luaL_checkint(luaVM, 2);
		luaL_argcheck(luaVM, 1 <= port && port <= 65536, 2,
		                 "port number out of range");
		ip->sin_port   = htons(port);
	}
	else if (lua_isnil(luaVM, 2) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   create an IP endpoint userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_in *create_ud_ipendpoint(lua_State *luaVM)
{
	struct sockaddr_in  *ip;

	ip = (struct sockaddr_in *) lua_newuserdata (luaVM, sizeof(struct sockaddr_in) );

	luaL_getmetatable(luaVM, "L.net.IpEndpoint");
	lua_setmetatable(luaVM, -2);
	return ip;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct sockaddr_in
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_in *check_ud_ipendpoint (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.net.IpEndpoint");
	luaL_argcheck(luaVM, ud != NULL, pos, "`IpEndpoint` expected");
	return (struct sockaddr_in *) ud;
}


/**--------------------------------------------------------------------------
 * \brief   set IP the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lparam  string   the xxx.xxx.xxx.xxx formatted IP address.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_set_ip (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);

	if (lua_isstring(luaVM, 2)) {
#ifdef _WIN32
		if ( InetPton (AF_INET, luaL_checkstring(luaVM, 2), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "InetPton() failed\n") );
#else
		if ( inet_pton(AF_INET, luaL_checkstring(luaVM, 2), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "inet_aton() failed\n") );
#endif
	}
	else if (lua_isnil(luaVM, 2) )
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   set port the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lparam  int      the port number.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_set_port (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);
	int port = luaL_checkint(luaVM, 2);

	luaL_argcheck(luaVM, 1 <= port && port <= 65536, 2,
	                 "port number out of range");

	ip->sin_port   = htons(port);
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   get IP the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lreturn int      sockkaddr port.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_get_ip (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);

	lua_pushstring(luaVM, inet_ntoa(ip->sin_addr) );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   get port the IP endpoint.
 * \param   luaVM    The lua state.
 * \lparam  sockaddr the sockaddr_in userdata.
 * \lreturn string   xxx.xxx.xxx.xxx formatted string representing sockkaddr IP.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_get_port (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);

	lua_pushinteger(luaVM, ntohs(ip->sin_port) );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the ip endpoint.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the sockaddr_in userdata.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_ipendpoint_tostring (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);
	lua_pushfstring(luaVM, "IpEndpoint{%s:%d}: %p",
			inet_ntoa(ip->sin_addr),
			ntohs(ip->sin_port),
			ip
	);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_net_ipendpoint_fm [] = {
	{"__call",      c_new_ipendpoint},
	{NULL,   NULL}
};


/**
 * \brief      the net IpEndpoint library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg l_net_ipendpoint_m [] = {
	{"setIp",       l_set_ip},
	{"getIp",       l_get_ip},
	{"setPort",     l_set_port},
	{"getPort",     l_get_port},
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
int luaopen_net_ipendpoint (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable(luaVM, "L.net.IpEndpoint");   // stack: functions meta
	luaL_newlib(luaVM, l_net_ipendpoint_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_ipendpoint_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as net.IpEndpoint.localhost
	lua_createtable(luaVM, 0, 0);
	lua_pushstring(luaVM, "127.0.0.1");
	lua_setfield(luaVM, -2, "localhost");
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib(luaVM, l_net_ipendpoint_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

