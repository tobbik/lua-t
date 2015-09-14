/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net.c
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


#ifndef _WIN32
/** -------------------------------------------------------------------------
 * check the set parameters of a particular Socket/Filedescriptor
 * \param   int  The File/Socket descriptor ident.
 * \return  int  Boolean - 1 if found; 0 if not.
 *-------------------------------------------------------------------------*/
int t_net_getfdinfo( int fd )
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


int lt_net_getfdinfo( lua_State *L )
{
	struct t_net *s = t_net_check_ud( L, 1, 1 );
	t_net_getfdinfo( s->fd );
	return 0;
}


int lt_net_getfdsinfo( lua_State *L )
{
	UNUSED( L );
	int numFd  = getdtablesize();
	int fd;
	for (fd=0; fd<numFd; fd++)
		t_net_getfdinfo( fd );
	return 0;
}
#endif

/**--------------------------------------------------------------------------
 * Evaluate elements on stack to be definitions or instances of sock and ip.
 * This is a helper function to handle possible input scenarios for many
 * functions such as bind, listen, connect etc.
 * possible combinations:
 *  - ...( sck, ip )
 *  - ...( ip)     TODO: FIX THIS
 *  - ...( sck,'ipString', port )
 *  - ...( sck, port )           -- IP will be 0.0.0.0
 *  - ...( sck, 'ipstring' )     -- Port unassigned
 *  - ...( sck )                 -- Port unassigned, IP 0.0.0.0
 *
 * \param   L              The lua state.
 * \param   int            position on the stack.
 * \return  struct t_net*  pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
void
t_net_getdef( lua_State *L, int pos, struct t_net **s, struct sockaddr_in **ip,
              enum t_net_t t )
{
	*s  = t_net_check_ud( L, pos+0, 0 );
	*ip = t_net_ip4_check_ud( L, pos+1, 0 );

	if (NULL == *s)     // handle T.Net.whatever( )
	{
		*s = t_net_create_ud( L, t, 1 );
		lua_insert( L, pos+0 );
	}

	if (NULL == *ip)
	{
		*ip = t_net_ip4_create_ud( L );
		t_net_ip4_set( L, pos+1, *ip );
		lua_insert( L, pos+1 );
	}
}


/**--------------------------------------------------------------------------
 * Create a socket and push to LuaStack.
 * \param   L         The lua state.
 * \param   enum t_net_t  Type of socket.
 * \param   bool/int      Should socket be created.
 * \return  struct t_net* pointer to the socket struct.
 * --------------------------------------------------------------------------*/
struct t_net
*t_net_create_ud( lua_State *L, enum t_net_t type, int create )
{
	struct t_net  *s = (struct t_net*) lua_newuserdata( L, sizeof( struct t_net ) );
	size_t         one = 1;

	if (create)
	{
		switch (type)
		{
			case T_NET_UDP:
				if ( (s->fd  =  socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1 )
					return NULL;
				luaL_getmetatable( L, "T.Net.UDP" );
				break;

			case T_NET_TCP:
				if ( (s->fd  =  socket( AF_INET, SOCK_STREAM, 0 )) == -1 )
					return NULL;
				if (-1 == setsockopt( s->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) )
					return NULL;
				luaL_getmetatable( L, "T.Net.TCP" );
				break;

			default:
				return NULL;
		}
	}

	s->t = type;

	lua_setmetatable( L, -2 );
	return s;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net.
 * \param   L    The lua state.
 * \param   int      position on the stack.
 * \return  struct t_net*  pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
struct t_net
*t_net_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Net.TCP" );
	if (NULL == ud)
		ud = luaL_testudata( L, pos, "T.Net.UDP" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Net.TCP/UDP` expected" );
	return (NULL==ud) ? NULL : (struct t_net *) ud;
}


/** -------------------------------------------------------------------------
 * Set a socket option.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \return  int    # of values pushed onto the stack.
 * TODO: Actually do option settings with multiple options etc here...
 *-------------------------------------------------------------------------*/
int
t_net_reuseaddr( lua_State *L, struct t_net *s )
{
	size_t one           = 1;
	int flag             = 0;

	if (-1 != s->fd)
	{
		if (-1 == setsockopt( s->fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof( one ) ) )
			return t_push_error( L, "ERROR setting option" );
		flag = fcntl( s->fd, F_GETFL, 0 );           // Get socket flags
		fcntl( s->fd, F_SETFL, flag | O_NONBLOCK );  // Add non-blocking flag
	}

	return 0;
}


int
lt_net_setoption( lua_State *L )
{
	struct t_net *s = t_net_check_ud( L, 1, 1 );
	return t_net_reuseaddr( L, s );
}



/** -------------------------------------------------------------------------
 * Close a socket.
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_close( lua_State *L, struct t_net *s )
{
	if (-1 != s->fd)
	{
		//printf( "closing socket: %d\n", s->fd );
		if (-1 == close( s->fd ))
			return t_push_error( L, "ERROR closing socket" );
		else
		{
			s->fd = -1;         // invalidate socket
		}
	}

	return 0;
}


int
lt_net_close( lua_State *L )
{
	struct t_net *s = t_net_check_ud( L, 1, 1 );
	return t_net_close( L, s );
}


/** -------------------------------------------------------------------------
 * Return the FD int representation of the socket
 * \param   L  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn socketFD int.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
lt_net_getfdid( lua_State *L )
{
	struct t_net      *s = t_net_check_ud( L, 1, 1 );
	lua_pushinteger( L, s->fd );
	return 1;
}


/**--------------------------------------------------------------------------
 * Prints out the socket.
 * \param   L    The lua state.
 * \lparam  struct t_net the socket userdata.
 * \lreturn string   formatted string representing sockkaddr (IP:Port).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net__tostring( lua_State *L )
{
	struct t_net *s = t_net_check_ud( L, 1, 1 );
	lua_pushfstring( L, "T.Net.%s{%d}: %p",
			(T_NET_TCP == s->t) ? "TCP" : "UDP",
			s->fd,
			s );
	return 1;
}


/** -------------------------------------------------------------------------
 * Listen on a socket or create a listening socket.
 * \param   L             The lua state.
 * \lparam  int           position on stack where socket might be.
 * \lparam  enum t_net_t  position on stack where socket might be.
 * \lparam  T.Net.TCP/UDP The socket userdata.
 * \lparam  int           Backlog connections.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_listen( lua_State *L, int pos, enum t_net_t t )
{
	struct t_net       *s  = t_net_check_ud( L, pos+0, 0 );
	struct sockaddr_in *ip = t_net_ip4_check_ud( L, pos+1, 0 );
	int                 backlog;

	if (NULL == s)
	{
		s = t_net_create_ud( L, t, 1 );
		lua_insert( L, pos+0 );

		t_net_getdef( L, pos+0, &s, &ip, t );
		t_stackDump( L );
		//S: t_net,t_net_ip4
		if (bind( s->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
			return t_push_error( L, "ERROR binding socket to %s:%d",
					 inet_ntoa( ip->sin_addr ),
					 ntohs( ip->sin_port ) );
	}
	backlog = luaL_checkinteger( L, pos+2 );
	lua_remove( L, pos+2 );

	if (listen( s->fd , backlog ) == -1)
		return t_push_error( L, "ERROR listen to socket" );

	return 2;  // socket, ip
}


/** -------------------------------------------------------------------------
 * Bind a socket to an address.
 * \param   L  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  int    # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
int
t_net_bind( lua_State *L, enum t_net_t t )
{
	struct t_net       *s  = NULL;
	struct sockaddr_in *ip = NULL;

	t_net_getdef( L, 1, &s, &ip, t );

	if (bind( s->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
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
t_net_connect( lua_State *L, enum t_net_t t )
{
	struct t_net       *s  = NULL;
	struct sockaddr_in *ip = NULL;

	t_net_getdef( L, 1, &s, &ip, t );

	if (connect( s->fd , (struct sockaddr*) &(*ip), sizeof( struct sockaddr ) ) == -1)
		return t_push_error( L, "ERROR connecting socket to %s:%d",
					 inet_ntoa(ip->sin_addr),
					 ntohs(ip->sin_port) );

	return 2; //socket,ip
}


/** -------------------------------------------------------------------------
 * Helper to take sockets from Lua tables to FD_SET.
 * Itertates over the table puls out the socket structs and adds the actual
 * sockets to the fd_set.
 * \param   L  The lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
static void
make_fdset( lua_State *L, int stack_pos, fd_set *collection, int *max_hndl )
{
	struct t_net  *hndl;

	FD_ZERO( collection );
	// empty table == nil
	if (lua_isnil( L, stack_pos) )
	{
		return;
	}
	// only accept tables
	luaL_checktype( L, stack_pos, LUA_TTABLE );
	// TODO: check table for len==0 and return

	// adding fh to FD_SETs
	lua_pushnil( L );
	while (lua_next( L, stack_pos ))
	{
		hndl = t_net_check_ud( L, -1, 1 );
		FD_SET( hndl->fd, collection );
		*max_hndl = (hndl->fd > *max_hndl) ? hndl->fd : *max_hndl;
		lua_pop( L, 1 );   // remove the socket, keep key for next()
	}
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   L  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  int    # of values pushed onto the stack.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_net_select( lua_State *L )
{
	fd_set          rfds, wfds;
	struct t_net   *hndl;
	int             rnum, wnum, readsocks, i/*, rp*/;
	int             rset=1, wset=1;

	wnum = -1;
	rnum = -1;
	make_fdset( L, 1, &rfds, &rnum );
	make_fdset( L, 2, &wfds, &wnum );

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create result table
	lua_createtable( L, 0, 0 );     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti( L, 1, i );
		// in table this is when the last index is found
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );
			break;
		}
		hndl = t_net_check_ud( L, -1, 1 );
		if FD_ISSET( hndl->fd, &rfds )
		{
			lua_rawseti( L, -3, rset++ );
			readsocks--;
			if (0 == readsocks)
				break;
		}
		else
			lua_pop( L, 1 );
	}
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti( L, 2, i );
		// in table this is when the last index is found
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );
			break;
		}
		hndl = t_net_check_ud( L, -1, 1 );
		if FD_ISSET( hndl->fd, &wfds )
		{
			lua_rawseti( L, -2, wset++ );
			readsocks--;
			if (0 == readsocks)
				break;
		}
		else
			lua_pop( L, 1 );
	}
	return 2;
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   L  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  int    # of values pushed onto the stack.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int
lt_net_selectk( lua_State *L )
{
	fd_set          rfds, wfds;
	struct t_net   *hndl;
	int              rnum, wnum, readsocks /*, rp*/;

	wnum = -1;
	rnum = -1;
	make_fdset( L, 1, &rfds, &rnum );
	make_fdset( L, 2, &wfds, &wnum );

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable( L, 0, 0 );     // create result table
	//lua_createtable(L, 0, 0);
	lua_pushnil( L );
	while (lua_next( L, 1 ))
	{
		hndl = t_net_check_ud( L, -1, 1 );
		if FD_ISSET( hndl->fd, &rfds )
		{
			//TODO: Find better way to preserve key for next iteration
			lua_pushvalue( L, -2 );
			lua_pushvalue( L, -2 );
			lua_rawset( L, -5 );
			if (0 == --readsocks) {
				lua_pop( L, 2 );
				break;
			}
		}
		lua_pop( L, 1 );
	}
	return 1;
}

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_cf [] =
{
	{ "select",      lt_net_select },
	{ "selectK",     lt_net_selectk },
#ifndef _WIN32
	{ "showAllFd",   lt_net_getfdsinfo },
#endif
	{ NULL,    NULL }
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
luaopen_t_net( lua_State *L )
{
	luaL_newlib( L, t_net_cf );
	luaopen_t_net_tcp( L );
	lua_setfield( L, -2, "TCP" );
	luaopen_t_net_udp( L );
	lua_setfield( L, -2, "UDP" );
	luaopen_t_net_ip4( L );
	lua_setfield( L, -2, "IPv4" );
	return 1;
}
