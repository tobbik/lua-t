/**
 * \file        create network sockets
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


/** -------------------------------------------------------------------------
 * \brief   create a socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int c_new_socket(lua_State *luaVM)
{
	struct xt_hndl   __attribute__ ((unused)) *hndl;
	enum   xt_hndl_t                           type;

	type = (enum xt_hndl_t) luaL_checkoption (luaVM, 2, "TCP", xt_hndl_t_lst);
	hndl = create_ud_socket(luaVM, type);
	return 1 ;
}


/**--------------------------------------------------------------------------
 * \brief   create a socket and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_hndl*  pointer to the socket struct
 * --------------------------------------------------------------------------*/
struct xt_hndl *create_ud_socket(lua_State *luaVM, enum xt_hndl_t type)
{
	struct xt_hndl  *hndl;
	int              on=1;

	hndl = (struct xt_hndl*) lua_newuserdata(luaVM, sizeof(struct xt_hndl));
	if (UDP == type) {
		if ( (hndl->fd  =  socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ) {
			return( pusherror(luaVM, "ERROR opening UDP socket") );
		}
	}
	else if (TCP == type) {
		if ( (hndl->fd  =  socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
			return( pusherror(luaVM, "ERROR opening TCP socket") );
		}
		/* Enable address reuse */
		setsockopt( hndl->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	}

	luaL_getmetatable(luaVM, "L.net.Socket");
	lua_setmetatable(luaVM, -2);
	return hndl;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a struct xt_hndl
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_hndl*  pointer to the struct xt_hndl
 * --------------------------------------------------------------------------*/
struct xt_hndl *check_ud_socket (lua_State *luaVM, int pos) {
	void *ud = luaL_checkudata(luaVM, pos, "L.net.Socket");
	luaL_argcheck(luaVM, ud != NULL, pos, "`Socket` expected");
	return (struct xt_hndl *) ud;
}


/** -------------------------------------------------------------------------
 * \brief   create a UDP socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_create_udp_socket(lua_State *luaVM)
{
	struct xt_hndl  __attribute__ ((unused)) *hndl;

	hndl = create_ud_socket(luaVM, UDP);

	return 1 ;
}


/** -------------------------------------------------------------------------
 * \brief   create a TCP socket and return it.
 * \param   luaVM   The lua state.
 * \lreturn socket  socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_create_tcp_socket(lua_State *luaVM)
{
	struct xt_hndl  __attribute__ ((unused)) *hndl;

	hndl = create_ud_socket(luaVM, TCP);
	return 1 ;
}


/** -------------------------------------------------------------------------
 * \brief   listen on a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_listen_socket(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	int                 backlog;

	hndl = check_ud_socket (luaVM, 1);
	backlog = luaL_checkint(luaVM, 2);

	if( listen(hndl->fd , backlog ) == -1)
		return( pusherror(luaVM, "ERROR listen to socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   bind a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_bind_socket(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	struct sockaddr_in *ip;

	hndl = check_ud_socket (luaVM, 1);
	ip = check_ud_ipendpoint (luaVM, 2);

	if( bind(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   connect a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_connect_socket(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	struct sockaddr_in *ip;

	hndl = check_ud_socket (luaVM, 1);
	ip = check_ud_ipendpoint (luaVM, 2);

	if( connect(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR connecting socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   accept a (TCP) socket connection.
 * \param   luaVM   The lua state.
 * \lparam  socket  socket userdata.
 * \lreturn socket  socket userdata for new connection.
 * \lreturn ip      sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_accept_socket(lua_State *luaVM)
{
	struct xt_hndl  *lhndl;   // listening socket
	struct xt_hndl  *ahndl;   // accepted connection socket
	struct sockaddr_in *si_cli;
	socklen_t           their_addr_size = sizeof(struct sockaddr_in);

	lhndl = check_ud_socket (luaVM, 1);
	ahndl = create_ud_socket (luaVM, TCP);

	si_cli = create_ud_ipendpoint(luaVM);
	ahndl->fd = accept(lhndl->fd, (struct sockaddr *) &(*si_cli), &their_addr_size);

	return( 2 );
}


/** -------------------------------------------------------------------------
 * \brief   close a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_close_socket(lua_State *luaVM)
{
	struct xt_hndl  *hndl;

	hndl = check_ud_socket (luaVM, 1);

	if( close(hndl->fd)  == -1)
		return( pusherror(luaVM, "ERROR closing socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   send Datagram over a UDP socket to an IP endpoint.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_send_to(lua_State *luaVM)
{
	struct xt_hndl     *hndl;
	struct sockaddr_in *ip;
	int                 sent;
	const char         *msg;

	hndl = check_ud_socket (luaVM, 1);
	ip = check_ud_ipendpoint (luaVM, 2);
	if (lua_isstring(luaVM, 3))
		msg   = lua_tostring(luaVM, 3);
	else
		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes msg argument") );

	if ((sent = sendto(
	  hndl->fd,
	  msg, strlen(msg), 0,
	  (struct sockaddr *) &(*ip), sizeof(struct sockaddr))
	  ) == -1)
		return( pusherror(luaVM, "Failed to send UDP packet") );

	lua_pushinteger(luaVM, sent);
	return( 1 );
}


/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_recv_from(lua_State *luaVM)
{
	struct xt_hndl     *hndl;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[MAX_BUF_SIZE];

	unsigned int        slen=sizeof(si_cli);

	hndl = check_ud_socket (luaVM, 1);
	si_cli = create_ud_ipendpoint(luaVM);

	if ((rcvd = recvfrom(
	  hndl->fd,
	  buffer, sizeof(buffer)-1, 0,
	  (struct sockaddr *) &(*si_cli), &slen)
	  ) == -1)
		return( pusherror(luaVM, "Failed to recieve UDP packet") );

	// return buffer, length, IpEndpoint
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);
	lua_pushvalue(luaVM, -3);
	return 3;
}


/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket into a byteBuffer.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  byteBuffer The Buffer to recieve into.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
//static int recv_from_into_buffer(lua_State *luaVM)
//{
//	struct xt_hndl  *hndl;
//	int                 rcvd;
//	struct byteBuffer  *buffer;
//
//	struct sockaddr_in  si_cli;
//	unsigned int        slen=sizeof(si_cli);
//
//
//	if (lua_isuserdata(luaVM, 1))
//		hndl = (struct xt_hndl*) lua_touserdata(luaVM, 1);
//	else
//		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes socket argument") );
//
//	if ((rcvd = recvfrom(
//	  hndl->fd,
//	  buffer, sizeof(buffer)-1, 0,
//	  (struct sockaddr *) &si_cli, &slen)
//	  ) == -1)
//		return( pusherror(luaVM, "Failed to recieve UDP packet") );
//
//	// return buffer, length, ip, port
//	lua_pushlstring(luaVM, buffer, rcvd );
//	lua_pushinteger(luaVM, rcvd);
//	lua_pushstring(luaVM, inet_ntoa(si_cli.sin_addr) );
//	lua_pushinteger(luaVM, ntohs(si_cli.sin_port) );
//	return( 4 );
//
//}


/** -------------------------------------------------------------------------
 * \brief   send a message over a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_send_strm(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	size_t              sent;
	size_t              to_send;
	const char         *msg;
	size_t              into_msg=0;   // where in the message to start sending from

	hndl = check_ud_socket (luaVM, 1);
	if (lua_isstring(luaVM, 2))
		msg   = lua_tostring(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR send(socket,msg) takes msg argument") );
	if (lua_isnumber(luaVM, 3)) {
		into_msg = lua_tointeger(luaVM, 3);
	}
	msg    = msg + into_msg;
	to_send = strlen(msg) - into_msg;

	if ((sent = send(
	  hndl->fd,
	  msg, to_send, 0)
	  ) == -1)
		return( pusherror(luaVM, "Failed to send TCP message") );

	lua_pushinteger(luaVM, sent);
	return( 1 );
}


/** -------------------------------------------------------------------------
 * \brief   recieve some data from a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn string The recieved message.
 * \lreturn rcvd   number of bytes recieved.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow it to accept an existing LuaBuffer to ammend incoming packages
 *-------------------------------------------------------------------------*/
static int l_recv_strm(lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	int                 rcvd;
	char                buffer[MAX_BUF_SIZE];

	hndl = check_ud_socket (luaVM, 1);

	if ((rcvd = recv(
	  hndl->fd,
	  buffer, sizeof(buffer)-1, 0)
	  ) == -1)
		return( pusherror(luaVM, "Failed to recieve TCP packet") );

	// return buffer, length, ip, port
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);

	return( 2 );
}


/**--------------------------------------------------------------------------
 * \brief   prints out the socket.
 * \param   luaVM     The lua state.
 * \lparam  struct xt_hndl the socket userdata.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_socket_tostring (lua_State *luaVM) {
	struct xt_hndl *hndl = check_ud_socket(luaVM, 1);
	lua_pushfstring(luaVM, "Socket(%d): %p",
			hndl->fd,
			hndl
	);
	return 1;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_net_socket_fm [] = {
	{"__call",      c_new_socket},
	{NULL,   NULL}
};


/**
 * \brief      the net library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_net_socket_m [] =
{
	{"listen",    l_listen_socket},
	{"bind",      l_bind_socket},
	{"connect",   l_connect_socket},
	{"accept",    l_accept_socket},
	{"close",     l_close_socket},
	{"sendTo",    l_send_to},
	{"recvFrom",  l_recv_from},
	{"send",      l_send_strm},
	{"recv",      l_recv_strm},
	{NULL,        NULL}
};


/**
 * \brief      the net library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_net_socket_cf [] =
{
	{"createUdp", l_create_udp_socket},
	{"createTcp", l_create_tcp_socket},
	{NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * \brief   pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_net_socket (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable(luaVM, "L.net.Socket");   // stack: functions meta
	luaL_newlib(luaVM, l_net_socket_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_socket_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as net.Socket.localhost
	luaL_newlib(luaVM, l_net_socket_cf);
	//lua_createtable(luaVM, 0, 0);
	//lua_pushcfunction(luaVM, l_create_tcp_socket);
	//lua_setfield(luaVM, -2, "createTcp");
	//lua_pushcfunction(luaVM, l_create_udp_socket);
	//lua_setfield(luaVM, -2, "createUdp");
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib(luaVM, l_net_socket_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

