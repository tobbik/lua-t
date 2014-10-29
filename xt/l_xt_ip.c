/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_ip.c
 * \brief     OOP wrapper for network addresses
 *            This is a thin wrapper around struct sockaddr_in
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
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

#include "xt.h"
#include "xt_sck.h"


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
	ip = create_ud_ipendpoint(luaVM);
	set_ipendpoint_values(luaVM, 2, ip);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   evaluate stack parameters to set endpoint criteria.
 * \param   luaVM The lua state.
 * \param   int   offset  on stack to start reading values
 * \param   struct sockaddr_in*  pointer to ip where values will be set
 * \lparam  port  the port for the socket.
 * \lparam  ip    the IP address for the socket.
 * \lreturn int error number .
 * TODO:  allow for empty endpoints if it makes sense
 * --------------------------------------------------------------------------*/
int set_ipendpoint_values(lua_State *luaVM, int pos, struct sockaddr_in *ip)
{
	int           port;
	const char   *ips;

	memset( (char *) &(*ip), 0, sizeof(ip) );
	ip->sin_family = AF_INET;

	if (lua_isstring(luaVM, pos+0)) {
		ips = luaL_checkstring(luaVM, pos+0);
#ifdef _WIN32
		if ( InetPton (AF_INET, ips, &(ip->sin_addr))==0)
			return ( xt_push_error(luaVM, "InetPton() of %s failed", ips ) );
#else
		if ( inet_pton(AF_INET, ips, &(ip->sin_addr))==0)
			return ( xt_push_error(luaVM, "inet_aton() of %s failed", ips ) );
#endif
		if ( lua_isnumber(luaVM, pos+1) ) {
			port = luaL_checkint(luaVM, pos+1);
			luaL_argcheck(luaVM, 1 <= port && port <= 65536, 3,
								  "port number out of range");
			ip->sin_port   = htons(port);
		}
	}
	else if ( lua_isnumber(luaVM, pos+0) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
		port = luaL_checkint(luaVM, pos+0);
		luaL_argcheck(luaVM, 1 <= port && port <= 65536, 2,
		                 "port number out of range");
		ip->sin_port   = htons(port);
	}
	else if (lua_isnil(luaVM, pos+0) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	return 0;
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

	luaL_getmetatable(luaVM, "L.IpEndpoint");
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
	void *ud = luaL_checkudata(luaVM, pos, "L.IpEndpoint");
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
	const char         *ips;  ///< IP String representation

	if (lua_isstring(luaVM, 2)) {
		ips = luaL_checkstring(luaVM, 2);
#ifdef _WIN32
		if ( InetPton (AF_INET, ips, &(ip->sin_addr))==0)
			return ( xt_push_error(luaVM, "InetPton() of %s failed", ips) );
#else
		if ( inet_pton(AF_INET, ips, &(ip->sin_addr))==0)
			return ( xt_push_error(luaVM, "inet_aton() of %s failed", ips) );
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
static int xt_ip___tostring (lua_State *luaVM) {
	struct sockaddr_in *ip = check_ud_ipendpoint(luaVM, 1);
	lua_pushfstring(luaVM, "IpEndpoint{%s:%d}: %p",
			inet_ntoa(ip->sin_addr),
			ntohs(ip->sin_port),
			ip
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   compares values of two IP Endpoints.
 * \param   luaVM     The lua state.
 * \lparam  sockaddr  the sockaddr_in userdata.
 * \lparam  sockaddr  the sockaddr_in userdata.
 * \lreturn boolean   if IP:Port and .
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_ip___eq (lua_State *luaVM) {
	struct sockaddr_in *ip    = check_ud_ipendpoint(luaVM, 1);
	struct sockaddr_in *ipCmp = check_ud_ipendpoint(luaVM, 2);

	if (ip->sin_family != ipCmp->sin_family) {
		lua_pushboolean(luaVM, 0);
		return 1;
	}

	if (ip->sin_family == AF_INET) {
		if (ip->sin_addr.s_addr != ipCmp->sin_addr.s_addr ||
		    ip->sin_port != ipCmp->sin_port)
		{
			lua_pushboolean(luaVM, 0);
			return 1;
		}
	} else {
		// TODO: Deal with IPv6 at some point!
			lua_pushboolean(luaVM, 0);
			return 1;
	}
	lua_pushboolean(luaVM, 1);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   converts an integer into an IP address xxx.xxx.xxx.xxx representation
 * \param   luaVM     The lua state.
 * \lparam  int       the IPv4 Address as 32 bit integer.
 * \lreturn string    IP address in xxx.xxx.xxx.xxx
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int xt_ip_int2ip (lua_State *luaVM)
{
	uint32_t  ip_int  = luaL_checkint (luaVM, 1);
	uint8_t  *ip      = (uint8_t *) &ip_int;

	lua_pushfstring (luaVM, "%d.%d.%d.%d",
		ip[3], ip[2], ip[1], ip[0]);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   converts an IPv4 Address into an unisgned integer
 * \param   luaVM     The lua state.
 * \lparam  string    the IPv4 Address as xxx.xxx.xxx.xxx string
 * \lreturn integer representing the IpAddress
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int xt_ip_ip2int (lua_State *luaVM)
{
	uint32_t       ip[4];
	const char   *ip_str  = luaL_checkstring (luaVM, 1);
	sscanf (ip_str, "%d.%d.%d.%d", &ip[3], &ip[2], &ip[1], &ip[0]);

	//printf ("strined IP[%d.%d.%d.%d]\n", ip[3], ip[2], ip[1], ip[0]);
	lua_pushinteger (luaVM, ip[0] | ip[1] << 8 | ip[2] << 16 | ip[3] << 24);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_ipendpoint_fm [] = {
	{"__call",      c_new_ipendpoint},
	{NULL,   NULL}
};


/**
 * \brief      the IpEndpoint library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg l_ipendpoint_m [] = {
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
int luaopen_ipendpoint (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable(luaVM, "L.IpEndpoint");   // stack: functions meta
	luaL_newlib(luaVM, l_ipendpoint_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, xt_ip___tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pushcfunction(luaVM, xt_ip___eq);
	lua_setfield(luaVM, -2, "__eq");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as IpEndpoint.localhost
	lua_createtable(luaVM, 0, 0);
	lua_pushstring(luaVM, "127.0.0.1");
	lua_setfield(luaVM, -2, "localhost");
	lua_pushcfunction (luaVM, xt_ip_int2ip);
	lua_setfield (luaVM, -2, "int2ip");
	lua_pushcfunction (luaVM, xt_ip_ip2int);
	lua_setfield (luaVM, -2, "ip2int");
	// set the methods as metatable
	luaL_newlib(luaVM, l_ipendpoint_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

