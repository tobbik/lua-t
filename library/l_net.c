/**
 * \file        within the pktgen, cretae network endpoints according to need
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

#include "library.h"
#include "l_net.h"
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


/** -------------------------------------------------------------------------
 * \brief   create a UDP socket and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_create_udp_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 on=1;

	sock = (struct udp_socket*) lua_newuserdata(luaVM, sizeof(struct udp_socket));

	if ( (sock->socket  =  socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
		return( pusherror(luaVM, "ERROR opening UDP socket") );
	/* Enable address reuse */
	setsockopt( sock->socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	return 1 ;
}


/** -------------------------------------------------------------------------
 * \brief   create a TCP socket and return it.
 * \param   luaVM   The lua state.
 * \lreturn socket  socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_create_tcp_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 on=1;

	sock = (struct udp_socket*) lua_newuserdata(luaVM, sizeof(struct udp_socket));
	if ( (sock->socket  =  socket(AF_INET, SOCK_STREAM, 0)) == -1 )
		return( pusherror(luaVM, "ERROR opening TCP socket") );
	/* Enable address reuse */
	setsockopt( sock->socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	return( 1 );
}


/**--------------------------------------------------------------------------
 * create an IP endpoint and return it.
 * \param   luaVM  The lua state.
 * \lparam  port   the port for the socket.
 * \lparam  ip     the IP address for the socket.
 * \lreturn socket sockkaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int l_create_ip_endpoint(lua_State *luaVM)
{
	struct sockaddr_in  *ip;

	ip = (struct sockaddr_in *) lua_newuserdata (luaVM, sizeof(struct sockaddr_in) );
	memset( (char *) &(*ip), 0, sizeof(ip) );
	ip->sin_family = AF_INET;
	if (lua_isstring(luaVM, 1)) {
#ifdef _WIN32
		if ( InetPton(AF_INET, luaL_checkstring(luaVM, 1), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "InetPton() failed\n") );
#else
		if ( inet_pton(AF_INET, luaL_checkstring(luaVM,1), &(ip->sin_addr))==0)
			return ( pusherror(luaVM, "inet_aton() failed\n") );
#endif
	}
	else if (lua_isnil(luaVM, 1) )
		ip->sin_addr.s_addr = htonl(INADDR_ANY);

	ip->sin_port   = htons(luaL_checkint(luaVM, 2));

	return 1;
}


/** -------------------------------------------------------------------------
 * \brief   listen on a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  int    Backlog connections.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_listen_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 backlog;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR listen(socket, backlog) takes socket argument") );
	backlog = luaL_checkint(luaVM, 2);

	if( listen(sock->socket , backlog ) == -1)
		return( pusherror(luaVM, "ERROR listen to socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   bind a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket The socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_bind_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	struct sockaddr_in *ip;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR bind(socket,ip) takes socket argument") );
	if (lua_isuserdata(luaVM, 2))
		ip   = (struct sockaddr_in*) lua_touserdata(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR bind(socket,ip) takes IP argument") );

	if( bind(sock->socket , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   connect a socket to an address.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  ip     sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_connect_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	struct sockaddr_in *ip;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR connect(socket,ip) takes socket argument") );
	if (lua_isuserdata(luaVM, 2))
		ip   = (struct sockaddr_in*) lua_touserdata(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR connect(socket,ip) takes IP argument") );

	if( connect(sock->socket , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 0 );
}


/** -------------------------------------------------------------------------
 * \brief   accept a (TCP) socket connection.
 * \param   luaVM   The lua state.
 * \lparam  socket  socket userdata.
 * \lreturn socket  socket userdata for new connection.
 * \lreturn ip      sockaddr userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_accept_socket(lua_State *luaVM)
{
	struct udp_socket  *lsock;   // listening socket
	struct udp_socket  *asock;   // accepted connection socket
	struct sockaddr_in *si_cli;
	socklen_t           their_addr_size = sizeof(struct sockaddr_in);

	if (lua_isuserdata(luaVM, 1))
		lsock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR accept(socket) takes socket argument") );

	asock  = (struct udp_socket*)   lua_newuserdata (luaVM, sizeof(struct udp_socket)  );
	si_cli = (struct sockaddr_in *) lua_newuserdata (luaVM, sizeof(struct sockaddr_in) );
	asock->socket = accept(lsock->socket, (struct sockaddr *) &(*si_cli), &their_addr_size);

	return( 2 );
}


/** -------------------------------------------------------------------------
 * \brief   close a socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_close_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR close(socket,ip) takes socket argument") );

	if( close(sock->socket)  == -1)
		return( pusherror(luaVM, "ERROR closing socket") );

	return( 0 );
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
static int l_send_to(lua_State *luaVM)
{
	struct udp_socket  *sock;
	struct sockaddr_in *ip;
	int                 sent;
	const char         *msg;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes socket argument") );
	if (lua_isuserdata(luaVM, 2))
		ip   = (struct sockaddr_in*) lua_touserdata(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes IP argument") );
	if (lua_isstring(luaVM, 3))
		msg   = lua_tostring(luaVM, 3);
	else
		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes msg argument") );

	if ((sent = sendto(
	  sock->socket,
	  msg, strlen(msg), 0,
	  (struct sockaddr *) &(*ip), sizeof(struct sockaddr))
	  ) == -1)
		return( pusherror(luaVM, "Failed to send UDP packet") );

	lua_pushinteger(luaVM, sent);
	return( 1 );
}


/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint userdata.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_recv_from(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 rcvd;
	char                buffer[4096];

	struct sockaddr_in *si_cli;
	unsigned int        slen=sizeof(si_cli);

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR recvFrom(socket) takes socket argument") );

	si_cli = (struct sockaddr_in *) lua_newuserdata (luaVM, sizeof(struct sockaddr_in) );
	if ((rcvd = recvfrom(
	  sock->socket,
	  buffer, sizeof(buffer)-1, 0,
	  (struct sockaddr *) &(*si_cli), &slen)
	  ) == -1)
		return( pusherror(luaVM, "Failed to recieve UDP packet") );

	// return buffer, length, IpEndpoint
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);
	lua_pushvalue(luaVM, -3);
	return 3;
}


/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket into a byteBuffer.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  byteBuffer The Buffer to recieve into.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
//static int recv_from_into_buffer(lua_State *luaVM)
//{
//	struct udp_socket  *sock;
//	int                 rcvd;
//	struct byteBuffer  *buffer;
//
//	struct sockaddr_in  si_cli;
//	unsigned int        slen=sizeof(si_cli);
//
//
//	if (lua_isuserdata(luaVM, 1))
//		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
//	else
//		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes socket argument") );
//
//	if ((rcvd = recvfrom(
//	  sock->socket,
//	  buffer, sizeof(buffer)-1, 0,
//	  (struct sockaddr *) &si_cli, &slen)
//	  ) == -1)
//		return( pusherror(luaVM, "Failed to recieve UDP packet") );
//
//	// return buffer, length, ip, port
//	lua_pushlstring(luaVM, buffer, rcvd );
//	lua_pushinteger(luaVM, rcvd);
//	lua_pushstring(luaVM, inet_ntoa(si_cli.sin_addr) );
//	lua_pushinteger(luaVM, ntohs(si_cli.sin_port) );
//	return( 4 );
//
//}


/** -------------------------------------------------------------------------
 * \brief   send a message over a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lparam  msg    luastring.
 * \lreturn sent   number of bytes sent.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_send_strm(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 sent;
	const char         *msg;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR send(socket,msg) takes socket argument") );
	if (lua_isstring(luaVM, 2))
		msg   = lua_tostring(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR send(socket,msg) takes msg argument") );

	if ((sent = send(
	  sock->socket,
	  msg, strlen(msg), 0)
	  ) == -1)
		return( pusherror(luaVM, "Failed to send UDP packet") );

	lua_pushinteger(luaVM, sent);
	return( 1 );
}


/** -------------------------------------------------------------------------
 * \brief   recieve some data from a TCP socket.
 * \param   luaVM  The lua state.
 * \lparam  socket socket userdata.
 * \lreturn string The recieved message.
 * \lreturn rcvd   number of bytes recieved.
 * \return  The number of results to be passed back to the calling Lua script.
 * TODO:  Allow it to accept an existing LuaBuffer to ammend incoming packages
 *-------------------------------------------------------------------------*/
static int l_recv_strm(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 rcvd;
	char                buffer[4096];

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR recv(socket) takes socket argument") );

	if ((rcvd = recv(
	  sock->socket,
	  buffer, sizeof(buffer)-1, 0)
	  ) == -1)
		return( pusherror(luaVM, "Failed to recieve TCP packet") );

	// return buffer, length, ip, port
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);

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
static void make_fdset(lua_State *luaVM, int stack_pos, fd_set *collection, int *max_sock)
{
	int                 i;      // table iterator
	struct udp_socket  *sock;

	FD_ZERO(collection);
	// empty table == nil
	if (lua_isnil(luaVM, stack_pos)) return;
	// only accept tables
	luaL_checktype(luaVM, stack_pos, LUA_TTABLE);

	// adding sockets to FD_SETs
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
		sock = (struct udp_socket*) lua_touserdata(luaVM, -1);
		FD_SET( sock->socket, collection );
		*max_sock = (sock->socket > *max_sock) ? sock->socket : *max_sock;
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
static int l_select_socket(lua_State *luaVM)
{
	fd_set              rfds, wfds;
	struct udp_socket  *sock;
	int                 rnum, wnum, readsocks, i/*, rp*/;
	int                 rset=1;//, wset=1;   // write the values back into the tables on the stack

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
		sock = (struct udp_socket*) lua_touserdata(luaVM, -1);
		if FD_ISSET( sock->socket, &rfds)
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


/**
 * \brief      the net library definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg l_net_lib [] =
{
	{"createUdp", l_create_udp_socket},
	{"createTcp", l_create_tcp_socket},
	{"createIp",  l_create_ip_endpoint},
	{"listen",    l_listen_socket},
	{"bind",      l_bind_socket},
	{"connect",   l_connect_socket},
	{"accept",    l_accept_socket},
	{"close",     l_close_socket},
	{"sendTo",    l_send_to},
	{"recvFrom",  l_recv_from},
	{"send",      l_send_strm},
	{"recv",      l_recv_strm},
	{"select",    l_select_socket},
	{"sleep",     l_sleep},
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
	return 1;
}

