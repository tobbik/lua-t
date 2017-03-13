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
 * Create a socket and push to LuaStack.
 * \param   L        Lua state.
 * \lparam  domain   string:'ip4', 'ip6', 'raw'.
 * \lparam  protocol string:'TCP', 'UDP', ...
 * \return  struct t_net_sck pointer to the socket struct.
 * --------------------------------------------------------------------------*/
static int
lt_net_sck__Call( lua_State *L )
{
	lua_remove( L, 1 );         // remove CLASS table
	int               domain   = t_net_domainType[
	                                luaL_checkoption( L, 1, "ip4", t_net_domainName )
	                             ];
	int               protocol = t_net_getProtocol( L, 2 );
	int               type;
	struct t_net_sck *sck;

	switch (protocol)
	{
		case IPPROTO_TCP:    type = SOCK_STREAM;    break;
		case IPPROTO_UDP:    type = SOCK_DGRAM;     break;
		default:             type = SOCK_RAW;       break;
	}

	if (AF_UNIX == domain)
		protocol = 0;

	sck = t_net_sck_create_ud( L, domain, type, protocol, 1 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L      Lua state.
 * \param   enum   t_net_t  Type of socket.
 * \param   bool   1 if Should socket be created, else 0.
 * \return  struct t_net_sck* pointer to the socket struct.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_create_ud( lua_State *L, int domain, int type, int protocol, int create )
{
	struct t_net_sck *sck = (struct t_net_sck *) lua_newuserdata( L, sizeof( struct t_net_sck ) );

	if (create)
		sck->fd = socket( domain, type, protocol );
		if (sck->fd == -1)
			t_push_error( L, "Couldn't create socket" );
	sck->domain   = domain;
	sck->protocol = protocol;
	sck->domain   = type;
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

	luaL_getmetafield( L, -1, "__name" );
	lua_pushfstring( L, "%s{%d}: %p"
		, lua_tostring( L , -1 )
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
	struct t_net_sck   *sck = t_net_sck_check_ud( L, pos+0, 0 );
	struct sockaddr_in *ip  = t_net_ip4_check_ud( L, pos+1, 0 );
	int                 bl  = luaL_checkinteger( L, -1 ); // backlog

	lua_pop( L, 1 );   // remove address

	if (NULL == sck)
	{
		t_net_getdef( L, pos+0, &sck, &ip );
		//S: t_net,t_net_ip4
		if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
			return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );
	}

	if (listen( sck->fd , bl ) == -1)
		return t_push_error( L, "ERROR listen to socket" );

	return 2;  // socket, ip
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
	struct t_net_sck   *sck = t_net_sck_check_ud( L, pos+0, 0 );
	struct sockaddr_in *ip  = t_net_ip4_check_ud( L, pos+1, 0 );

	if (NULL==sck || NULL == ip)
		t_net_getdef( L, 1, &sck, &ip );

	if (bind( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );

	return 2;  // socket, ip
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
	struct t_net_sck   *sck = t_net_sck_check_ud( L, pos+0, 0 );
	struct sockaddr_in *ip  = t_net_ip4_check_ud( L, pos+1, 0 );

	if (NULL==sck || NULL == ip)
		t_net_getdef( L, 1, &sck, &ip );

	if (connect( sck->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return 2; //socket,ip
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
 * \lparam  ud     T.Net.Tcp userdata instance( server socket ).
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
		return t_push_error( L, "Failed to send message to %s:%d",
					 inet_ntoa( addr->sin_addr ),
					 ntohs(     addr->sin_port ) );

	return sent;
}


/** -------------------------------------------------------------------------
 * Send a message.
 * \param   L      Lua state.
 * \lparam  ud     T.Net.Sck userdata instance.
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
	struct t_net_sck   *sck    = t_net_sck_check_ud( L, 1, 1 );
	struct sockaddr_in *si_cli = t_net_ip4_create_ud( L );
	int                 rcvd;
	size_t              len;
	char               *rcv    = t_buf_tolstring( L, 2, &len, NULL );

	if (NULL == rcv)
	{
		char buffer[ BUFSIZ ];
		rcv  = &(buffer[0]);
		len  = sizeof (buffer)-1;
	}

	rcvd = t_net_sck_recv( L, sck, si_cli, rcv, len );

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
	struct t_net_sck   *sck = t_net_sck_check_ud( L, 1, 1 );;
	struct sockaddr_in *ip;
	socklen_t           ip_len = sizeof( struct sockaddr_in );

	if (lua_isuserdata( L, 2 ))  ip = t_net_ip4_check_ud( L, 2, 1 );
	else                         ip = t_net_ip4_create_ud( L );

	getsockname( sck->fd, (struct sockaddr*) &(*ip), &ip_len );
	return 1;
}


/** -------------------------------------------------------------------------
 * Helper to take sockets from Lua tables to FD_SET.
 * Itertates over the table puls out the socket structs and adds the actual
 * sockets to the fd_set.
 * \param   L      Lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
static void
t_net_sck_mkFdset( lua_State *L, int pos, fd_set *set, int *maxFd )
{
	struct t_net_sck  *sck;

	FD_ZERO( set );
	// empty table == nil
	if (lua_isnil( L, pos) )
		return;
	// only accept tables
	luaL_checktype( L, pos, LUA_TTABLE );
	// TODO: check table for len==0 and return

	// adding fh to FD_SETs
	lua_pushnil( L );
	while (lua_next( L, pos ))
	{
		sck = t_net_sck_check_ud( L, -1, 1 );
		FD_SET( sck->fd, set );
		*maxFd = (sck->fd > *maxFd) ? sck->fd : *maxFd;
		lua_pop( L, 1 );   // remove the socket, keep key for next()
	}
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
lt_net_sck_select( lua_State *L )
{
	fd_set            rfds, wfds;
	struct t_net_sck *hndl;
	int               rnum, wnum, readsocks, i;

	wnum = -1;
	rnum = -1;
	t_net_sck_mkFdset( L, 1, &rfds, &rnum );
	t_net_sck_mkFdset( L, 2, &wfds, &wnum );

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
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
			hndl = t_net_sck_check_ud( L, -1, 1 );   //S: rdi wri rdr wrr key sck
			if FD_ISSET( hndl->fd, &rfds )
			{
				if (lua_isinteger( L, -2 ))
					lua_rawseti( L, i+2, lua_rawlen( L, i+2 )+1 );
				else
				{
					lua_pushvalue( L, -2 );
					lua_insert( L, -2 );               //S: rdi wri rdr wrr key key sck
					lua_rawset( L, i+2 );
				}
				if (0 == --readsocks) {
					lua_pop( L, 1 );
					break;
				}
			}
		}
	}
	return 2;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_sck_fm [] = {
	  { "__call",    lt_net_sck__Call }
	, { NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_cf [] =
{
	  { "select"     , lt_net_sck_select }
	, { "bind"       , lt_net_sck_bind }
	, { "connect"    , lt_net_sck_connect }
	, { "listen"     , lt_net_sck_listen }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_m [] =
{
	// metamethods
	  { "__tostring",  lt_net_sck__tostring }
	//, { "__eq",        lt_net__eq }
	, { "__gc",        lt_net_sck_close }  // reuse function
	// object methods
	, { "listen",      lt_net_sck_listen }
	, { "bind",        lt_net_sck_bind }
	, { "connect",     lt_net_sck_connect }
	, { "accept",      lt_net_sck_accept }
	, { "close",       lt_net_sck_close }
	, { "send",        lt_net_sck_send }
	, { "recv",        lt_net_sck_recv }
	, { "getsockname", lt_net_sck_getsockname }
	// generic net functions -> reuse functions
	, { "getFd",       lt_net_sck_getFd }
	//, { "setOption",   lt_net_setoption }
	, { NULL,        NULL }
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
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as Socket.<member>
	luaL_newlib( L, t_net_sck_cf );
	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_sck_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
