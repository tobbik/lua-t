/**
 * \file        manage net library
 */

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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#include "l_xt.h"
#include "l_xt_net.h"
//#include "l_byte_buffer.h"


/**
 * \brief     Used to reverse the order of bytes for a 16 bit unsigned integer
 *
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 */
inline uint16_t Reverse2Bytes(uint16_t value)
{
	return (
		(value & 0xFFU) << 8 |
		(value & 0xFF00U) >> 8
	);
}


/**
 * \brief     Used to reverse the order of bytes for a 32 bit unsigned integer
 *
 * \param     value Unsigned 32 bit integer
 * \return    integer with the opposite Endianness
 */
 inline uint32_t Reverse4Bytes(uint32_t value)
{
	return (value & 0x000000FFU) << 24 |
			 (value & 0x0000FF00U) << 8  |
			 (value & 0x00FF0000U) >> 8  |
			 (value & 0xFF000000U) >> 24;
}


/**
 * \brief   Used to reverse the order of bytes for a 64 bit unsigned integer
 *
 * \param   value Unsigned 64 bit integer
 * \return  Integer with the opposite Endianness
 */
 inline uint64_t Reverse8Bytes(uint64_t value)
{
	return (value & 0x00000000000000FFUL) << 56 |
			 (value & 0x000000000000FF00UL) << 40 |
			 (value & 0x0000000000FF0000UL) << 24 |
			 (value & 0x00000000FF000000UL) << 8  |
			 (value & 0x000000FF00000000UL) >> 8  |
			 (value & 0x0000FF0000000000UL) >> 24 |
			 (value & 0x00FF000000000000UL) >> 40 |
			 (value & 0xFF00000000000000UL) >> 56;
}


/**
 * \brief       calculate the CRC16 checksum
 * \detail      calculate over the payload and sticks it to the end of the buffer
 * \param       char buffer *data  data including the needed extra (2) bytes
 * \param       size_t size  how much of data is the payload
 */
static void get_crc16(const unsigned char *data, size_t size)
{
	uint16_t  *checksum      = (uint16_t *) (data + size -2);

	uint16_t out = 0;
	int bits_read = 0, bit_flag;
	int i;
	uint16_t crc = 0;
	int j = 0x0001;

	/* Sanity check: */
	if(data == NULL)
		return ;

	// the size reported here includes the padding for the chacksum -> remove for calculation
	size-=2;
	while(size > 0)
	{
		bit_flag = out >> 15;

		/* Get next bit: */
		out <<= 1;
		out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

		/* Increment bit counter: */
		bits_read++;
		if(bits_read > 7)
		{
			bits_read = 0;
			data++;
			size--;
		}

		/* Cycle check: */
		if(bit_flag)
			out ^= CRC16;
	}

	// item b) "push out" the last 16 bits
	for (i = 0; i < 16; ++i) {
		bit_flag = out >> 15;
		out <<= 1;
		if(bit_flag)
			out ^= CRC16;
	}
	
	//item c) reverse the bits
	i = 0x8000;
	for (; i != 0; i >>=1, j <<= 1) {
		if (i & out) crc |= j;
	}

	*checksum = crc;
}


/**
 * \brief      a system call to sleep (Lua lacks that)
 *             Lua has no build in sleep method.
 * \param      The Lua state.
 * \lparam     int  milliseconds to sleep
 * \return     0 return values
 */
static int l_sleep(lua_State *luaVM)
{
#ifdef _WIN32
	fd_set dummy;
	int s;
#endif
	uint32_t         msec_len = (uint32_t) luaL_checkint(luaVM, 1);
	struct timeval to;
	to.tv_sec  = msec_len/1000;
	to.tv_usec = (msec_len % 1000) * 1000;
#ifdef _WIN32
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	FD_ZERO(&dummy);
	FD_SET(s, &dummy);
	select(0, 0,0,&dummy, &to);
#else
	select(0, 0,0,0, &to);
#endif
	return 0;
}


/** -------------------------------------------------------------------------
 * \brief   helper to create and populate an IP endpoint
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static struct sockaddr_in *create_ud_tcp_ipendpoint(lua_State *luaVM)
{
	struct sockaddr_in  *ip;
	int                  port;
	ip   = create_ud_ipendpoint (luaVM);
	memset( (char *) &(*ip), 0, sizeof(ip) );
	ip->sin_family = AF_INET;

	if (lua_isstring(luaVM, 1)) {
#ifdef _WIN32
		if ( InetPton (AF_INET, luaL_checkstring(luaVM, 1), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "InetPton() failed\n") );
#else
		if ( inet_pton(AF_INET, luaL_checkstring(luaVM, 1), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "inet_aton() failed\n") );
#endif
		if ( lua_isnumber(luaVM, 2) ) {
			port = luaL_checkint(luaVM, 3);
			luaL_argcheck(luaVM, 1 <= port && port <= 65536, 2,
								  "port number out of range");
			ip->sin_port   = htons(port);
		}
	}
	else if ( lua_isnumber(luaVM, 1) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
		port = luaL_checkint(luaVM, 1);
		luaL_argcheck(luaVM, 1 <= port && port <= 65536, 1,
							  "port number out of range");
		ip->sin_port   = htons(port);
	}
	else if (lua_isnil(luaVM, 1) ) {
		ip->sin_addr.s_addr = htonl(INADDR_ANY);
	}

	return ip;
}


/** -------------------------------------------------------------------------
 * \brief   convienience connect to create the TCP socket and the endpoint
 * \param   luaVM   The lua state.
 * \lparam  xt_hndl The socket userdata.
 * \lparam  ip      sockaddr userdata.
 *    /// or
 * \lparam  ip      string IP address.
 * \lparam  port    int port.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_connect_net(lua_State *luaVM)
{
	struct xt_hndl      *hndl;
	struct sockaddr_in  *ip;

	hndl = create_ud_socket (luaVM, TCP);

	if ( lua_isuserdata(luaVM, 1) ) {
		// it's assumed that IP/port et cetera are assigned
		ip   = check_ud_ipendpoint (luaVM, 1);
		lua_pushvalue(luaVM, 1);
	}
	else {
		ip   = create_ud_tcp_ipendpoint (luaVM);
	}

	if( bind(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 2 );
}


/** -------------------------------------------------------------------------
 * \brief   convienience bind to create the TCP socket and the endpoint
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 *    /// or
 * \lparam  ip     string IP address.
 * \lparam  port   int port.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_bind_net(lua_State *luaVM)
{
	struct xt_hndl     *hndl;
	struct sockaddr_in *ip;
	
	hndl = create_ud_socket (luaVM, TCP);

	if ( lua_isuserdata(luaVM, 1) ) {
		// it's assumed that IP/port et cetera are assigned
		ip   = check_ud_ipendpoint (luaVM, 1);
		lua_pushvalue(luaVM, 1);
	}
	else {
		ip   = create_ud_tcp_ipendpoint (luaVM);
	}

	if( bind(hndl->fd , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 2 );
}


/** -------------------------------------------------------------------------
 * \brief   Helper to take sockets from Lua tables to FD_SET
 *          Itertates over the table puls out the socket structs and adds the
 *          actual sockets to the fd_set
 * \param   luaVM  The lua state.
 * \param   int    position on stack where table is located
 * \param  *fd_set the set of sockets(fd) to be filled
 * \param  *int    the maximum socket(fd) value
 * \return  void.
 *-------------------------------------------------------------------------*/
static void make_fdset(lua_State *luaVM, int stack_pos, fd_set *collection, int *max_hndl)
{
	int              i;      // table iterator
	struct xt_hndl  *hndl;

	FD_ZERO(collection);
	// empty table == nil
	if (lua_isnil(luaVM, stack_pos)) return;
	// only accept tables
	luaL_checktype(luaVM, stack_pos, LUA_TTABLE);

	// adding fh to FD_SETs
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, stack_pos, i);
		// in table this is when the last index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}

		// get the values and clear the stack
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		FD_SET( hndl->fd, collection );
		*max_hndl = (hndl->fd > *max_hndl) ? hndl->fd : *max_hndl;
		lua_pop(luaVM, 1);   // remove the socket from the stack
	}
}


/** -------------------------------------------------------------------------
 * \brief   select from open sockets.
 * \param   luaVM  The lua state.
 * \lparam  socket_array   All sockets to read from.
 * \lparam  socket_array   All sockets to write to.
 * \lparam  socket_array   All sockets to check errors.
 * \lreturn int            Number of affected sockets.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow for a Time Out to be handed to it
 *-------------------------------------------------------------------------*/
static int l_select_handle(lua_State *luaVM)
{
	fd_set           rfds, wfds;
	struct xt_hndl  *hndl;
	int              rnum, wnum, readsocks, i/*, rp*/;
	int              rset=1;//, wset=1;

	wnum = -1;
	rnum = -1;
	make_fdset(luaVM, 1, &rfds, &rnum);
	//make_fdset(luaVM, 2, &wfds, &wnum);
	FD_ZERO(&wfds);

	readsocks = select(
		(wnum > rnum) ? wnum+1 : rnum+1,
		(-1  != rnum) ? &rfds  : NULL,
		(-1  != wnum) ? &wfds  : NULL,
		(fd_set *) 0,
		NULL
	);

	lua_createtable(luaVM, 0, 0);     // create result table
	//TODO: check if readsocks hit 0 and break cuz we are done
	//lua_createtable(luaVM, 0, 0);
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}

		// get the values and clear the stack
		hndl = (struct xt_hndl*) lua_touserdata(luaVM, -1);
		if FD_ISSET( hndl->fd, &rfds)
		{
			// put into reads
			lua_rawseti(luaVM, -2, rset++);
			readsocks--;
		}
		
		//if FD_ISSET( sock->socket, &wfds)
		//{
		//	// put into writes
		//	lua_rawseti(luaVM, -2, wset++);
		//	readsocks--;
		//}
		else {
			lua_pop(luaVM, 1);   // remove the socket from the stack
		}
	}
	//lua_pop(luaVM, -1);   // remove the table from the stack

	return (1);

}


/**
 * \brief      the net library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_net_lib [] =
{
	{"sleep",     l_sleep},
	{"select",    l_select_handle},
	{"bind",      l_bind_net},
	{"connect",   l_connect_net},
	{NULL,        NULL}
};


/**
 * \brief     Export the net library to Lua
 *\param      The Lua state.
 * \return     1 return value
 */
LUAMOD_API int luaopen_net (lua_State *luaVM)
{
	luaL_newlib (luaVM, l_net_lib);
	luaopen_net_ipendpoint(luaVM);
	lua_setfield(luaVM, -2, "IpEndpoint");
	luaopen_net_socket(luaVM);
	lua_setfield(luaVM, -2, "Socket");
	return 1;
}

