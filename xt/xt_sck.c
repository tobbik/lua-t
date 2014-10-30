/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_sck.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP, read write connect bind etc
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
#include "xt_buf.h"         // the ability to send and recv buffers



/**--------------------------------------------------------------------------
 * construct a Socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table Socket
 * \lparam  string    type "TCP", "UDP", ...
 * \lreturn struct xt_sck userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_sck__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lxt_sck_New( luaVM );
}


/** -------------------------------------------------------------------------
 * \brief   create a socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int lxt_sck_New( lua_State *luaVM )
{
	struct xt_sck   __attribute__ ((unused)) *sck;

	sck = xt_sck_create_ud( luaVM,
			(enum xt_sck_t) luaL_checkoption (luaVM, 1, "TCP", xt_sck_t_lst) );
	return 1 ;
}


/**--------------------------------------------------------------------------
 * \brief   create a socket and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_sck*  pointer to the socket struct
 * --------------------------------------------------------------------------*/
struct xt_sck *xt_sck_create_ud( lua_State *luaVM, enum xt_sck_t type )
{
	int             on = 1;
	struct xt_sck  *sck = (struct xt_sck*) lua_newuserdata( luaVM, sizeof( struct xt_sck ) );

	switch (type)
	{
		case XT_SCK_UDP:
			if ( (sck->fd  =  socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ) {
				return NULL;
			}
			break;

		case XT_SCK_TCP:
			if ( (sck->fd  =  socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
				return NULL;
			}
			// Enable address reuse
			setsockopt( sck->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
			break;
		default:
			return NULL;
	}

	sck->t = type;

	luaL_getmetatable( luaVM, "xt.Socket" );
	lua_setmetatable( luaVM, -2 );
	return sck;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct xt_sck.
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \return  struct xt_sck*  pointer to the struct xt_sck.
 * --------------------------------------------------------------------------*/
struct xt_sck *xt_sck_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Socket" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Socket` expected" );
	return (struct xt_sck *) ud;
}


/** -------------------------------------------------------------------------
 * Listen on a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_listen( lua_State *luaVM )
{
	struct xt_sck  *sck;
	int             backlog = luaL_checkint(luaVM, 2);

	sck = xt_sck_check_ud (luaVM, 1);
	luaL_argcheck( luaVM, (XT_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if( listen(sck->fd , backlog ) == -1)
		return xt_push_error(luaVM, "ERROR listen to socket");

	return( 0 );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_bind( lua_State *luaVM )
{
	struct xt_sck      *sck;
	struct sockaddr_in *ip;
	enum   xt_sck_t     type;

	if (lua_isuserdata( luaVM, 1 ))
	{ // handle sock:bind()
		sck = xt_sck_check_ud( luaVM, 1 );
	}
	else
	{                            // handle xt.Socket.bind()
		type = (enum xt_sck_t) luaL_checkoption( luaVM, 1, "TCP", xt_sck_t_lst );
		sck = xt_sck_create_ud( luaVM, type );
	}

	if ( lua_isuserdata(luaVM, 2) )
	{
		// it's assumed that IP/port et cetera are assigned
		ip   = check_ud_ipendpoint (luaVM, 2);
		lua_pushvalue(luaVM, 1);
	}
	else {
		ip = create_ud_ipendpoint( luaVM );
		set_ipendpoint_values( luaVM, 2, ip );
	}

	if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return xt_push_error( luaVM, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	return 2;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_connect( lua_State *luaVM )
{
	struct xt_sck      *sck;
	struct sockaddr_in *ip;
	enum   xt_sck_t     type;

	if (lua_isuserdata( luaVM, 1 ))
	{ // handle sock:connect()
		sck = xt_sck_check_ud( luaVM, 1 );
	}
	else
	{                            // handle xt.Socket.connect()
		type = (enum xt_sck_t) luaL_checkoption( luaVM, 1, "TCP", xt_sck_t_lst );
		sck = xt_sck_create_ud( luaVM, type);
	}
	if (lua_isuserdata( luaVM, 2 ))
	{
		// it's assumed that IP/port et cetera are assigned
		ip = check_ud_ipendpoint( luaVM, 2 );
		lua_pushvalue( luaVM, 1 );
	}
	else
	{
		ip = create_ud_ipendpoint( luaVM );
		set_ipendpoint_values( luaVM, 2, ip );
	}

	if (connect( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return xt_push_error( luaVM, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return 2; //socket,ip
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   luaVM   The lua state.
 * \lparam  socket  socket userdata.
 * \lreturn socket  socket userdata for new connection.
 * \lreturn ip      sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_accept( lua_State *luaVM )
{
	struct xt_sck  *lsck;   // listening socket
	struct xt_sck  *asck;   // accepted connection socket
	struct sockaddr_in *si_cli;
	socklen_t           their_addr_size = sizeof( struct sockaddr_in );

	lsck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_TCP == lsck->t), 1, "Must be an TCP socket" );
	asck = xt_sck_create_ud( luaVM, XT_SCK_TCP );

	si_cli = create_ud_ipendpoint( luaVM );
	asck->fd = accept( lsck->fd, (struct sockaddr *) &(*si_cli), &their_addr_size );

	return 2;
}


/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_close( lua_State *luaVM )
{
	struct xt_sck  *sck;

	sck = xt_sck_check_ud (luaVM, 1);

	if( close(sck->fd)  == -1)
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
static int lxt_sck_send_to(lua_State *luaVM)
{
	struct xt_sck     *sck;
	struct sockaddr_in *ip;
	struct xt_buf      *buf;
	int                 sent;
	int                 len;
	const char         *msg;

	sck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	ip   = check_ud_ipendpoint( luaVM, 2 );
	if (lua_isstring( luaVM, 3 ))
	{
		msg   = lua_tostring( luaVM, 3 );
		len   = strlen( msg );
	}
	else if (lua_isuserdata( luaVM, 3 ))
	{
		buf  = xt_buf_check_ud( luaVM, 3 );
		msg  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return xt_push_error( luaVM, "ERROR sendTo(socket,ip,msg) takes msg argument" );

	if ((sent = sendto(
	  sck->fd,
	  msg, len, 0,
	  (struct sockaddr *) &(*ip), sizeof( struct sockaddr ))
	  ) == -1)
		return xt_push_error( luaVM, "Failed to send UDP packet to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	lua_pushinteger( luaVM, sent );
	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve Datagram from a UDP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_recv_from(lua_State *luaVM)
{
	struct xt_sck      *sck;
	struct xt_buf      *buf;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[ MAX_BUF_SIZE ];
	char               *rcv = &(buffer[ 0 ]);
	int                 len = sizeof( buffer )-1;

	unsigned int        slen = sizeof( si_cli );

	sck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	if (lua_isuserdata( luaVM, 2 )) {
		buf  = xt_buf_check_ud ( luaVM, 2 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	si_cli = create_ud_ipendpoint( luaVM );

	if ((rcvd = recvfrom(
	  sck->fd,
	  rcv, len, 0,
	  (struct sockaddr *) &(*si_cli), &slen )
	  ) == -1)
		return( xt_push_error( luaVM, "Failed to recieve UDP packet") );

	// return buffer, length, IpEndpoint
	lua_pushlstring( luaVM, rcv, rcvd );
	lua_pushinteger( luaVM, rcvd );
	lua_pushvalue( luaVM, -3 );
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
static int lxt_sck_send_strm( lua_State *luaVM )
{
	struct xt_sck  *sck;
	ssize_t         sent;         // How much did get sent out
	size_t          to_send;      // How much should get send out maximally
	const char     *msg;
	size_t          into_msg=0;   // where in the message to start sending from

	sck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isstring( luaVM, 2 ))
		msg   = lua_tolstring( luaVM, 2, &to_send );
	else
		return xt_push_error( luaVM, "ERROR send(socket,msg) takes msg argument" ) ;
	if (lua_isnumber( luaVM, 3 )) {
		into_msg = lua_tointeger( luaVM, 3 );
	}
	msg      = msg + into_msg;
	to_send -= into_msg;

	if ((sent = send(
	  sck->fd,
	  msg, to_send, 0)
	  ) == -1)
		return xt_push_error( luaVM, "Failed to send TCP message" ) ;

	lua_pushinteger( luaVM, sent );
	return 1;
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
static int lxt_sck_recv_strm( lua_State *luaVM )
{
	struct xt_sck  *sck;
	struct xt_buf  *buf;
	int             rcvd;
	char            buffer[MAX_BUF_SIZE];
	char           *rcv = &(buffer[0]);
	int             len = sizeof (buffer)-1;

	sck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isuserdata( luaVM, 2 )) {
		buf  = xt_buf_check_ud( luaVM, 2 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}

	if ((rcvd = recv(
	  sck->fd,
	  rcv, len, 0)
	  ) == -1)
		return xt_push_error( luaVM, "Failed to recieve TCP packet" );

	// return buffer, length, ip, port
	lua_pushlstring( luaVM, buffer, rcvd );
	lua_pushinteger( luaVM, rcvd );

	return 2;
}


/** -------------------------------------------------------------------------
 * Recieve IpEndpoint from a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  IpEndpoint userdata.
 * \lreturn IpEndpoint userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow it to accept an existing LuaBuffer to ammend incoming packages
 *-------------------------------------------------------------------------*/
static int lxt_sck_getsockname( lua_State *luaVM )
{
	struct xt_sck      *sck;
	struct sockaddr_in *ip;
	socklen_t           ip_len = sizeof( struct sockaddr_in );

	sck = xt_sck_check_ud( luaVM, 1 );
	luaL_argcheck( luaVM, (XT_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if (lua_isuserdata( luaVM, 2 ))  ip = check_ud_ipendpoint( luaVM, 2 );
	else                             ip = create_ud_ipendpoint( luaVM );

	getsockname( sck->fd, (struct sockaddr*) &(*ip), &ip_len );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the socket.
 * \param   luaVM     The lua state.
 * \lparam  struct xt_sck the socket userdata.
 * \lreturn string    formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_sck__tostring( lua_State *luaVM )
{
	struct xt_sck *sck = xt_sck_check_ud( luaVM, 1 );
	lua_pushfstring( luaVM, "Socket{%s:%d}: %p",
			(XT_SCK_TCP == sck->t) ? "TCP" : "UDP",
			sck->fd,
			sck );
	return 1;
}

/** -------------------------------------------------------------------------
 * Return the FD int representation of the socket
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int lxt_sck_getfdid( lua_State *luaVM )
{
	struct xt_sck      *sck;

	sck = xt_sck_check_ud( luaVM, 1 );
	lua_pushinteger( luaVM, sck->fd );
	return 1;
}



/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_sck_fm [] = {
	{"__call",      lxt_sck__Call},
	{NULL,   NULL}
};


/**
 * \brief      the socketlibrary definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_sck_m [] =
{
	{"listen",    lxt_sck_listen},
	{"bind",      lxt_sck_bind},
	{"connect",   lxt_sck_connect},
	{"accept",    lxt_sck_accept},
	{"close",     lxt_sck_close},
	{"sendTo",    lxt_sck_send_to},
	{"recvFrom",  lxt_sck_recv_from},
	{"send",      lxt_sck_send_strm},
	{"recv",      lxt_sck_recv_strm},
	{"getId",     lxt_sck_getfdid},
	{"getIp",     lxt_sck_getsockname},
	{NULL,        NULL}
};


/**
 * \brief      the socket library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_sck_cf [] =
{
	{"new",       lxt_sck_New},
	{"bind",      lxt_sck_bind},
	{"connect",   lxt_sck_connect},
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
int luaopen_xt_sck( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "xt.Socket" );   // stack: functions meta
	luaL_newlib( luaVM, xt_sck_m);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_sck__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( luaVM, xt_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( luaVM, xt_sck_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}

