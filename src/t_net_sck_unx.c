/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP/RAW, read write connect listen bind etc
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

/*
#ifdef _WIN32
#include <WinSock2.h>
#include <winsock.h>
#include <time.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
*/

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "t.h"
#include "t_net.h"
#include "t_buf.h"         // the ability to send and recv buffers


/**--------------------------------------------------------------------------
 * Create the actual socket handle.
 * \param   L        Lua state.
 * \param   sck      struct t_net_sck*
 * \param   family   int AF_INET, AF_INET6, ...
 * \param   protocol int IPPROTO_UDP, IPPROTO_TCP, ...
 * \param   type     int SOCK_STREAM, SOCK_DGRAM, ...
 * --------------------------------------------------------------------------*/
void
t_net_sck_createHandle( lua_State *L, struct t_net_sck *sck, int family, int type, int protocol )
{
	sck->fd = socket( family, type, protocol );
	if (-1 == sck->fd)
		t_push_error( L, "Can't create socket" );
}


/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L    Lua state.
 * \param   struct t_net_sck pointer.
 *-------------------------------------------------------------------------*/
int
t_net_sck_close( lua_State *L, struct t_net_sck *sck )
{
	if (-1 != sck->fd)
	{
		if (-1 == close( sck->fd ))
			return t_push_error( L, "Can't close socket" );
		else
			sck->fd = -1;         // invalidate socket
	}

	return 0;
}


/** -------------------------------------------------------------------------
 * Shutdown a socket.
 * \param   L        Lua state.
 * \param   struct t_net_sck pointer.
 * \param   shutVal  int; SHUT_* value.
 *-------------------------------------------------------------------------*/
int
t_net_sck_shutDown( lua_State *L, struct t_net_sck *sck, int shutVal )
{
	if (-1 != sck->fd)
	{
		if (-1 == shutdown( sck->fd, shutVal ))
			return t_push_error( L, "Can't shutdown socket" );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L      Lua state.
 * \lparam  int    position on stack where socket might be.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_listen( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *adr, const int bl )
{
	struct sockaddr_in  bnd;   ///< if needed, the address the port is bound to

	if (NULL!=adr && -1 == bind( sck->fd , (struct sockaddr*) &(*adr), sizeof( struct sockaddr ) ))
		return t_push_error( L, "Can't bind socket to %s:%d before listen()",
				 inet_ntoa( adr->sin_addr ),
				 ntohs( adr->sin_port ) );

	if (-1 == listen( sck->fd, bl ))
		return t_push_error( L, "Can't listen() on socket" );

	// adr is, if created, by t_net_getdef(), which guarantees an unset port to
	// be 0
	if (NULL!=adr && 0 == ntohs( adr->sin_port ))
	{
		if (t_net_sck_getsockname( sck, &bnd ))
			adr->sin_port = bnd.sin_port;
	}

	return 1;
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_bind( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *adr )
{
	if (bind( sck->fd , (struct sockaddr*) &(*adr), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "Can't bind socket to %s:%d",
					 inet_ntoa( adr->sin_addr ),
					 ntohs( adr->sin_port ) );
	else
		return 1;
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_connect( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *adr )
{
	if (connect( sck->fd , (struct sockaddr*) &(*adr), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "Can't connect to socket on %s:%d",
					 inet_ntoa(adr->sin_addr),
					 ntohs(adr->sin_port) );
	else
		return 1;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \param   int    position of server socket on stack.
 * \lparam  ud     T.Net.Socket userdata instance( server socket ).
 * \return  t_net* Client pointer.  Leaves cli_sock and cli_IP on stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_accept( lua_State *L, struct t_net_sck *srv, struct t_net_sck *cli, struct sockaddr_in *adr )
{
	socklen_t  cl_sz = sizeof( struct sockaddr_in );

	if ( (cli->fd  =  accept( srv->fd, (struct sockaddr *) &(*adr), &cl_sz )) == -1 )
		return t_push_error( L, "Can't accept from socket" );

	return 2;
}


/** -------------------------------------------------------------------------
 * Send some data via socket.
 * \param   L       Lua state.
 * \param   sck     struct t_net_sck   pointer userdata.
 * \param   addr    struct sockaddr_in pointer userdata.
 * \param   buf     char* buffer.
 * \param   len     size of char buffer.
 * \return  sent    int; number of bytes sent out.
 *-------------------------------------------------------------------------*/
int
t_net_sck_send( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *addr, const char* buf, size_t len )
{
	int sent;

	if (-1 == (sent = sendto(
	  sck->fd,
	  buf, len, 0,
	  (struct sockaddr *) &(*addr), sizeof( struct sockaddr ))))
	{
		if (NULL == addr)
			return t_push_error( L, "Can't send message" );
		else
			return t_push_error( L, "Can't send message to %s:%d",
					 inet_ntoa( addr->sin_addr ),
					 ntohs(     addr->sin_port ) );
	}

	return sent;
}


/** -------------------------------------------------------------------------
 * Recieve some data from socket.
 * \param   L            Lua state.
 * \param   t_net_sck    userdata.
 * \param   sockaddr_in  userdata.
 * \param   buff         char buffer.
 * \param   sz           size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_net_sck_recv( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *addr, char *buf, size_t len )
{
	int          rcvd;
	unsigned int sLen     = sizeof( addr );

	if (-1 == (rcvd = recvfrom(
	  sck->fd,
	  buf, len, 0,
	  (struct sockaddr *) &(*addr), &sLen)))
	{
		return t_push_error( L, "Can't recieve message" );
	}
	return rcvd;
}


/** -------------------------------------------------------------------------
 * Recieve sockaddr_in a socket is bound to.
 * \param   L      Lua state.
 * \param  ud      T.Net.Socket userdata instance.
 * \param  ud      T.Net.Ip4 userdata instance.
 * \return success bool; was address received.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_getsockname( struct t_net_sck *sck, struct sockaddr_in *adr )
{
	socklen_t adrLen = sizeof( struct sockaddr_in );

	return 0 == getsockname( sck->fd, (struct sockaddr*) &(*adr), &adrLen );
}


/** -------------------------------------------------------------------------
 * Helper to take sockets from Lua tables to FD_SET.
 * Itertates over the table puls out the socket structs and adds the actual
 * sockets to the fd_set.
 * \param   L      Lua state.
 * \param   int    position on stack where table is located.
 * \param  *fd_set the set of sockets(fd) to be filled.
 * \param  *int    the maximum socket(fd) value.
 * \return  maxFd  highest FD number in set.
 *-------------------------------------------------------------------------*/
int
t_net_sck_mkFdSet( lua_State *L, int pos, fd_set *set )
{
	struct t_net_sck  *sck;
	int                maxFd = -1;

	if (lua_isnil( L, pos) )                // empty table == nil
		return maxFd;
	luaL_checktype( L, pos, LUA_TTABLE );   // only accept tables
	FD_ZERO( set );

	// adding all sd to FD_SET
	lua_pushnil( L );
	while (lua_next( L, pos ))
	{
		sck   = t_net_sck_check_ud( L, -1, 1 );
		maxFd = (sck->fd > maxFd) ? sck->fd : maxFd;
		FD_SET( sck->fd, set );
		lua_pop( L, 1 );   // remove the socket, keep key for next()
	}
	return maxFd;
}


/** -------------------------------------------------------------------------
 * Get socket option values on stack.
 * \param   L        Lua state.
 * \param   sckOpt   int Socket option number.
 * \param   optName  const char* Socket option name.
 * \lreturn value  int or bool socket option value or function.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_getSocketOption( lua_State *L, struct t_net_sck *sck, int sckOpt,
                                         const char       *sckOptName )
{
	struct sockaddr   adr;
	socklen_t      adrLen = sizeof( adr );
	int               val;
	socklen_t         len = sizeof( val );

	switch (sckOpt)
	{
		case O_NONBLOCK:
			val = fcntl( sck->fd, F_GETFL );
			lua_pushboolean( L, (-1==val) ? 0 :(val & O_NONBLOCK) == O_NONBLOCK );
			break;

		// returning integer values
		case SO_RCVLOWAT:
		case SO_RCVTIMEO:
		case SO_SNDBUF:
		case SO_SNDLOWAT:
		case SO_SNDTIMEO:
		case SO_ERROR:
		case SO_RCVBUF:
			if (getsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, &len ) < 0)
				lua_pushinteger( L, -1 );
			else
				lua_pushinteger(L, val );
			break;

		// returning booleans flags
		case SO_BROADCAST:
		case SO_DEBUG:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_OOBINLINE:
		case SO_REUSEADDR:
#ifdef SO_USELOOPBACK
		case SO_USELOOPBACK:
#endif
#ifdef SO_REUSEPORT
		case SO_REUSEPORT:
#endif
			if (getsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, &len ) < 0)
				lua_pushboolean( L, 0 );
			else
				lua_pushboolean(L, val );
			break;

		// Special cases returning strings
		case T_NET_SO_FAMILY:
			if (0 == getsockname( sck->fd, &adr, &adrLen ))
			{
				lua_pushinteger( L, adr.sa_family );
				t_getTypeByValue( L, -1, -1, t_net_familyList );
			}
			else
				lua_pushnil( L );
			break;
#ifdef SO_PROTOCOL
		case SO_PROTOCOL:
			if (getsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, &len ) < 0)
				lua_pushnil( L );
			else
			{
				lua_pushinteger( L, val );
				t_net_getProtocolByValue ( L, -1, -1 );
			}
			break;
#endif
		case SO_TYPE:
			if (getsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, &len ) < 0)
				lua_pushnil( L );
			else
			{
				lua_pushinteger( L, val );
				t_getTypeByValue( L, -1, -1, t_net_typeList );
			}
			break;

		default:
			// should never get here
			luaL_error( L, "unknown socket option: %s", sckOptName );
	}
	return 1;
}


/** -------------------------------------------------------------------------
 * Set socket option values.
 * \param   L        Lua state.
 * \param   sckOpt   int Socket option number.
 * \param   optName  const char* Socket option name.
 * \param   val      int Value to set option to.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_setSocketOption( lua_State *L, struct t_net_sck *sck , int sckOpt,
                                         const char *sckOptName, int val )
{
	int    flags;

	switch (sckOpt)
	{
		case O_NONBLOCK:
			flags = fcntl( sck->fd, F_GETFL );
			if (flags > 0)
			{
				if (val)
					flags |= sckOpt;
				else
					flags &= ~sckOpt;
				if (fcntl( sck->fd, F_SETFL, flags ) < 0)
					return t_push_error( L, "Can't set socket option" );
			}
			else
				return t_push_error( L, "Can't set socket option" );
			break;

		case SO_RCVLOWAT:
		case SO_RCVTIMEO:
		case SO_SNDBUF:
		case SO_SNDLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVBUF:
			if (setsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, sizeof( val ) ) < 0)
				return t_push_error( L, "Can't set socket option" );
			break;

		case SO_BROADCAST:
		case SO_DEBUG:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_OOBINLINE:
		case SO_REUSEADDR:
#ifdef SO_USELOOPBACK
		case SO_USELOOPBACK:
#endif
#ifdef SO_REUSEPORT
		case SO_REUSEPORT:
#endif
			if (setsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, sizeof( val ) ) < 0)
				return t_push_error( L, "Can't set socket option" );
			break;

		case SO_ERROR:
#ifdef SO_PROTOCOL
		case SO_PROTOCOL:
#endif
		case SO_TYPE:
			return luaL_error( L, "Can't set readonly socket option: %s", sckOptName );
		default:
			// should never get here
			return luaL_error( L, "unknown socket option: %s", sckOptName );
	}
	return 0;
}

