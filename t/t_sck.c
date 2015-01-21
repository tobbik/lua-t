/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_sck.c
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
#include "t_sck.h"
#include "t_buf.h"         // the ability to send and recv buffers


#ifndef _WIN32
int t_sck_getfdinfo( int fd )
{
	char buf[256];
	char path[256];
	int fd_flags;
	int fl_flags;
	ssize_t s;

	sprintf( path, "/proc/self/fd/%d", fd );
	s = readlink( path, &buf[0], 256 );
	if ( s == -1 )
	{
		//printf( " (%s): not available", path);
		return 0;
	}
	else
	{
		fd_flags = fcntl( fd, F_GETFD );
		if ( fd_flags == -1 )
			return 0;

		fl_flags = fcntl( fd, F_GETFL );
		if ( fl_flags == -1 )
			return 0;

		printf( " (%s): ", path);
		memset( &buf[0], 0, 256 );

		if ( fd_flags & FD_CLOEXEC )  printf( "cloexec " );

		// file status
		if ( fl_flags & O_APPEND   )  printf( "append " );
		if ( fl_flags & O_NONBLOCK )  printf( "nonblock " );

		// acc mode
		if ( fl_flags & O_RDONLY   )  printf( "read-only " );
		if ( fl_flags & O_RDWR     )  printf( "read-write " );
		if ( fl_flags & O_WRONLY   )  printf( "write-only " );

		if ( fl_flags & O_DSYNC    )  printf( "dsync " );
		if ( fl_flags & O_RSYNC    )  printf( "rsync " );
		if ( fl_flags & O_SYNC     )  printf( "sync " );

		struct flock fl;
		fl.l_type   = F_WRLCK;
		fl.l_whence = 0;
		fl.l_start  = 0;
		fl.l_len    = 0;
		fcntl( fd, F_GETLK, &fl );
		if ( fl.l_type != F_UNLCK )
		{
			if ( fl.l_type == F_WRLCK )
				printf( "write-locked" );
			else
				printf( "read-locked" );
			printf( "(pid: %d)", fl.l_pid );
		}
		printf("\n");
		return 1;
	}
	return 0;
}


int lt_sck_getfdinfo( lua_State *luaVM )
{
	struct t_sck      *sck = t_sck_check_ud( luaVM, 1, 1 );
	t_sck_getfdinfo( sck->fd );
	return 0;
}


int lt_sck_getfdsinfo( lua_State *luaVM )
{
	UNUSED( luaVM );
	int numFd  = getdtablesize();
	int fd;
	for (fd=0; fd<numFd; fd++)
		t_sck_getfdinfo( fd );
	return 0;
}
#endif


/** -------------------------------------------------------------------------
 * \brief   create a socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int
lt_sck_New( lua_State *luaVM )
{
	struct t_sck   __attribute__ ((unused)) *sck;

	sck = t_sck_create_ud( luaVM,
			(enum t_sck_t) luaL_checkoption (luaVM, 1, "TCP", t_sck_t_lst) );
	return 1 ;
}


/**--------------------------------------------------------------------------
 * construct a Socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table Socket
 * \lparam  string    type "TCP", "UDP", ...
 * \lreturn struct t_sck userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_sck__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_sck_New( luaVM );
}


/**--------------------------------------------------------------------------
 * \brief   create a socket and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct t_sck*  pointer to the socket struct
 * --------------------------------------------------------------------------*/
struct t_sck
*t_sck_create_ud( lua_State *luaVM, enum t_sck_t type )
{
	struct t_sck  *sck = (struct t_sck*) lua_newuserdata( luaVM, sizeof( struct t_sck ) );
	size_t         one = 1;

	switch (type)
	{
		case T_SCK_UDP:
			if ( (sck->fd  =  socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1 )
				return NULL;
			break;

		case T_SCK_TCP:
			if ( (sck->fd  =  socket( AF_INET, SOCK_STREAM, 0 )) == -1 )
				return NULL;
			if (-1 == setsockopt( sck->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
				return NULL;
			break;
		default:
			return NULL;
	}

	sck->t = type;

	luaL_getmetatable( luaVM, "T.Socket" );
	lua_setmetatable( luaVM, -2 );
	return sck;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_sck.
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_sck*  pointer to the struct t_sck.
 * --------------------------------------------------------------------------*/
struct t_sck
*t_sck_check_ud( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_testudata( luaVM, pos, "T.Socket" );
	luaL_argcheck( luaVM, (ud != NULL || !check), pos, "`T.Socket` expected" );
	return (NULL==ud) ? NULL : (struct t_sck *) ud;
}


/**--------------------------------------------------------------------------
 * Evaluate elements on stack to be definitions or instances of sock and ip.
 * possible combinations:
 *  - ...(sck,ip)
 *  - ...("TCP/UDP", ip)
 *  - ...(sck,'ipString', port)
 *  - ...(sck, port)           -- IP will be 0.0.0.0
 *  - ...(sck, 'ipstring')     -- Port unassigned
 *  - ...(sck)                 -- Port unassigned, IP 0.0.0.0
 *
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_sck*  pointer to the struct t_sck.
 * --------------------------------------------------------------------------*/
static void inline
t_sck_getdef( lua_State *luaVM, int pos, struct t_sck **sck, struct sockaddr_in **ip )
{
	enum t_sck_t type;

	*sck = t_sck_check_ud( luaVM, pos+0, 0 );
	*ip  = t_ipx_check_ud( luaVM, pos+1, 0 );

	if (NULL == *sck)     // handle T.Socket.whatever(), default to TCP
	{
		type = (enum t_sck_t) luaL_checkoption( luaVM, pos+0, NULL, t_sck_t_lst );
		*sck = t_sck_create_ud( luaVM, type );
		lua_replace( luaVM, pos+0 );
	}

	if (NULL == *ip)
	{
		*ip = t_ipx_create_ud( luaVM );
		t_ipx_set( luaVM, pos+1, *ip );
		lua_insert( luaVM, pos+1 );
	}
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int
t_sck_listen( lua_State *luaVM, int pos )
{
	struct t_sck       *sck = t_sck_check_ud( luaVM, pos+0, 0 );
	struct sockaddr_in *ip  = t_ipx_check_ud( luaVM, pos+1, 0 );
	int                 backlog;

	if (NULL == sck)
	{
		sck = t_sck_create_ud( luaVM, T_SCK_TCP );
		lua_insert( luaVM, pos+0 );

		t_sck_getdef( luaVM, pos+0, &sck, &ip );
		//S: t_sck,t_ip
		if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
			return t_push_error( luaVM, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );
	}
	backlog = luaL_checkinteger( luaVM, pos+2 );
	lua_remove( luaVM, pos+2 );

	//luaL_argcheck( luaVM, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if (listen( sck->fd , backlog ) == -1)
		return t_push_error( luaVM, "ERROR listen to socket" );

	return 2;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int
lt_sck_listen( lua_State *luaVM )
{
	return t_sck_listen( luaVM, 1 );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int
lt_sck_bind( lua_State *luaVM )
{
	struct t_sck       *sck = NULL;
	struct sockaddr_in *ip  = NULL;

	t_sck_getdef( luaVM, 1, &sck, &ip );

	if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( luaVM, "ERROR binding socket to %s:%d",
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
int
lt_sck_connect( lua_State *luaVM )
{
	struct t_sck       *sck = NULL;
	struct sockaddr_in *ip  = NULL;

	t_sck_getdef( luaVM, 1, &sck, &ip );

	if (connect( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( luaVM, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return 2; //socket,ip
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   luaVM   The lua state.
 * \param   t_sck   Server.
 * \param   sockaddr_in*  Client Address pointer.
 * \return  t_sck*  Client pointer.  Leaves cli_sock and cli_IP on stack.
 *-------------------------------------------------------------------------*/
int
t_sck_accept( lua_State *luaVM, int pos )
{
	struct t_sck       *srv    = t_sck_check_ud( luaVM, pos+0, 1 ); // listening socket
	struct t_sck       *cli;                                        // accepted socket
	struct sockaddr_in *si_cli;                                     // peer address
	socklen_t           cli_sz = sizeof( struct sockaddr_in );
	size_t              one    = 1;

	luaL_argcheck( luaVM, (T_SCK_TCP == srv->t), pos+0, "Must be an TCP socket" );
	cli    = t_sck_create_ud( luaVM, T_SCK_TCP );
	si_cli = t_ipx_create_ud( luaVM );

	if ( (cli->fd  =  accept( srv->fd, (struct sockaddr *) &(*si_cli), &cli_sz )) == -1 )
		return t_push_error( luaVM, "couldn't accept from socket" );
	if (-1 == setsockopt( cli->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
		return t_push_error( luaVM, "couldn't make client socket reusable" );
	return 2;
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   luaVM   The lua state.
 * \lparam  socket  socket userdata.
 * \lreturn socket  socket userdata for new connection.
 * \lreturn ip      sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int
lt_sck_accept( lua_State *luaVM )
{
	return t_sck_accept( luaVM, 1 );
}


/** -------------------------------------------------------------------------
 * Set a socket option.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO: Actually do option settings with multiple options etc here...
 *-------------------------------------------------------------------------*/
int
t_sck_reuseaddr( lua_State *luaVM, struct t_sck *sck )
{
	size_t one           = 1;
	int flag             = 0;

	if (-1 != sck->fd)
	{
		if (-1 == setsockopt( sck->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
			return t_push_error( luaVM, "ERROR setting option" );
		flag = fcntl( sck->fd, F_GETFL, 0 );           /* Get socket flags */
		fcntl( sck->fd, F_SETFL, flag | O_NONBLOCK );     /* Add non-blocking flag */
	}

	return 0;
}


static int
lt_sck_setoption( lua_State *luaVM )
{
	struct t_sck *sck = t_sck_check_ud( luaVM, 1, 1 );
	return t_sck_reuseaddr( luaVM, sck );
}



/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
int
t_sck_close( lua_State *luaVM, struct t_sck *sck )
{
	if (-1 != sck->fd)
	{
		printf( "closing socket: %d\n", sck->fd );
		t_sck_getfdinfo( sck->fd );
		if (-1 == close( sck->fd ))
			return t_push_error( luaVM, "ERROR closing socket" );
		else
		{
			t_sck_getfdinfo( sck->fd );
			sck->fd = -1;         // invalidate socket
			printf("NOW -1\n");
		}
	}

	return 0;
}


static int
lt_sck_close( lua_State *luaVM )
{
	struct t_sck *sck = t_sck_check_ud( luaVM, 1, 1 );
	return t_sck_close( luaVM, sck );
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
static int
lt_sck_sendto( lua_State *luaVM )
{
	struct t_sck       *sck;
	struct sockaddr_in *ip;
	struct t_buf       *buf;
	int                 sent;
	int                 len;
	const char         *msg;

	sck = t_sck_check_ud( luaVM, 1, 1 );
	luaL_argcheck( luaVM, (T_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	ip   = t_ipx_check_ud( luaVM, 2, 1 );
	if (lua_isstring( luaVM, 3 ))
	{
		msg   = lua_tostring( luaVM, 3 );
		len   = strlen( msg );
	}
	else if (lua_isuserdata( luaVM, 3 ))
	{
		buf  = t_buf_check_ud( luaVM, 3, 1 );
		msg  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	else
		return t_push_error( luaVM, "ERROR sendTo(socket,ip,msg) takes msg argument" );

	if ((sent = sendto(
	  sck->fd,
	  msg, len, 0,
	  (struct sockaddr *) &(*ip), sizeof( struct sockaddr ))
	  ) == -1)
		return t_push_error( luaVM, "Failed to send UDP packet to %s:%d",
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
static int
lt_sck_recvfrom( lua_State *luaVM )
{
	struct t_sck       *sck;
	struct t_buf       *buf;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[ BUFSIZ ];
	char               *rcv = &(buffer[ 0 ]);
	int                 len = sizeof( buffer )-1;

	unsigned int        slen = sizeof( si_cli );

	sck = t_sck_check_ud( luaVM, 1, 1 );
	luaL_argcheck( luaVM, (T_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	if (lua_isuserdata( luaVM, 2 )) {
		buf  = t_buf_check_ud ( luaVM, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	si_cli = t_ipx_create_ud( luaVM );

	if ((rcvd = recvfrom(
	  sck->fd,
	  rcv, len, 0,
	  (struct sockaddr *) &(*si_cli), &slen )
	  ) == -1)
		return t_push_error( luaVM, "Failed to recieve UDP packet");

	// return buffer, length, IpEndpoint
	lua_pushlstring( luaVM, rcv, rcvd );
	lua_pushinteger( luaVM, rcvd );
	lua_pushvalue( luaVM, -3 );
	return 3;
}


/** -------------------------------------------------------------------------
 * \brief   send some data to a TCP socket.
 * \param   luaVM  The lua state.
 * \param   t_sck  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_sck_send( lua_State *luaVM, struct t_sck *sck, const char* buff, size_t sz )
{
	int     rslt;

	if ((rslt = send(
	  sck->fd,
	  buff, sz, 0)
	  ) == -1)
		return t_push_error( luaVM, "Failed to send TCP message" ) ;

	return rslt;
}


/** -------------------------------------------------------------------------
 * \brief   send a message over a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int
lt_sck_send( lua_State *luaVM )
{
	struct t_sck *sck;
	size_t        to_send;      // How much should get send out maximally
	const char   *msg;
	size_t        into_msg=0;   // where in the message to start sending from

	sck = t_sck_check_ud( luaVM, 1, 1 );
	luaL_argcheck( luaVM, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isstring( luaVM, 2 ))
		msg   = lua_tolstring( luaVM, 2, &to_send );
	else
		return t_push_error( luaVM, "ERROR send(socket,msg) takes msg argument" ) ;
	if (lua_isnumber( luaVM, 3 )) {
		into_msg = lua_tointeger( luaVM, 3 );
	}
	msg      = msg + into_msg;
	to_send -= into_msg;

	lua_pushinteger( luaVM, t_sck_send( luaVM, sck, msg, to_send ) );

	return 1;
}


/** -------------------------------------------------------------------------
 * \brief   recieve some data from a TCP socket.
 * \param   luaVM  The lua state.
 * \param   t_sck  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_sck_recv( lua_State *luaVM, struct t_sck *sck, char* buff, size_t sz )
{
	int     rslt;

	if ((rslt = recv(
	  sck->fd,
	  buff, sz, 0)
	  ) == -1)
		t_push_error( luaVM, "Failed to recieve TCP packet" );

	return rslt;
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
static int
lt_sck_recv( lua_State *luaVM )
{
	struct t_sck *sck;
	struct t_buf *buf;
	int           rcvd;
	char          buffer[ BUFSIZ ];
	char         *rcv = &(buffer[0]);
	int           len = sizeof (buffer)-1;

	sck = t_sck_check_ud( luaVM, 1, 1 );
	luaL_argcheck( luaVM, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isuserdata( luaVM, 2 )) {
		buf  = t_buf_check_ud( luaVM, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}

	rcvd = t_sck_recv( luaVM, sck, rcv, len );

	// return buffer, length
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
static int
lt_sck_getsockname( lua_State *luaVM )
{
	struct t_sck       *sck;
	struct sockaddr_in *ip;
	socklen_t           ip_len = sizeof( struct sockaddr_in );

	sck = t_sck_check_ud( luaVM, 1, 1 );
	luaL_argcheck( luaVM, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if (lua_isuserdata( luaVM, 2 ))  ip = t_ipx_check_ud( luaVM, 2, 1 );
	else                             ip = t_ipx_create_ud( luaVM );

	getsockname( sck->fd, (struct sockaddr*) &(*ip), &ip_len );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the socket.
 * \param   luaVM    The lua state.
 * \lparam  struct t_sck the socket userdata.
 * \lreturn string   formatted string representing sockkaddr (IP:Port).
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_sck__tostring( lua_State *luaVM )
{
	struct t_sck *sck = t_sck_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM, "T.Socket{%s:%d}: %p",
			(T_SCK_TCP == sck->t) ? "TCP" : "UDP",
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
static int
lt_sck_getfdid( lua_State *luaVM )
{
	struct t_sck      *sck = t_sck_check_ud( luaVM, 1, 1 );
	lua_pushinteger( luaVM, sck->fd );
	return 1;
}



/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg t_sck_fm [] = {
	{"__call",      lt_sck__Call},
	{NULL,   NULL}
};


/**
 * \brief      the socketlibrary definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_sck_m [] =
{
	{"listen",    lt_sck_listen},
	{"bind",      lt_sck_bind},
	{"connect",   lt_sck_connect},
	{"accept",    lt_sck_accept},
	{"close",     lt_sck_close},
	{"sendTo",    lt_sck_sendto},
	{"recvFrom",  lt_sck_recvfrom},
	{"send",      lt_sck_send},
	{"recv",      lt_sck_recv},
	{"getId",     lt_sck_getfdid},
	{"getFdInfo", lt_sck_getfdinfo},
	{"getIp",     lt_sck_getsockname},
	{"setOption", lt_sck_setoption},
	{NULL,        NULL}
};


/**
 * \brief      the socket library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_sck_cf [] =
{
	{"new",       lt_sck_New},
#ifndef _WIN32
	{"showAllFd", lt_sck_getfdsinfo},
#endif
	{"bind",      lt_sck_bind},
	{"connect",   lt_sck_connect},
	{"listen",    lt_sck_listen},
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
LUA_API int
luaopen_t_sck( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "T.Socket" );   // stack: functions meta
	luaL_newlib( luaVM, t_sck_m);
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_sck__tostring );
	lua_setfield( luaVM, -2, "__tostring");
	lua_pushcfunction( luaVM, lt_sck_close );
	lua_setfield( luaVM, -2, "__gc");
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( luaVM, t_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( luaVM, t_sck_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}
