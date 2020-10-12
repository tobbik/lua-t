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
#include <errno.h>      // errno
#include <signal.h>     // signal( SIGPIPE, SIG_IGN )

#include "t_net_l.h"
#include "t_tim.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Push function related error and format the errno on stack.
 * \param   lua_State Lua.
 * \param   t_net_adr Address struct.
 * \param   msg       char pointer to formatting message.
 * \return  returns 1 which is # of elements left on stack.
 * --------------------------------------------------------------------------*/
static int
p_net_sck_pushErrno( lua_State *L, struct sockaddr_storage *adr, const char *msg, ... )
{
	char                     dst[ INET6_ADDRSTRLEN ];
	va_list                  argp;

	lua_pushboolean( L, 0==1 );
	va_start( argp, msg );
	luaL_where( L, 1 );
	lua_pushvfstring( L, msg, argp );
	va_end( argp );
	if (NULL == adr)
		lua_pushfstring( L, " (%s)", strerror( errno ) );
	else
	{
		SOCK_ADDR_INET_NTOP( adr, dst );
		lua_pushfstring( L, " %s:%d (%s)", dst,
		   ntohs( SOCK_ADDR_SS_PORT( adr ) ), strerror( errno ) );
	}
	lua_concat( L, 3 );
	lua_pushinteger( L, errno );
	return 3;
}


/**--------------------------------------------------------------------------
 * Create the actual socket handle.
 * \param   L        Lua state.
 * \param   sck      struct t_net_sck*
 * \param   family   int AF_INET, AF_INET6, ...
 * \param   protocol int IPPROTO_UDP, IPPROTO_TCP, ...
 * \param   type     int SOCK_STREAM, SOCK_DGRAM, ...
 * --------------------------------------------------------------------------*/
void
p_net_sck_createHandle( lua_State *L, struct t_net_sck *sck, int family, int type, int protocol )
{
	sck->fd = socket( family, type, protocol );
	if (-1 == sck->fd)
		t_push_error( L, "Can't create socket" );
}


/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L    Lua state.
 * \param   sck* struct t_net_sck pointer.
 * --------------------------------------------------------------------------*/
int
p_net_sck_close( lua_State *L, struct t_net_sck *sck )
{
	if (-1 != sck->fd)
	{
		if (-1 == close( sck->fd ))
			return p_net_sck_pushErrno( L, NULL, "Can't close socket" );
		else
			sck->fd = -1;         // invalidate socket
	}

	return 0;
}


/** -------------------------------------------------------------------------
 * Shutdown a socket.
 * \param   L        Lua state.
 * \param   sck*     struct t_net_sck pointer.
 * \param   shutVal  int; SHUT_* value.
 * --------------------------------------------------------------------------*/
int
p_net_sck_shutDown( lua_State *L, struct t_net_sck *sck, int shutVal )
{
	if (-1 != sck->fd)
	{
		if (-1 == shutdown( sck->fd, shutVal ))
			return p_net_sck_pushErrno( L, NULL, "Can't shutdown socket" );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L        Lua state.
 * \param   sck*     struct t_net_sck pointer.
 * \param   bl       int; backlog value for listen() syscall.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
p_net_sck_listen( lua_State *L, struct t_net_sck *sck, const int bl )
{
	if (-1 == listen( sck->fd, bl ))
		return p_net_sck_pushErrno( L, NULL, "Can't listen on socket" );
	else
		return 1;
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
p_net_sck_bind( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr )
{
	if (-1 == bind( sck->fd, SOCK_ADDR_PTR( adr ), SOCK_ADDR_SS_LEN( adr ) ))
		return p_net_sck_pushErrno( L, adr, "Can't bind socket to" );
	else
	{
		lua_pushboolean( L, 1 );
		return 1;
	}
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     sockaddr_storage userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
p_net_sck_connect( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr )
{
	if (-1 == connect( sck->fd, SOCK_ADDR_PTR( adr ), SOCK_ADDR_SS_LEN( adr ) ))
		return p_net_sck_pushErrno( L, adr, "Can't connect socket" );
	else
	{
		lua_pushboolean( L, 1 );
		return 1;
	}
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   lua_State L.
 * \param   t_net_sck Socket Struct for server.
 * \param   t_net_sck Socket Struct for client.
 * \param   t_net_adr Address Struct indication client.
 * \return  2 == success for client and address; 0 == error;
 *-------------------------------------------------------------------------*/
int
p_net_sck_accept( lua_State *L, struct t_net_sck *srv, struct t_net_sck *cli,
                                struct sockaddr_storage *adr )
{
	socklen_t adr_len = SOCK_ADDR_SS_LEN( adr );

	if (-1 == (cli->fd = accept( srv->fd, SOCK_ADDR_PTR( adr ), &adr_len )))
		return p_net_sck_pushErrno( L, adr, "Can't accept on socket bound" );
	//printf("ACCEPTING HANDLE: %d\n", cli->fd);

	return 2;
}


/** -------------------------------------------------------------------------
 * Send some data via socket.
 * \param   L       Lua state.
 * \param   sck     struct t_net_sck        pointer userdata.
 * \param   adr     struct sockaddr_storage pointer userdata.
 * \param   buf     char* buffer.
 * \param   len     how many bytes to send from the buffer.
 * \return  snt    int; number of bytes sent out.
 *-------------------------------------------------------------------------*/
ssize_t
p_net_sck_send( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr,
                              const char* buf, size_t len )
{
	ssize_t snt;

	if (-1 == (snt = sendto(
	  sck->fd,
	  buf, len, 0, SOCK_ADDR_PTR( adr ), SOCK_ADDR_SS_LEN( adr ))))
	{
		p_net_sck_pushErrno( L, adr,
			(NULL == adr) ? "Can't send message" : "Can't send Message to" );
	}
	return snt;
}


/** -------------------------------------------------------------------------
 * Recieve some data from socket.
 * \param   L       Lua state.
 * \param   sck     struct t_net_sck        pointer userdata.
 * \param   adr     struct sockaddr_storage pointer userdata.
 * \param   buf     char* buffer.
 * \param   len     how many bytes to recieve into the the buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
ssize_t
p_net_sck_recv( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr,
                              char *buf, size_t len )
{
	ssize_t       rcvd;
	socklen_t adr_len = SOCK_ADDR_SS_LEN( adr );

	if (-1 == (rcvd = recvfrom(
	  sck->fd,
	  buf, len, 0,
	  SOCK_ADDR_PTR( adr ), &adr_len)))
	{
		p_net_sck_pushErrno( L, adr,
			(NULL == adr) ? "Can't receive message" : "Can't receive Message from" );
	}
	return rcvd;
}


/** -------------------------------------------------------------------------
 * Recieve sockaddr_storage a socket is bound to.
 * \param   L      Lua state.
 * \param  ud      Net.Socket userdata instance.
 * \param  ud      Net.Address userdata instance.
 * \return success bool; was address received.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
p_net_sck_getsockname( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr )
{
	socklen_t adr_len = SOCK_ADDR_SS_LEN( adr );

	if (-1==getsockname( sck->fd, SOCK_ADDR_PTR( adr ), &adr_len ) )
		return p_net_sck_pushErrno( L, NULL, "Couldn't get Address from" );
	else
		return 1;
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
p_net_sck_mkFdSet( lua_State *L, int pos, fd_set *set )
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
p_net_sck_getSocketOption( lua_State *L, struct t_net_sck        *sck,
                                         struct t_net_sck_option *opt )
{
	struct sockaddr_storage   adr;
	int                       ival;
	struct timeval            tv;
	socklen_t                 len;

	switch (opt->type)
	{
		case T_NET_SCK_OTP_FCNTL:
			if (-1 == (ival = fcntl( sck->fd, opt->getlevel )))
				lua_pushboolean( L, 0 );
			else
				lua_pushboolean( L, (ival & opt->option) == opt->option );
			break;
		case T_NET_SCK_OTP_IOCTL:
			if (ioctl( sck->fd, opt->getlevel, &ival ) < 0)
				lua_pushboolean( L, 0 );
			else
				lua_pushboolean( L, ival);
			break;
		case T_NET_SCK_OTP_BOOL:
			len = sizeof( ival );
			if (getsockopt( sck->fd, opt->getlevel, opt->option, &ival, &len ) < 0)
				lua_pushboolean( L, 0);
			else
				lua_pushboolean( L, ival );
			break;
		case T_NET_SCK_OTP_INT:
			len = sizeof( ival );
			if (getsockopt( sck->fd, opt->getlevel, opt->option, &ival, &len ) < 0)
				lua_pushinteger( L, -1 );
			else
				lua_pushinteger( L, ival );
			break;
		case T_NET_SCK_OTP_TIME:
			len = sizeof( tv );
			if (getsockopt( sck->fd, opt->getlevel, opt->option, &tv, &len ) < 0)
				lua_pushinteger( L, -1 );
			else
				t_tim_create_ud( L, &tv );     // push t.Time userdata
			break;
		case T_NET_SCK_OTP_DSCR:
			if (-1==sck->fd) lua_pushnil( L );
			else             lua_pushinteger( L, sck->fd );
			break;
		// Special cases returning strings
		case T_NET_SCK_OTP_FMLY:
			len = sizeof( struct sockaddr_storage );
			if (0 == getsockname( sck->fd, SOCK_ADDR_PTR( &adr ), &len ))
			{
				lua_pushinteger( L, SOCK_ADDR_SS_FAMILY( &adr ) );
				t_net_getFamilyValue( L, -1 );
			}
			else
				lua_pushnil( L );
			break;
		case T_NET_SCK_OTP_PRTC:
			len = sizeof( ival );
			if (getsockopt( sck->fd, opt->getlevel, opt->option, &ival, &len ) < 0)
				lua_pushnil( L );
			else
			{
				luaL_getsubtable( L, LUA_REGISTRYINDEX, "_LOADED" );
				lua_getfield( L, -1, "t."T_NET_IDNT );
				lua_getfield( L, -1, T_NET_SCK_IDNT );
				lua_getfield( L, -1, T_NET_SCK_PTC_IDNT );
				lua_pushinteger( L, ival );
				lua_rawget( L, -2 );      //S: sck key _LD net sck prt
				lua_rotate( L, -1, 5 );   //S: sck prt key _LD net sck
				lua_pop( L, 4 );
			}
			break;
		case T_NET_SCK_OTP_TYPE:
			len = sizeof( ival );
			if (getsockopt( sck->fd, opt->getlevel, opt->option, &ival, &len ) < 0)
				lua_pushnil( L );
			else
			{
				lua_pushinteger( L, ival );
				t_getTypeByValue( L, -1, -1, t_net_typeList );
			}
			break;

		default:
			// should never get here
			luaL_error( L, "unknown socket option: `%s`", lua_tostring( L, 2 ) );
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
p_net_sck_setSocketOption( lua_State *L, struct t_net_sck        *sck,
                                         struct t_net_sck_option *opt )
{
	int                       ival;
	struct timeval           *tv;

	switch (opt->type)
	{
		case T_NET_SCK_OTP_FCNTL:
			ival  = fcntl( sck->fd, opt->getlevel, 0 );
			if (ival > 0)
			{
				if (lua_toboolean( L, 3 ))
					ival |=  opt->option;
				else
					ival &=  opt->option;
				if (fcntl( sck->fd, opt->setlevel, ival ) < 0)
					return p_net_sck_pushErrno( L, NULL, "Can't set socket option `%s`", lua_tostring( L, 2 ) );
			}
			else
				return p_net_sck_pushErrno( L, NULL, "Can't set socket option `%s`", lua_tostring( L, 2 ) );
			break;
		case T_NET_SCK_OTP_BOOL:
			ival = lua_toboolean( L, 3 );
			if (setsockopt( sck->fd, opt->getlevel, opt->option, &ival, sizeof( ival ) ) < 0)
				return p_net_sck_pushErrno( L, NULL, "Can't set socket option `%s`", lua_tostring( L, 2 ) );
			break;
		case T_NET_SCK_OTP_INT:
			ival = luaL_checkinteger( L, 3 );
			if (setsockopt( sck->fd, opt->getlevel, opt->option, &ival, sizeof( ival ) ) < 0)
				return p_net_sck_pushErrno( L, NULL, "Can't set socket option `%s`", lua_tostring( L, 2 ) );
			break;
		case T_NET_SCK_OTP_TIME:
			tv = t_tim_check_ud( L, 3, 1 );
			if (setsockopt( sck->fd, opt->getlevel, opt->option, tv, sizeof( struct timeval ) ) < 0)
				return p_net_sck_pushErrno( L, NULL, "Can't set socket option `%s`", lua_tostring( L, 2 ) );
			break;
		default:
			// should never get here
			return luaL_error( L, "unknown socket option: %s", lua_tostring( L, 2 ) );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Open socket functionality for UNIX
 * \param   void.
 * \return  fine.
 *-------------------------------------------------------------------------*/
int
p_net_sck_open( void )
{
	// otherwise sck:send( "foo" ) on unconnected socket crashes silently and
	// testing that is too expensive for non-blocking sockets on each send()
	signal( SIGPIPE, SIG_IGN );
	return 1;
}



