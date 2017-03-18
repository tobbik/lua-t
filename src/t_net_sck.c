/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP/RAW, read write connect listen bind etc
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif
#include "t_net.h"
#include "t_buf.h"         // the ability to send and recv buffers


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \lparam  protocol string: 'TCP', 'UDP' ...
 * \lparam  family   string: 'ip4', 'ip6', 'raw' ...
 * \lparam  type     string: 'stream', 'datagram' ...
 * \usage   T.Net.Socket( )                   -> create TCP IPv4 Socket
 *          T.Net.Socket( 'TCP' )             -> create TCP IPv4 Socket
 *          T.Net.Socket( 'TCP', 'ip4' )      -> create TCP IPv4 Socket
 *          T.Net.Socket( 'UDP', 'ip4' )      -> create UDP IPv4 Socket
 *          T.Net.Socket( 'UDP', 'ip6' )      -> create UDP IPv6 Socket
 * \return  struct t_net_sck pointer to the socket struct.
 * --------------------------------------------------------------------------*/
static int
lt_net_sck__Call( lua_State *L )
{
	struct t_net_sck *sck;

	lua_remove( L, 1 );         // remove CLASS table

	//printf("CREATE PROTO:  "); t_stackDump( L );
	t_net_getProtocolByName( L, 1, "TCP" );
	//printf("CREATE FAMILY: "); t_stackDump( L );
	t_getTypeByName( L, 2, "AF_INET", t_net_familyList );
	//printf("CREATE TYPE:   "); t_stackDump( L );
	t_getTypeByName( L, 3,
	   (IPPROTO_TCP == luaL_checkinteger( L, 1 ))
	      ? "SOCK_STREAM"
	      : (IPPROTO_UDP == luaL_checkinteger( L, 1 ))
	         ? "SOCK_DGRAM"
	         : "SOCK_RAW",
	   t_net_typeList );

	//printf("CREATE SOCKET: "); t_stackDump( L );

	sck = t_net_sck_create_ud( L,
	   (AF_UNIX==luaL_checkinteger( L, 2 )) ? 0 : lua_tointeger( L, 2 ),
	   luaL_checkinteger( L, 3 ),
	   luaL_checkinteger( L, 1 ),
	   1 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \param   family   int AF_INET, AF_INET6, ...
 * \param   protocol int IPPROTO_UDP, IPPROTO_TCP, ...
 * \param   type     int SOCK_STREAM, SOCK_DGRAM, ...
 * \param   create   bool, should socket be created or just wrapping userdata.
 * \return  struct t_net_sck* pointer to the socket struct.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_create_ud( lua_State *L, int family, int type, int protocol, int create )
{
	struct t_net_sck *sck  = (struct t_net_sck *) lua_newuserdata( L, sizeof( struct t_net_sck ) );

	if (create)
		sck->fd = socket( family, type, protocol );
	if (sck->fd == -1)
		t_push_error( L, (create) ? "couldn't create socket" : "socket was already closed" );
	luaL_getmetatable( L, T_NET_SCK_TYPE );
	lua_setmetatable( L, -2 );

	return sck;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net.
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \return  struct t_net_sck*  pointer to the struct t_net_sck.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_SCK_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_NET_SCK_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_net_sck *) ud;
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
		//printf( "closing socket: %d\n", s->fd );
		if (-1 == close( sck->fd ))
			return t_push_error( L, "Can't close socket" );
		else
			sck->fd = -1;         // invalidate socket
	}

	return 0;
}


/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L    Lua state.
 * \lparam  ud   t_net_sck userdata instance.
 * \return  int  # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_close( lua_State *L )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, 1, 1 );
	return t_net_sck_close( L, sck );
}


/** -------------------------------------------------------------------------
 * Return the FD int representation of the socket
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_sck_getFd( lua_State *L )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 1 );
	lua_pushinteger( L, sck->fd );
	return 1;
}


/**--------------------------------------------------------------------------
 * Prints out the socket.
 * \param   L      Lua state.
 * \lparam  ud     t_net userdata instance.
 * \lreturn string formatted string representing socket.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_sck__tostring( lua_State *L )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, 1, 1 );

	lua_pushfstring( L, T_NET_SCK_TYPE"{%d}: %p"
		, sck->fd
		, sck );
	return 1;
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
t_net_sck_listen( lua_State *L, const int pos )
{
	struct t_net_sck   *sck = t_net_sck_check_ud( L, pos, 0 );
	struct sockaddr_in *adr = t_net_ip4_check_ud( L, pos+((NULL==sck) ? 0:1), 0 );
	struct sockaddr_in  bnd;   ///< if needed, the address the port is bound to
	int                 bl  = SOMAXCONN, returnables = 0;

	if (lua_isinteger( L, -1 ) && LUA_TSTRING != lua_type( L, -2 ))
	{
		bl = lua_tointeger( L, -1 );
		lua_pop( L, 1 );
	}
	if (NULL!=sck && 1==lua_gettop( L ))
		; // No address, or host like info given -> assume it's bound already
	else
		returnables += t_net_getdef( L, pos, &sck, &adr );

	if (adr != NULL)
	{
		if (bind( sck->fd , (struct sockaddr*) &(*adr), sizeof( struct sockaddr ) ) == -1)
			return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( adr->sin_addr ),
					 ntohs( adr->sin_port ) );
	}

	if (-1 == listen( sck->fd, bl ))
		return t_push_error( L, "ERROR listen to socket" );

	// adr is, if created, by t_net_getdef(), which guarantees an unset port to
	// be 0
	if (NULL!=adr && 0 == ntohs( adr->sin_port ))
	{
		if (t_net_sck_getsockname( sck, &bnd ))
			adr->sin_port = bnd.sin_port;
	}

	return returnables;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Sck userdata instance( socket ).
 * \lparam  ud     T.Net.Ip4 userdata instance( ipaddr ).
 * \lparam  int    port to listen on.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_listen( lua_State *L )
{
	return t_net_sck_listen( L, 1 );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_bind( lua_State *L, const int pos )
{
	struct t_net_sck   *sck         = NULL;
	struct sockaddr_in *ip          = NULL;
	int                 returnables = t_net_getdef( L, pos, &sck, &ip );

	if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	return returnables;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_bind( lua_State *L )
{
	return t_net_sck_bind( L, 1 );
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_connect( lua_State *L, const int pos )
{
	struct t_net_sck   *sck         = NULL;
	struct sockaddr_in *ip          = NULL;
	int                 returnables = t_net_getdef( L, pos, &sck, &ip );

	if (connect( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return returnables;
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L      Lua state.
 * \lparam  ud     t_net_sck userdata instance.
 * \lparam  ud     t_net_ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_connect( lua_State *L )
{
	return t_net_sck_connect( L, 1 );
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \param   int    position of server socket on stack.
 * \lparam  ud     T.Net.Socket userdata instance( server socket ).
 * \return  t_net* Client pointer.  Leaves cli_sock and cli_IP on stack.
 *-------------------------------------------------------------------------*/
int
t_net_sck_accept( lua_State *L, const int pos )
{
	struct t_net_sck   *srv    = t_net_sck_check_ud( L, pos+0, 1 ); // listening socket
	struct t_net_sck   *cli;                                        // accepted socket
	struct sockaddr_in *si_cli;                                     // peer address
	socklen_t           cli_sz = sizeof( struct sockaddr_in );
	size_t              one    = 1;

	cli     = t_net_sck_create_ud( L, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0 );
	si_cli  = t_net_ip4_create_ud( L );

	if ( (cli->fd  =  accept( srv->fd, (struct sockaddr *) &(*si_cli), &cli_sz )) == -1 )
		return t_push_error( L, "couldn't accept from socket" );

	if (-1 == setsockopt( cli->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
		return t_push_error( L, "couldn't make client socket reusable" );
	return 2;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket(TCP) userdata instance( server socket ).
 * \lreturn ud     T.Net.Socket(TCP) userdata instance( new client socket ).
 * \lreturn ud     T.Net.IpX userdata instance( new client sockaddr ).
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_accept( lua_State *L )
{
	return t_net_sck_accept( L, 1 );
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
	int  sent;

	if ((sent = sendto(
	  sck->fd,
	  buf, len, 0,
	  (struct sockaddr *) &(*addr), sizeof( struct sockaddr ))
	  ) == -1)
	{
		if (NULL == addr)
			return t_push_error( L, "Failed to send message");
		else
			return t_push_error( L, "Failed to send message to %s:%d",
					 inet_ntoa( addr->sin_addr ),
					 ntohs(     addr->sin_port ) );
	}

	return sent;
}


/** -------------------------------------------------------------------------
 * Send a message.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  ud     T.Net.Ip4 userdata instance (optional).
 * \lparam  string msg attempting to send.
 *       OR
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lparam  ofs    Offset in string or buffer.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_send( lua_State *L )
{
	struct t_net_sck   *sck      = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *ip       = t_net_ip4_check_ud( L, 2, 0 );
	size_t              len;      // Cap amount to send
	const char         *msg      = t_buf_checklstring( L, 3, &len, NULL );
	size_t              msg_ofs  = (lua_isinteger( L, 4 )) ? lua_tointeger( L, 4 ) : 0;
	int                 sent;

	sent = t_net_sck_send( L, sck, ip, msg+msg_ofs, len-msg_ofs );
	lua_pushinteger( L, sent );
	return 1;
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
	int                 rcvd;
	unsigned int        slen     = sizeof( addr );

	if ((rcvd = recvfrom( sck->fd, buf, len, 0, (struct sockaddr *) &(*addr), &slen )) == -1)
		return t_push_error( L, "Failed to recieve message" );
	return rcvd;
}


/** -------------------------------------------------------------------------
 * Recieve some data from a socket.
 * If the second parameter is a T.Buffer or a T.Buffer.Segement received data
 * will be written into them.  That automatically caps recieving data to the
 * length of that Buffer.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  ud     T.Buffer/Segment userdata instance.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_recv( lua_State *L )
{
	struct t_net_sck   *sck  = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *ip   = t_net_ip4_create_ud( L );
	int                 rcvd;
	size_t              len;
	char               *rcv  = t_buf_tolstring( L, 2, &len, NULL );

	if (NULL == rcv)
	{
		char buffer[ BUFSIZ ];
		rcv  = &(buffer[0]);
		len  = sizeof (buffer)-1;
	}

	rcvd = t_net_sck_recv( L, sck, ip, rcv, len );

	// return buffer, length, IpEndpoint
	if (0 == rcvd)
		lua_pushnil( L );
	else
		lua_pushlstring( L, rcv, rcvd );
	lua_pushinteger( L, rcvd );
	lua_pushvalue( L, -3 );
	return 3;
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
 * Recieve IpEndpoint from a (TCP) socket.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  ud     T.Net.Ip4 userdata instance.
 * \lreturn ud     T.Net.Ip4 userdata instance.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_getsockname( lua_State *L )
{
	struct t_net_sck   *sck         = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *ip          = t_net_ip4_check_ud( L, 2, 0 );

	if (NULL == ip)
		ip = t_net_ip4_create_ud( L );

	if (! t_net_sck_getsockname( sck, ip))
		lua_pushnil( L );
	return 1;  // return no matter what to allow testing for nil
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
static int
t_net_sck_mkFdset( lua_State *L, int pos, fd_set *set )
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
 * Systemcall select() for ready sockets.
 * \param   L      Lua state.
 * \lparam  table  T.Net socket array All sockets to read from.
 * \lparam  table  T.Net socket array All sockets to write to.
 * \lreturn table  T.Net.Socket table of sockets ready to read from.
 * \lreturn table  T.Net.Socket table of sockets ready to write to.
 * \return  int    # of values pushed onto the stack.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_net_sck_Select( lua_State *L )
{
	fd_set            rfds, wfds;
	struct t_net_sck *hndl;
	int               readySocks, i;
	int               rMax          = t_net_sck_mkFdset( L, 1, &rfds );
	int               wMax          = t_net_sck_mkFdset( L, 2, &wfds );

	readySocks = select(
		(wMax > rMax) ? wMax+1 : rMax+1,
		(-1  != rMax) ? &rfds  : NULL,
		(-1  != wMax) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create read  result table
	lua_createtable( L, 0, 0 );     // create write result table
	for (i=1; i<3; i++)
	{
		lua_pushnil( L );
		while (lua_next( L, i ))
		{
			hndl = t_net_sck_check_ud( L, -1, 1 ); //S: rdi wri rdr wrr key sck
			if FD_ISSET( hndl->fd, (1==i) ? &rfds : &wfds )
			{
				if (lua_isinteger( L, -2 ))         // append numeric idx
					lua_rawseti( L, i+2, lua_rawlen( L, i+2 )+1 );
				else
				{
					lua_pushvalue( L, -2 );          // reuse key for hash idx
					lua_insert( L, -2 );             //S: rdi wri rdr wrr key key sck
					lua_rawset( L, i+2 );
				}
				if (0 == --readySocks)
				{
					lua_pop( L, 1 );
					break;
				}
			}
			else
				lua_pop( L, 1 );
		}
	}
	return 2;
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
 * __index; used to get socket option values
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or fucntion name.
 * \lreturn value  int or bool socket option value or function.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__index( lua_State *L )
{
	struct t_net_sck *sck  = t_net_sck_check_ud( L, 1, 1 );

	lua_pushvalue( L, 2 );   // preserve the key
	t_getTypeByName( L, -1, NULL, t_net_optionList );

	if (lua_isnil( L, -1 ))
	{
		// in case no socket option was requested, relay functions from the
		// metatable if available (send,recv,accept,bind etc.)
		lua_pop( L, 1 );   // pop the nil
		lua_getmetatable( L, 1 );
		lua_pushvalue( L, 2 );
		lua_gettable( L, -2 );
		return 1;
	}
	else
	{
		return t_net_sck_getSocketOption(
			L,
			sck,
			luaL_checkinteger( L, -1 ),
			lua_tostring( L, 2 ) );
	}
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
					return t_push_error( L, "Couldn't set socket option" );
			}
			else
				return t_push_error( L, "Failed to set socket option" );
			break;

		case SO_RCVLOWAT:
		case SO_RCVTIMEO:
		case SO_SNDBUF:
		case SO_SNDLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVBUF:
			if (setsockopt( sck->fd, SOL_SOCKET, sckOpt, &val, sizeof( val ) ) < 0)
				return t_push_error( L, "Couldn't set socket option" );
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
				return t_push_error( L, "Couldn't set socket option" );
			break;

		case SO_ERROR:
#ifdef SO_PROTOCOL
		case SO_PROTOCOL:
#endif
		case SO_TYPE:
			return luaL_error( L, "can't set readonly socket option: %s", sckOptName );
		default:
			// should never get here
			return luaL_error( L, "unknown socket option: %s", sckOptName );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * __newindex; used to get socket option values
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Socket userdata instance.
 * \lparam  string socket option name or function name.
 * \lparam  value  int or bool socket option value.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_net_sck__newindex( lua_State *L )
{
	lua_pushvalue( L, 2 );
	t_getTypeByName( L, 4, NULL, t_net_optionList );
	if (lua_isnil( L, 4 ))
		return luaL_error( L, "unknown socket option: %s", lua_tostring( L, 2 ) );

	return t_net_sck_setSocketOption(
		L,
		t_net_sck_check_ud( L, 1, 1 ),
		lua_tointeger( L, 4 ),
		lua_tostring( L, 2 ),
		(lua_isboolean( L, 3 ))
			? lua_toboolean( L, 3 )
			: luaL_checkinteger( L, 3 )
	);
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_sck_fm [] = {
	  { "__call"      , lt_net_sck__Call }
	, { NULL          , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_cf [] =
{
	  { "select"      , lt_net_sck_Select }
	, { "bind"        , lt_net_sck_bind }
	, { "connect"     , lt_net_sck_connect }
	, { "listen"      , lt_net_sck_listen }
	, { NULL          , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_m [] =
{
	// metamethods
	  { "__tostring"  , lt_net_sck__tostring }
	, { "__index"     , lt_net_sck__index }
	, { "__newindex"  , lt_net_sck__newindex }
	, { "__gc"        , lt_net_sck_close }
	// object methods
	, { "listen"      , lt_net_sck_listen }
	, { "bind"        , lt_net_sck_bind }
	, { "connect"     , lt_net_sck_connect }
	, { "accept"      , lt_net_sck_accept }
	, { "close"       , lt_net_sck_close }
	, { "send"        , lt_net_sck_send }
	, { "recv"        , lt_net_sck_recv }
	, { "getsockname" , lt_net_sck_getsockname }
	// generic net functions -> reuse functions
	, { "getFd"       , lt_net_sck_getFd }
	//, { "setOption",   lt_net_setoption }
	, { NULL          , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     Lua state.
 * \lreturn table the library
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net_sck( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_NET_SCK_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_sck_m, 0 );
	lua_pop( L, 1 );

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_sck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
