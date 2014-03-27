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
#include "l_xt_hndl.h"
#include "xt_buf.h"         // the ability to send and recv buffers


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
			return xt_push_error(luaVM, "ERROR opening UDP socket") ;
		}
	}
	else if (TCP == type) {
		if ( (hndl->fd  =  socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
			return xt_push_error(luaVM, "ERROR opening TCP socket") ;
		}
		/* Enable address reuse */
		setsockopt( hndl->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	}

	luaL_getmetatable(luaVM, "L.Socket");
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
	void *ud = luaL_checkudata(luaVM, pos, "L.Socket");
	luaL_argcheck(luaVM, ud != NULL, pos, "`Socket` expected");
	return (struct xt_hndl *) ud;
}


/** -------------------------------------------------------------------------
 * \brief   create a UDP socket and return it.
 * \param   luaVM  The lua state.
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
		return xt_push_error(luaVM, "ERROR listen to socket");

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
	enum   xt_hndl_t    type;

	if ( lua_isuserdata(luaVM, 1) ) { // handle sock:bind()
		hndl = check_ud_socket (luaVM, 1);
	}
	else {                            // handle xt.Socket.bind()
		type = (enum xt_hndl_t) luaL_checkoption (luaVM, 1, "TCP", xt_hndl_t_lst);
		hndl = create_ud_socket(luaVM, type);
	}
	if ( lua_isuserdata(luaVM, 2) ) {
		// it's assumed that IP/port et cetera are assigned
		ip   = check_ud_ipendpoint (luaVM, 2);
		lua_pushvalue(luaVM, 1);
	}
	else {
		ip = create_ud_ipendpoint(luaVM);
		set_ipendpoint_values(luaVM, 2, ip);
	}

	if( bind(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return xt_push_error(luaVM, "ERROR binding socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port));

	return( 2 );  // socket, ip
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
	struct xt_hndl     *hndl;
	struct sockaddr_in *ip;
	enum   xt_hndl_t    type;

	if ( lua_isuserdata(luaVM, 1) ) { // handle sock:connect()
		hndl = check_ud_socket (luaVM, 1);
	}
	else {                            // handle xt.Socket.connect()
		type = (enum xt_hndl_t) luaL_checkoption (luaVM, 1, "TCP", xt_hndl_t_lst);
		hndl = create_ud_socket(luaVM, type);
	}
	if ( lua_isuserdata(luaVM, 2) ) {
		// it's assumed that IP/port et cetera are assigned
		ip   = check_ud_ipendpoint (luaVM, 2);
		lua_pushvalue(luaVM, 1);
	}
	else {
		ip = create_ud_ipendpoint(luaVM);
		set_ipendpoint_values(luaVM, 2, ip);
	}

	if( connect(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return xt_push_error(luaVM, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port));

	return( 2 ); //socket,ip
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
		return( xt_push_error(luaVM, "ERROR closing socket") );

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
	struct xt_buf      *buf;
	int                 sent;
	int                 len;
	const char         *msg;

	hndl = check_ud_socket (luaVM, 1);
	ip   = check_ud_ipendpoint (luaVM, 2);
	if (lua_isstring(luaVM, 3)) {
		msg   = lua_tostring(luaVM, 3);
		len   = strlen(msg);
	}
	else if (lua_isuserdata(luaVM, 3)) {
		buf  = xt_buf_check_ud (luaVM, 3);
		msg  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return( xt_push_error(luaVM, "ERROR sendTo(socket,ip,msg) takes msg argument") );

	if ((sent = sendto(
	  hndl->fd,
	  msg, len, 0,
	  (struct sockaddr *) &(*ip), sizeof(struct sockaddr))
	  ) == -1)
		return xt_push_error(luaVM, "Failed to send UDP packet to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port));

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
	struct xt_buf      *buf;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[MAX_BUF_SIZE];
	char                *rcv = &(buffer[0]);
	int                 len = sizeof(buffer)-1;

	unsigned int        slen=sizeof(si_cli);

	hndl = check_ud_socket (luaVM, 1);
	if (lua_isuserdata(luaVM, 2)) {
		buf  = xt_buf_check_ud (luaVM, 2);
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	si_cli = create_ud_ipendpoint(luaVM);

	if ((rcvd = recvfrom(
	  hndl->fd,
	  rcv, len, 0,
	  (struct sockaddr *) &(*si_cli), &slen)
	  ) == -1)
		return( xt_push_error(luaVM, "Failed to recieve UDP packet") );

	// return buffer, length, IpEndpoint
	lua_pushlstring(luaVM, rcv, rcvd );
	lua_pushinteger(luaVM, rcvd);
	lua_pushvalue(luaVM, -3);
	return 3;
}


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
	size_t           sent;         // How much did get sent out
	size_t           to_send;      // How much should get send out maximally
	const char      *msg;
	size_t           into_msg=0;   // where in the message to start sending from

	hndl = check_ud_socket (luaVM, 1);
	if (lua_isstring(luaVM, 2))
		msg   = lua_tolstring (luaVM, 2, &to_send);
	else
		return( xt_push_error(luaVM, "ERROR send(socket,msg) takes msg argument") );
	if (lua_isnumber(luaVM, 3)) {
		into_msg = lua_tointeger(luaVM, 3);
	}
	msg      = msg + into_msg;
	to_send -= into_msg;

	if ((sent = send(
	  hndl->fd,
	  msg, to_send, 0)
	  ) == -1)
		return( xt_push_error(luaVM, "Failed to send TCP message") );

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
 * TODO:  If no Buffer is passed for receiving, use a LuaL_Buffer instead
 *-------------------------------------------------------------------------*/
static int l_recv_strm (lua_State *luaVM)
{
	struct xt_hndl  *hndl;
	struct xt_buf   *buf;
	int              rcvd;
	char             buffer[MAX_BUF_SIZE];
	char            *rcv = &(buffer[0]);
	int              len = sizeof (buffer)-1;

	hndl = check_ud_socket (luaVM, 1);
	if (lua_isuserdata (luaVM, 2)) {
		buf  = xt_buf_check_ud (luaVM, 2);
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}

	if ((rcvd = recv(
	  hndl->fd,
	  rcv, len, 0)
	  ) == -1)
		return xt_push_error(luaVM, "Failed to recieve TCP packet");

	// return buffer, length, ip, port
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);

	return( 2 );
}


/** -------------------------------------------------------------------------
 * \brief   recieve IpEndpoint from a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  IpEndpoint userdata.
 * \lreturn IpEndpoint userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow it to accept an existing LuaBuffer to ammend incoming packages
 *-------------------------------------------------------------------------*/
static int xt_socket_getsockname (lua_State *luaVM)
{
	struct xt_hndl      *hndl;
	struct sockaddr_in  *ip;
	socklen_t            ip_len = sizeof(struct sockaddr_in);

	hndl = check_ud_socket(luaVM, 1);

	if (lua_isuserdata(luaVM, 2))  ip = check_ud_ipendpoint(luaVM, 2);
	else                           ip = create_ud_ipendpoint(luaVM);

	getsockname(hndl->fd, (struct sockaddr*) &(*ip), &ip_len);
	return 1;
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

/** -------------------------------------------------------------------------
 * \brief   return the FD int representation of the socket
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int xt_socket_getfdid (lua_State *luaVM)
{
	struct xt_hndl      *hndl;

	hndl = check_ud_socket (luaVM, 1);
	lua_pushinteger (luaVM, hndl->fd);
	return 1;
}



/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg l_socket_fm [] = {
	{"__call",      c_new_socket},
	{NULL,   NULL}
};


/**
 * \brief      the socketlibrary definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_socket_m [] =
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
	{"getId",     xt_socket_getfdid},
	{"getIp",     xt_socket_getsockname},
	{NULL,        NULL}
};


/**
 * \brief      the socket library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_socket_cf [] =
{
	{"createUdp", l_create_udp_socket},
	{"createTcp", l_create_tcp_socket},
	{"bind",      l_bind_socket},
	{"connect",   l_connect_socket},
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
int luaopen_socket (lua_State *luaVM) {
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable(luaVM, "L.Socket");   // stack: functions meta
	luaL_newlib(luaVM, l_socket_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, l_socket_tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib(luaVM, l_socket_cf);
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib(luaVM, l_socket_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}

