/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_tcp.c
 * \brief     OOP wrapper around TCP sockets.
 *            read, write, connect, bind etc
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
 * Create a TCP socket and return it.
 * \param   L  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_tcp_New( lua_State *L )
{
	struct t_net   __attribute__ ((unused)) *s;

	s = t_net_create_ud( L, T_NET_TCP, 1 );
	return 1 ;
}


/**--------------------------------------------------------------------------
 * construct a TCP Socket and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table Socket
 * \lparam  string    type "TCP", "UDP", ...
 * \lreturn struct t_net userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_tcp__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_net_tcp_New( L );
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net of TCP type.
 * If it's a TCP socket return pointer other wise retrun null.
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_net*  pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
struct t_net
*t_net_tcp_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Net.TCP" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Net.TCP` expected" );
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
lt_net_tcp_listen( lua_State *L )
{
	return t_net_listen( L, 1, T_NET_TCP );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_bind( lua_State *L )
{
	return t_net_bind( L, T_NET_TCP );
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_tcp_connect( lua_State *L )
{
	return t_net_connect( L, T_NET_TCP );
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L   The lua state.
 * \param   t_net   Server.
 * \param   sockaddr_in*  Client Address pointer.
 * \return  t_net*  Client pointer.  Leaves cli_sock and cli_IP on stack.
 *-------------------------------------------------------------------------*/
int
t_net_tcp_accept( lua_State *L, int pos )
{
	struct t_net       *srv    = t_net_tcp_check_ud( L, pos+0, 1 ); // listening socket
	struct t_net       *cli;                                        // accepted socket
	struct sockaddr_in *si_cli;                                     // peer address
	socklen_t           cli_sz = sizeof( struct sockaddr_in );
	size_t              one    = 1;

	cli     = t_net_create_ud( L, T_NET_TCP, 0 );
	si_cli  = t_net_ip4_create_ud( L );

	if ( (cli->fd  =  accept( srv->fd, (struct sockaddr *) &(*si_cli), &cli_sz )) == -1 )
		return t_push_error( L, "couldn't accept from socket" );

	if (-1 == setsockopt( cli->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
		return t_push_error( L, "couldn't make client socket reusable" );
	return 2;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L   The lua state.
 * \lparam  socket  socket userdata.
 * \lreturn socket  socket userdata for new connection.
 * \lreturn ip      sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_accept( lua_State *L )
{
	return t_net_tcp_accept( L, 1 );
}



/** -------------------------------------------------------------------------
 * Send some data to a TCP socket.
 * \param   L  The lua state.
 * \param   t_net  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes sent out.
 *-------------------------------------------------------------------------*/
int
t_net_tcp_send( lua_State *L, struct t_net *s, const char* buf, size_t sz )
{
	int     rslt;

	if ((rslt = send( s->fd, buf, sz, 0 )) == -1)
		return t_push_error( L, "Failed to send TCP message" ) ;

	return rslt;
}


/** -------------------------------------------------------------------------
 * Send a message over a TCP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_send( lua_State *L )
{
	struct t_net *s;
	struct t_buf *buf;
	size_t        to_send;      // How much should get send out maximally
	const char   *msg;
	size_t        into_msg = 0; // where in the message to start sending from

	s = t_net_tcp_check_ud( L, 1, 1 );
	// check for starting point
	if (lua_isnumber( L, 3 ))
	{
		into_msg = lua_tointeger( L, 3 );
	}
	// do we deal with a lib-t buffer
	if (lua_isuserdata( L, 2 ))
	{
		buf      = t_buf_check_ud( L, 2, 1 );
		msg      = (char *) &(buf->b[ 0 ]);
		to_send  = buf->len;
	}     // or is it a string
	else if (lua_isstring( L, 2 ))
		msg   = lua_tolstring( L, 2, &to_send );
	else
		return t_push_error( L, "ERROR send( socket, msg ) takes msg argument" ) ;

	msg      = msg + into_msg;
	to_send -= into_msg;

	lua_pushinteger( L, t_net_tcp_send( L, s, msg, to_send ) );

	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a TCP socket.
 * \param   L  The lua state.
 * \param   t_net  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_net_tcp_recv( lua_State *L, struct t_net *s, char* buff, size_t sz )
{
	int  rslt;

	if ((rslt = recv( s->fd, buff, sz, 0 )) == -1)
		t_push_error( L, "Failed to recieve TCP packet" );

	return rslt;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a TCP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn string The recieved message.
 * \lreturn rcvd   number of bytes recieved.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_recv( lua_State *L )
{
	struct t_net *s;
	struct t_buf *buf;
	int           rcvd;
	char          buffer[ BUFSIZ ];
	char         *rcv = &(buffer[0]);
	int           len = sizeof (buffer)-1;

	s = t_net_tcp_check_ud( L, 1, 1 );
	if (lua_isuserdata( L, 2 ))
	{
		buf  = t_buf_check_ud( L, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}

	rcvd = t_net_tcp_recv( L, s, rcv, len );

	// return buffer, length
	lua_pushlstring( L, buffer, rcvd );
	lua_pushinteger( L, rcvd );

	return 2;
}


/** -------------------------------------------------------------------------
 * Recieve IpEndpoint from a TCP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  IpEndpoint userdata.
 * \lreturn IpEndpoint userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_getsockname( lua_State *L )
{
	struct t_net       *s;
	struct sockaddr_in *ip;
	socklen_t           ip_len = sizeof( struct sockaddr_in );

	s = t_net_tcp_check_ud( L, 1, 1 );

	if (lua_isuserdata( L, 2 ))  ip = t_net_ip4_check_ud( L, 2, 1 );
	else                         ip = t_net_ip4_create_ud( L );

	getsockname( s->fd, (struct sockaddr*) &(*ip), &ip_len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_tcp_fm [] = {
	{ "__call",    lt_net_tcp__Call },
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_tcp_cf [] =
{
	{ "new",       lt_net_tcp_New },
	{ "bind",      lt_net_tcp_bind },
	{ "connect",   lt_net_tcp_connect },
	{ "listen",    lt_net_tcp_listen },
	{ NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_tcp_m [] =
{
	// metamethods
	{ "__tostring",  lt_net__tostring },
	//{ "__eq",        lt_net__eq },
	{ "__gc",        lt_net_close },  // reuse function
	// object methods
	{ "listen",      lt_net_tcp_listen },
	{ "bind",        lt_net_tcp_bind },
	{ "connect",     lt_net_tcp_connect },
	{ "accept",      lt_net_tcp_accept },
	{ "close",       lt_net_close },
	{ "send",        lt_net_tcp_send },
	{ "recv",        lt_net_tcp_recv },
	{ "getIp",       lt_net_tcp_getsockname },
	// generic net functions -> reuse functions
	{ "getId",       lt_net_getfdid },
	{ "getFdInfo",   lt_net_getfdinfo },
	{ "setOption",   lt_net_setoption },
	{ NULL,        NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the TCP Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net_tcp( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, "T.Net.TCP" );   // stack: functions meta
	luaL_setfuncs( L, t_net_tcp_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_tcp_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_tcp_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
