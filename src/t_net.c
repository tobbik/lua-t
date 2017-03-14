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


// ---------------------- HELPERS ---------------------------------------------
int
t_net_getProtocol( lua_State *L, const int pos )
{
	const char      *pName = luaL_optstring( L, pos, "TCP" );
	struct protoent  result_buf;
	struct protoent *result;
	char             buf[ BUFSIZ ];

#if defined(__linux__)
	luaL_argcheck( L,
		getprotobyname_r( pName, &result_buf, buf, sizeof( buf ), &result) == 0,
		pos,
		"unknown protocol name" );
#else
	result = getprotobyname( pName );
	luaL_argcheck( L, result != NULL, pos, "unknown protocol name" );
	result_buf = *result;
#endif
	return result_buf.p_proto;
}

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
 * \param   L      Lua state.
 * \param   int    Position on the stack.
 * \return  struct t_net* pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
void
t_net_getdef( lua_State *L, const int pos, struct t_net_sck **sock, struct sockaddr_in **addr )
{
	*sock = t_net_sck_check_ud( L, pos+0, 0 );
	*addr = t_net_ip4_check_ud( L, pos+1, 0 );

	if (NULL == *sock)     // handle T.Net.whatever( )
	{
		// in all shortcuts of listen(ip,port), bind(ip,port), connect(ip,port)
		// It defaults to IPv4 and TCP, for others Sockets and Addresses must be
		// created explicitely
		*sock = t_net_sck_create_ud( L, AF_INET, SOCK_STREAM, IPPROTO_TCP, 1  );
		lua_insert( L, pos+0 );
	}

	if (NULL == *addr)
	{
		*addr = t_net_ip4_create_ud( L );
		t_net_ip4_set( L, pos+1, *addr );
		lua_insert( L, pos+1 );
	}
}


int
t_net_testOption( lua_State *L, int pos, const char *const lst[] )
{
	const char *nme = luaL_checkstring( L, pos );
	int i;
	for (i=0; lst[i]; i++)
		if (strcmp( lst[i], nme ) == 0)
			return i;
	return -1;
}


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
	struct t_net_sck *s = t_net_sck_check_ud( L, 1, 1 );
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


/** -------------------------------------------------------------------------
 * Set a socket option.
 * \param   L      Lua state.
 * \param   struct t_net* instance.
 * \return  int    # of values pushed onto the stack.
 * TODO: Actually do option settings with multiple options etc here...
 *-------------------------------------------------------------------------*/
int
t_net_reuseaddr( lua_State *L, struct t_net_sck *s )
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
	struct t_net_sck *s = t_net_sck_check_ud( L, 1, 1 );
	return t_net_reuseaddr( L, s );
}

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_cf [] =
{
	  { "setOption",   lt_net_setoption }
#ifndef _WIN32
	, { "showAllFd",   lt_net_getfdsinfo }
#endif
	, { NULL,    NULL }
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
	luaopen_t_net_ip4( L );
	lua_setfield( L, -2, T_NET_IP4_NAME );
	luaopen_t_net_ifc( L );
	lua_setfield( L, -2, T_NET_IFC_NAME );
	luaopen_t_net_sck( L );
	lua_setfield( L, -2, T_NET_SCK_NAME );
	return 1;
}
