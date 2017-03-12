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


/**--------------------------------------------------------------------------
 * Construct a UDP Socket and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Net.UDP
 * \lparam  ud     T.Net.Udp userdata instance( socket ).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_udp__Call( lua_State *L )
{
	struct t_net   __attribute__ ((unused)) *s;

	lua_remove( L, 1 );
	s = t_net_create_ud( L, T_NET_UDP, 1 );
	return 1 ;
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
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Udp userdata instance( socket ).
 * \lparam  ud     T.Net.Ip4 userdata instance( ipaddr ).
 * \lparam  int    port to listen on.
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
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Udp userdata instance( socket ).
 * \lparam  ud     T.Net.IpX userdata instance( socket address ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_bind( lua_State *L )
{
	return t_net_bind( L, T_NET_UDP );
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Udp userdata instance( client socket ).
 * \lparam  ud     T.Net.IpX userdata instance( client socket address ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_udp_connect( lua_State *L )
{
	return t_net_connect( L, T_NET_UDP );
}


/** -------------------------------------------------------------------------
 * Send some data via a UDP socket.
 * \param   L       Lua state.
 * \param   sck     struct t_net       pointer userdata.
 * \param   addr    struct sockaddr_in pointer userdata.
 * \param   buf     char* buffer.
 * \param   len     size of char buffer.
 * \return  sent    int; number of bytes sent out.
 *-------------------------------------------------------------------------*/
static int
t_net_udp_send( lua_State *L, struct t_net *sck, struct sockaddr_in *addr, const char* buf, size_t len )
{
	int  sent;

	if ((sent = sendto(
	  sck->fd,
	  buf, len, 0,
	  (struct sockaddr *) &(*addr), sizeof( struct sockaddr ))
	  ) == -1)
		return t_push_error( L, "Failed to send UDP packet to %s:%d",
					 inet_ntoa( addr->sin_addr ),
					 ntohs(     addr->sin_port ) );

	return sent;
}


/** -------------------------------------------------------------------------
 * Send a message over a UDP socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.UDP userdata instance.
 * \lparam  ud     T.Net.IP4 userdata instance.
 * \lparam  string msg attempting to send.
 *       OR
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lparam  ofs    Offset in string or buffer.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_send( lua_State *L )
{
	struct t_net       *sck      = t_net_udp_check_ud( L, 1, 1 );
	struct sockaddr_in *ip       = t_net_ip4_check_ud( L, 2, 1 );
	int                 canwrite;
	size_t              len;      // Cap amount to send
	const char         *msg      = t_buf_checklstring( L, 3, &len, &canwrite );
	size_t              msg_ofs  = (lua_isinteger( L, 4 )) ? lua_tointeger( L, 4 ) : 0;
	int                 sent;

	sent = t_net_udp_send( L, sck, ip, msg+msg_ofs, len-msg_ofs );
	lua_pushinteger( L, sent );
	return 1;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a UDP socket.
 * \param   L            Lua state.
 * \param   t_net        userdata.
 * \param   sockaddr_in  userdata.
 * \param   buff         char buffer.
 * \param   sz           size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
static int
t_net_udp_recv( lua_State *L, struct t_net * sck, struct sockaddr_in *addr, char *buf, size_t len)
{
	int                 rcvd;
	unsigned int        slen     = sizeof( addr );

	if ((rcvd = recvfrom( sck->fd, buf, len, 0, (struct sockaddr *) &(*addr), &slen )) == -1)
		return t_push_error( L, "Failed to recieve UDP packet" );
	return rcvd;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a UDP socket.
 * If the second parameter is a T.Buffer or a T.Buffer.Segement received data
 * will be written into them.  That automatically caps recieving data to the
 * length of that Buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.UDP userdata instance.
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_udp_recv( lua_State *L )
{
	struct t_net       *sck      = t_net_udp_check_ud( L, 1, 1 );
	struct sockaddr_in *si_cli   = t_net_ip4_create_ud( L );
	int                 rcvd;
	int                 canwrite;
	size_t              len;
	char                buffer[ BUFSIZ ];
	char               *rcv      = t_buf_tolstring( L, 2, &len, &canwrite );

	if (NULL == rcv)
	{
		rcv  = &(buffer[0]);
		len  = sizeof (buffer)-1;
	}

	rcvd = t_net_udp_recv( L, sck, si_cli, rcv, len );

	// return buffer, length, IpEndpoint
	if (0 == rcvd)
		lua_pushnil( L );
	else
		lua_pushlstring( L, rcv, rcvd );
	lua_pushinteger( L, rcvd );
	lua_pushvalue( L, -3 );
	return 3;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_udp_fm [] = {
	  { "__call",    lt_net_udp__Call }
	, { NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_udp_cf [] =
{
	  { "bind",      lt_net_udp_bind }
	, { "connect",   lt_net_udp_connect }
	, { "listen",    lt_net_udp_listen }
	, { NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_udp_m [] =
{
	// metamethods
	  { "__tostring",  lt_net__tostring }
	//, { "__eq",        lt_net__eq }
	, { "__gc",        lt_net_close }  // reuse function
	// object methods
	, { "listen",      lt_net_udp_listen }
	, { "bind",        lt_net_udp_bind }
	, { "connect",     lt_net_udp_connect }
	, { "close",       lt_net_close }
	, { "sendto",      lt_net_udp_send }
	, { "recvfrom",    lt_net_udp_recv }
	// generic net functions -> reuse functions
	, { "getId",       lt_net_getfdid }
	, { "getFdInfo",   lt_net_getfdinfo }
	, { "setOption",   lt_net_setoption }
	, { NULL,        NULL }
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
