/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_udp.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP, read write connect bind etc
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t.h"
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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif
#include "t_net.h"
#include "t_buf.h"         // the ability to send and recv buffers


/** -------------------------------------------------------------------------
 * Create a UDP socket and return it.
 * \param   L  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_udp_New( lua_State *L )
{
	struct t_net   __attribute__ ((unused)) *s;

	s = t_net_create_ud( L, T_NET_UDP, 1 );

	return 1 ;
}


/**--------------------------------------------------------------------------
 * Construct a UDP Socket and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table Socket
 * \lparam  string    type "TCP", "UDP", ...
 * \lreturn struct t_net userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_udp__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_net_udp_New( L );
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net of UDP type.
 * If it's a UDP socket return pointer other wise return null.
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_net*  pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
struct t_net
*t_net_udp_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Net.UDP" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Net.UDP` expected" );
	return (NULL==ud) ? NULL : (struct t_net *) ud;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_listen( lua_State *L )
{
	return t_net_listen( L, 1, T_NET_UDP );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_bind( lua_State *L )
{
	return t_net_bind( L, T_NET_UDP );
}


/** -------------------------------------------------------------------------
 * Connect a UDP socket to an address.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_udp_connect( lua_State *L )
{
	return t_net_connect( L, T_NET_UDP );
}


/** -------------------------------------------------------------------------
 * Send Datagram over a UDP socket to an IP endpoint.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \lparam  msg    luastring.
 *       or
 * \lparam  T.Buffer   lua-t BUffer type.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_sendto( lua_State *L )
{
	struct t_net       *s;
	struct sockaddr_in *ip;
	struct t_buf       *buf;
	int                 sent;
	int                 len;
	const char         *msg;

	s   = t_net_udp_check_ud( L, 1, 1 );
	ip  = t_net_ip4_check_ud( L, 2, 1 );
	if (lua_isstring( L, 3 ))
	{
		msg   = lua_tostring( L, 3 );
		len   = strlen( msg );
	}
	else if (lua_isuserdata( L, 3 ))
	{
		buf  = t_buf_check_ud( L, 3, 1 );
		msg  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return t_push_error( L, "ERROR sendTo(socket,ip,msg) takes msg argument" );

	if ((sent = sendto(
	  s->fd,
	  msg, len, 0,
	  (struct sockaddr *) &(*ip), sizeof( struct sockaddr ))
	  ) == -1)
		return t_push_error( L, "Failed to send UDP packet to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	lua_pushinteger( L, sent );
	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve Datagram from a UDP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_recvfrom( lua_State *L )
{
	struct t_net       *s;
	struct t_buf       *buf;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[ BUFSIZ ];
	char               *rcv = &(buffer[ 0 ]);
	int                 len = sizeof( buffer )-1;

	unsigned int        slen = sizeof( si_cli );

	s = t_net_udp_check_ud( L, 1, 1 );
	if (lua_isuserdata( L, 2 )) {
		buf  = t_buf_check_ud ( L, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	si_cli = t_net_ip4_create_ud( L );

	if ((rcvd = recvfrom(
	  s->fd,
	  rcv, len, 0,
	  (struct sockaddr *) &(*si_cli), &slen )
	  ) == -1)
		return t_push_error( L, "Failed to recieve UDP packet" );

	// return buffer, length, IpEndpoint
	lua_pushlstring( L, rcv, rcvd );
	lua_pushinteger( L, rcvd );
	lua_pushvalue( L, -3 );
	return 3;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_udp_fm [] = {
	{ "__call",    lt_net_udp__Call },
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_udp_cf [] =
{
	{ "new",       lt_net_udp_New },
	{ "bind",      lt_net_udp_bind },
	{ "connect",   lt_net_udp_connect },
	{ "listen",    lt_net_udp_listen },
	{ NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_udp_m [] =
{
	// metamethods
	{ "__tostring",  lt_net__tostring },
	//{ "__eq",        lt_net__eq },
	{ "__gc",        lt_net_close },  // reuse function
	// object methods
	{ "listen",      lt_net_udp_listen },
	{ "bind",        lt_net_udp_bind },
	{ "connect",     lt_net_udp_connect },
	{ "close",       lt_net_close },
	{ "sendto",      lt_net_udp_sendto },
	{ "recvfrom",    lt_net_udp_recvfrom },
	// generic net functions -> reuse functions
	{ "getId",       lt_net_getfdid },
	{ "getFdInfo",   lt_net_getfdinfo },
	{ "setOption",   lt_net_setoption },
	{ NULL,        NULL }
};



/**--------------------------------------------------------------------------
 * Pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net_udp( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_NET_UDP_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_udp_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_udp_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_udp_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
