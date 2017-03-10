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


/**--------------------------------------------------------------------------
 * construct a TCP Socket and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Net.Ip4
 * \lparam  ud     T.Net.Tcp userdata instance( socket ).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_tcp__Call( lua_State *L )
{
	struct t_net   __attribute__ ((unused)) *s;

	lua_remove( L, 1 );
	s = t_net_create_ud( L, T_NET_TCP, 1 );
	return 1 ;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net of TCP type.
 * If it's a TCP socket return pointer other wise retrun null.
 * \param   L      Lua state.
 * \param   int    Position on the stack.
 * \return  struct t_net*  pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
struct t_net
*t_net_tcp_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_TCP_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_NET_TCP_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_net *) ud;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Tcp userdata instance( socket ).
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
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Tcp userdata instance( socket ).
 * \lparam  ud     T.Net.IpX userdata instance( socket address ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_bind( lua_State *L )
{
	return t_net_bind( L, T_NET_TCP );
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Tcp userdata instance( client socket ).
 * \lparam  ud     T.Net.IpX userdata instance( client socket address ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_tcp_connect( lua_State *L )
{
	return t_net_connect( L, T_NET_TCP );
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \param   int    position of server socket on stack.
 * \lparam  ud     T.Net.Tcp userdata instance( server socket ).
 * \return  t_net* Client pointer.  Leaves cli_sock and cli_IP on stack.
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
	//t_stackDump( L );

	if ( (cli->fd  =  accept( srv->fd, (struct sockaddr *) &(*si_cli), &cli_sz )) == -1 )
		return t_push_error( L, "couldn't accept from socket" );

	//t_stackDump( L );
	if (-1 == setsockopt( cli->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
		return t_push_error( L, "couldn't make client socket reusable" );
	return 2;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Tcp userdata instance( server socket ).
 * \lreturn ud     T.Net.Tcp userdata instance( new client socket ).
 * \lreturn ud     T.Net.IpX userdata instance( new client sockaddr ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_accept( lua_State *L )
{
	return t_net_tcp_accept( L, 1 );
}


/** -------------------------------------------------------------------------
 * Send some data to a TCP socket.
 * \param   L      Lua state.
 * \param   struct t_net  userdata.
 * \param   char*  char buffer.
 * \param   uint   size of char buffer.
 * \return  number of bytes sent out.
 *-------------------------------------------------------------------------*/
int
t_net_tcp_send( lua_State *L, struct t_net *s, const char* buf, size_t sz )
{
	int     sent;

	if ((sent = send( s->fd, buf, sz, 0 )) == -1)
		return t_push_error( L, "Failed to send TCP message" ) ;
	return sent;
}


/** -------------------------------------------------------------------------
 * Send a message over a TCP socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.TCP userdata instance.
 * \lparam  string msg attempting to send.
 *        OR
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_send( lua_State *L )
{
	struct t_net *sck      = t_net_tcp_check_ud( L, 1, 1 );
	size_t        to_send;      // How much should get send out maximally
	int           canwrite;
	const char   *msg      = t_buf_checklstring( L, 2, &to_send, &canwrite );
	size_t        msg_ofs  = (lua_isinteger( L, 3 )) ? lua_tointeger( L, 3 ) : 0;

	lua_pushinteger( L, t_net_tcp_send( L, sck, msg+msg_ofs, to_send-msg_ofs ) );
	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a TCP socket.
 * \param   L      Lua state.
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
		t_push_error( L, "Failed to receive TCP packet" );
	return rslt;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a TCP socket.
 * If the second parameter is a T.Buffer or a T.Buffer.Segement received data
 * will be written into them.  That automatically caps recieving data to the
 * length of that Buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.TCP userdata instance.
 * \lparam  ud     T.Buffer/Segment userdata instance (optional).
 * \lreturn string The recieved message.
 * \lreturn rcvd   number of bytes recieved.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_tcp_recv( lua_State *L )
{
	struct t_net *sck       = t_net_tcp_check_ud( L, 1, 1 );
	int           canwrite;
	size_t        len;
	char         *rcv       = t_buf_tolstring( L, 2, &len, &canwrite );
	char          buffer[ BUFSIZ ];
	int           rcvd;

	if (NULL == rcv)
	{
		rcv  = &(buffer[0]);
		len  = sizeof (buffer)-1;
	}

	rcvd = t_net_tcp_recv( L, sck, rcv, len );

	// return buffer, length
	if (0 == rcvd)
		lua_pushnil( L );
	else
		lua_pushlstring( L, rcv, rcvd );
	lua_pushinteger( L, rcvd );

	return 2;
}


/** -------------------------------------------------------------------------
 * Recieve IpEndpoint from a TCP socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.TCP userdata instance.
 * \lparam  ud     T.Net.Ip4 userdata instance.
 * \lreturn ud     T.Net.Ip4 userdata instance.
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
	  { "__call",    lt_net_tcp__Call }
	, { NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_tcp_cf [] =
{
	  { "bind",      lt_net_tcp_bind }
	, { "connect",   lt_net_tcp_connect }
	, { "listen",    lt_net_tcp_listen }
	, { NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_tcp_m [] =
{
	// metamethods
	  { "__tostring",  lt_net__tostring }
	//, { "__eq",        lt_net__eq }
	, { "__gc",        lt_net_close }  // reuse function
	// object methods
	, { "listen",      lt_net_tcp_listen }
	, { "bind",        lt_net_tcp_bind }
	, { "connect",     lt_net_tcp_connect }
	, { "accept",      lt_net_tcp_accept }
	, { "close",       lt_net_close }
	, { "send",        lt_net_tcp_send }
	, { "recv",        lt_net_tcp_recv }
	, { "getsockname", lt_net_tcp_getsockname }
	// generic net functions -> reuse functions
	, { "getId",       lt_net_getfdid }
	, { "getFdInfo",   lt_net_getfdinfo }
	, { "setOption",   lt_net_setoption }
	, { NULL,        NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the TCP Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     Lua state.
 * \lreturn table the library
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net_tcp( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_NET_TCP_TYPE );   // stack: functions meta
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
