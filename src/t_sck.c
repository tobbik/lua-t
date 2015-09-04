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
/** -------------------------------------------------------------------------
 * check the set parameters of a particular Socket/Filedescriptor
 * \param   int  The File/Socket descriptor ident.
 * \return  int  Boolean - 1 if found; 0 if not.
 *-------------------------------------------------------------------------*/
int t_sck_getfdinfo( int fd )
{
	char    buf[ 256 ];
	char    path[ 256 ];
	int     fd_flags;
	int     fl_flags;
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
		printf( "\n" );
		return 1;
	}
	return 0;
}


int lt_sck_getfdinfo( lua_State *L )
{
	struct t_sck      *sck = t_sck_check_ud( L, 1, 1 );
	t_sck_getfdinfo( sck->fd );
	return 0;
}


int lt_sck_getfdsinfo( lua_State *L )
{
	UNUSED( L );
	int numFd  = getdtablesize();
	int fd;
	for (fd=0; fd<numFd; fd++)
		t_sck_getfdinfo( fd );
	return 0;
}
#endif


/** -------------------------------------------------------------------------
 * \brief   create a socket and return it.
 * \param   L  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_sck_New( lua_State *L )
{
	struct t_sck   __attribute__ ((unused)) *sck;

	sck = t_sck_create_ud( L,
	   (enum t_sck_t) luaL_checkoption( L, 1, "TCP", t_sck_t_lst ),
	   1 );
	return 1 ;
}


/**--------------------------------------------------------------------------
 * construct a Socket and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table Socket
 * \lparam  string    type "TCP", "UDP", ...
 * \lreturn struct t_sck userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_sck__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_sck_New( L );
}


/**--------------------------------------------------------------------------
 * \brief   create a socket and push to LuaStack.
 * \param   L         The lua state.
 * \param   enum t_sck_t  Type of socket.
 * \param   bool/int      Should socket be created.
 * \return  struct t_sck* pointer to the socket struct.
 * --------------------------------------------------------------------------*/
struct t_sck
*t_sck_create_ud( lua_State *L, enum t_sck_t type, int create )
{
	struct t_sck  *sck = (struct t_sck*) lua_newuserdata( L, sizeof( struct t_sck ) );
	size_t         one = 1;

	if (create)
	{
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
	}

	sck->t = type;

	luaL_getmetatable( L, "T.Socket" );
	lua_setmetatable( L, -2 );
	return sck;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_sck.
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_sck*  pointer to the struct t_sck.
 * --------------------------------------------------------------------------*/
struct t_sck
*t_sck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Socket" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Socket` expected" );
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
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_sck*  pointer to the struct t_sck.
 * --------------------------------------------------------------------------*/
static void inline
t_sck_getdef( lua_State *L, int pos, struct t_sck **sck, struct sockaddr_in **ip )
{
	enum t_sck_t type;

	*sck = t_sck_check_ud( L, pos+0, 0 );
	*ip  = t_ipx_check_ud( L, pos+1, 0 );

	if (NULL == *sck)     // handle T.Socket.whatever(), default to TCP
	{
		type = (enum t_sck_t) luaL_checkoption( L, pos+0, NULL, t_sck_t_lst );
		*sck = t_sck_create_ud( L, type, 1 );
		lua_replace( L, pos+0 );
	}

	if (NULL == *ip)
	{
		*ip = t_ipx_create_ud( L );
		t_ipx_set( L, pos+1, *ip );
		lua_insert( L, pos+1 );
	}
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_sck_listen( lua_State *L, int pos )
{
	struct t_sck       *sck = t_sck_check_ud( L, pos+0, 0 );
	struct sockaddr_in *ip  = t_ipx_check_ud( L, pos+1, 0 );
	int                 backlog;

	if (NULL == sck)
	{
		sck = t_sck_create_ud( L, T_SCK_TCP, 1 );
		lua_insert( L, pos+0 );

		t_sck_getdef( L, pos+0, &sck, &ip );
		//S: t_sck,t_ip
		if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
			return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );
	}
	backlog = luaL_checkinteger( L, pos+2 );
	lua_remove( L, pos+2 );

	//luaL_argcheck( L, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if (listen( sck->fd , backlog ) == -1)
		return t_push_error( L, "ERROR listen to socket" );

	return 2;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_sck_listen( lua_State *L )
{
	return t_sck_listen( L, 1 );
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_sck_bind( lua_State *L )
{
	struct t_sck       *sck = NULL;
	struct sockaddr_in *ip  = NULL;

	t_sck_getdef( L, 1, &sck, &ip );

	if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	return 2;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Connect a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_sck_connect( lua_State *L )
{
	struct t_sck       *sck = NULL;
	struct sockaddr_in *ip  = NULL;

	t_sck_getdef( L, 1, &sck, &ip );

	if (connect( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return 2; //socket,ip
}


/** -------------------------------------------------------------------------
 * Accept a (TCP) socket connection.
 * \param   L   The lua state.
 * \param   t_sck   Server.
 * \param   sockaddr_in*  Client Address pointer.
 * \return  t_sck*  Client pointer.  Leaves cli_sock and cli_IP on stack.
 *-------------------------------------------------------------------------*/
int
t_sck_accept( lua_State *L, int pos )
{
	struct t_sck       *srv    = t_sck_check_ud( L, pos+0, 1 ); // listening socket
	struct t_sck       *cli;                                        // accepted socket
	struct sockaddr_in *si_cli;                                     // peer address
	socklen_t           cli_sz = sizeof( struct sockaddr_in );
	size_t              one    = 1;

	luaL_argcheck( L, (T_SCK_TCP == srv->t), pos+0, "Must be an TCP socket" );
	cli     = t_sck_create_ud( L, T_SCK_TCP, 0 );
	si_cli  = t_ipx_create_ud( L );

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
int
lt_sck_accept( lua_State *L )
{
	return t_sck_accept( L, 1 );
}


/** -------------------------------------------------------------------------
 * Set a socket option.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \return  int    # of values pushed onto the stack.
 * TODO: Actually do option settings with multiple options etc here...
 *-------------------------------------------------------------------------*/
int
t_sck_reuseaddr( lua_State *L, struct t_sck *sck )
{
	size_t one           = 1;
	int flag             = 0;

	if (-1 != sck->fd)
	{
		if (-1 == setsockopt( sck->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
			return t_push_error( L, "ERROR setting option" );
		flag = fcntl( sck->fd, F_GETFL, 0 );           /* Get socket flags */
		fcntl( sck->fd, F_SETFL, flag | O_NONBLOCK );     /* Add non-blocking flag */
	}

	return 0;
}


static int
lt_sck_setoption( lua_State *L )
{
	struct t_sck *sck = t_sck_check_ud( L, 1, 1 );
	return t_sck_reuseaddr( L, sck );
}



/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_sck_close( lua_State *L, struct t_sck *sck )
{
	if (-1 != sck->fd)
	{
		//printf( "closing socket: %d\n", sck->fd );
		if (-1 == close( sck->fd ))
			return t_push_error( L, "ERROR closing socket" );
		else
		{
			sck->fd = -1;         // invalidate socket
		}
	}

	return 0;
}


static int
lt_sck_close( lua_State *L )
{
	struct t_sck *sck = t_sck_check_ud( L, 1, 1 );
	return t_sck_close( L, sck );
}


/** -------------------------------------------------------------------------
 * \brief   send Datagram over a UDP socket to an IP endpoint.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_sck_sendto( lua_State *L )
{
	struct t_sck       *sck;
	struct sockaddr_in *ip;
	struct t_buf       *buf;
	int                 sent;
	int                 len;
	const char         *msg;

	sck = t_sck_check_ud( L, 1, 1 );
	luaL_argcheck( L, (T_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	ip   = t_ipx_check_ud( L, 2, 1 );
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
	  sck->fd,
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
lt_sck_recvfrom( lua_State *L )
{
	struct t_sck       *sck;
	struct t_buf       *buf;
	struct sockaddr_in *si_cli;
	int                 rcvd;
	char                buffer[ BUFSIZ ];
	char               *rcv = &(buffer[ 0 ]);
	int                 len = sizeof( buffer )-1;

	unsigned int        slen = sizeof( si_cli );

	sck = t_sck_check_ud( L, 1, 1 );
	luaL_argcheck( L, (T_SCK_UDP == sck->t), 1, "Must be an UDP socket" );
	if (lua_isuserdata( L, 2 )) {
		buf  = t_buf_check_ud ( L, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}
	si_cli = t_ipx_create_ud( L );

	if ((rcvd = recvfrom(
	  sck->fd,
	  rcv, len, 0,
	  (struct sockaddr *) &(*si_cli), &slen )
	  ) == -1)
		return t_push_error( L, "Failed to recieve UDP packet");

	// return buffer, length, IpEndpoint
	lua_pushlstring( L, rcv, rcvd );
	lua_pushinteger( L, rcvd );
	lua_pushvalue( L, -3 );
	return 3;
}


/** -------------------------------------------------------------------------
 * \brief   send some data to a TCP socket.
 * \param   L  The lua state.
 * \param   t_sck  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_sck_send( lua_State *L, struct t_sck *sck, const char* buff, size_t sz )
{
	int     rslt;

	if ((rslt = send(
	  sck->fd,
	  buff, sz, 0)
	  ) == -1)
		return t_push_error( L, "Failed to send TCP message" ) ;

	return rslt;
}


/** -------------------------------------------------------------------------
 * \brief   send a message over a TCP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_sck_send( lua_State *L )
{
	struct t_sck *sck;
	size_t        to_send;      // How much should get send out maximally
	const char   *msg;
	size_t        into_msg=0;   // where in the message to start sending from

	sck = t_sck_check_ud( L, 1, 1 );
	luaL_argcheck( L, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isstring( L, 2 ))
		msg   = lua_tolstring( L, 2, &to_send );
	else
		return t_push_error( L, "ERROR send(socket,msg) takes msg argument" ) ;
	if (lua_isnumber( L, 3 )) {
		into_msg = lua_tointeger( L, 3 );
	}
	msg      = msg + into_msg;
	to_send -= into_msg;

	lua_pushinteger( L, t_sck_send( L, sck, msg, to_send ) );

	return 1;
}


/** -------------------------------------------------------------------------
 * \brief   recieve some data from a TCP socket.
 * \param   L  The lua state.
 * \param   t_sck  userdata.
 * \param   buff   char buffer.
 * \param   sz     size of char buffer.
 * \return  number of bytes received.
 *-------------------------------------------------------------------------*/
int
t_sck_recv( lua_State *L, struct t_sck *sck, char* buff, size_t sz )
{
	int     rslt;

	if ((rslt = recv(
	  sck->fd,
	  buff, sz, 0)
	  ) == -1)
		t_push_error( L, "Failed to recieve TCP packet" );

	return rslt;
}


/** -------------------------------------------------------------------------
 * \brief   recieve some data from a TCP socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn string The recieved message.
 * \lreturn rcvd   number of bytes recieved.
 * \return  int    # of values pushed onto the stack.
 * TODO:  If no Buffer is passed for receiving, use a LuaL_Buffer instead
 *-------------------------------------------------------------------------*/
static int
lt_sck_recv( lua_State *L )
{
	struct t_sck *sck;
	struct t_buf *buf;
	int           rcvd;
	char          buffer[ BUFSIZ ];
	char         *rcv = &(buffer[0]);
	int           len = sizeof (buffer)-1;

	sck = t_sck_check_ud( L, 1, 1 );
	luaL_argcheck( L, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );
	if (lua_isuserdata( L, 2 )) {
		buf  = t_buf_check_ud( L, 2, 1 );
		rcv  = (char *) &(buf->b[ 0 ]);
		len  = buf->len;
	}

	rcvd = t_sck_recv( L, sck, rcv, len );

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
 * TODO:  Allow it to accept an existing LuaBuffer to ammend incoming packages
 *-------------------------------------------------------------------------*/
static int
lt_sck_getsockname( lua_State *L )
{
	struct t_sck       *sck;
	struct sockaddr_in *ip;
	socklen_t           ip_len = sizeof( struct sockaddr_in );

	sck = t_sck_check_ud( L, 1, 1 );
	luaL_argcheck( L, (T_SCK_TCP == sck->t), 1, "Must be an TCP socket" );

	if (lua_isuserdata( L, 2 ))  ip = t_ipx_check_ud( L, 2, 1 );
	else                         ip = t_ipx_create_ud( L );

	getsockname( sck->fd, (struct sockaddr*) &(*ip), &ip_len );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   prints out the socket.
 * \param   L    The lua state.
 * \lparam  struct t_sck the socket userdata.
 * \lreturn string   formatted string representing sockkaddr (IP:Port).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_sck__tostring( lua_State *L )
{
	struct t_sck *sck = t_sck_check_ud( L, 1, 1 );
	lua_pushfstring( L, "T.Socket{%s:%d}: %p",
			(T_SCK_TCP == sck->t) ? "TCP" : "UDP",
			sck->fd,
			sck );
	return 1;
}


/** -------------------------------------------------------------------------
 * Return the FD int representation of the socket
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_sck_getfdid( lua_State *L )
{
	struct t_sck      *sck = t_sck_check_ud( L, 1, 1 );
	lua_pushinteger( L, sck->fd );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_sck_fm [] = {
	{ "__call",    lt_sck__Call },
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_sck_cf [] =
{
	{ "new",       lt_sck_New },
#ifndef _WIN32
	{ "showAllFd", lt_sck_getfdsinfo },
#endif
	{ "bind",      lt_sck_bind },
	{ "connect",   lt_sck_connect },
	{ "listen",    lt_sck_listen },
	{ NULL,        NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_sck_m [] =
{
	{ "listen",    lt_sck_listen },
	{ "bind",      lt_sck_bind },
	{ "connect",   lt_sck_connect },
	{ "accept",    lt_sck_accept },
	{ "close",     lt_sck_close },
	{ "sendTo",    lt_sck_sendto },
	{ "recvFrom",  lt_sck_recvfrom },
	{ "send",      lt_sck_send },
	{ "recv",      lt_sck_recv },
	{ "getId",     lt_sck_getfdid },
	{ "getFdInfo", lt_sck_getfdinfo },
	{ "getIp",     lt_sck_getsockname },
	{ "setOption", lt_sck_setoption },
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
luaopen_t_sck( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, "T.Socket" );   // stack: functions meta
	luaL_newlib( L, t_sck_m);
	lua_setfield( L, -2, "__index" );
	lua_pushcfunction( L, lt_sck__tostring );
	lua_setfield( L, -2, "__tostring");
	lua_pushcfunction( L, lt_sck_close );
	lua_setfield( L, -2, "__gc");
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_sck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
