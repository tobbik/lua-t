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
#include <errno.h>
#endif
#include "l_sock.h"
//#include "l_byte_buffer.h"


/**
 * Return an error string to the LUA script.
 * @param L The Lua intepretter object.
 * @param info An error string to return to the caller.
 *             Pass NULL to use the return value of strerror.
 */
static int pusherror(lua_State *luaVM, const char *info)
{
	lua_pushnil(luaVM);
	if (info==NULL)
		lua_pushstring(luaVM, strerror(errno));
	else
		lua_pushfstring(luaVM, "%s: %s", info, strerror(errno));
	lua_pushnumber(luaVM, errno);
	return 3;
}

/**
 * \brief     Used to reverse the order of bytes for a 16 bit unsigned integer
 *
 * \param     value Unsigned 16 bit integer
 * \return    Integer with the opposite Endianness
 */
inline u_int2 Reverse2Bytes(u_int2 value)
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
 inline u_int4 Reverse4Bytes(u_int4 value)
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
 inline u_int8 Reverse8Bytes(u_int8 value)
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
	u_int2  *checksum      = (u_int2 *) (data + size -2);

	u_int2 out = 0;
	int bits_read = 0, bit_flag;
	int i;
	u_int2 crc = 0;
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
 * create a UDP socket and return it.
 * \param   L    The lua state.
 * \lparam  port the port for the socket.
 * \lparam  ip   the IP address for the socket.
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

/**--------------------------------------------------------------------------
 * create a TCP socket and return it.
 * \param   L    The lua state.
 * \lparam  port the port for the socket.
 * \lparam  ip   the IP address for the socket.
 * \lreturn socket Lua UserData wrapped socket.
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
 * \brief   bind a socket to an address.
 * \param   L      The lua state.
 * \lparam  socket The socket.
 * \lparam  ip     the IP endpoint.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_bind_socket(lua_State *luaVM)
{
	struct udp_socket  *sock;
	struct sockaddr_in *ip;

	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR bindUdp(socket,ip) takes socket argument") );
	if (lua_isuserdata(luaVM, 2))
		ip   = (struct sockaddr_in*) lua_touserdata(luaVM, 2);
	else
		return( pusherror(luaVM, "ERROR bindUdp(socket,ip) takes IP argument") );

	if( bind(sock->socket , (struct sockaddr*) &(*ip), sizeof(struct sockaddr) ) == -1)
		return( pusherror(luaVM, "ERROR binding socket") );

	return( 0 );
}

/** -------------------------------------------------------------------------
 * \brief   send Datagram over a UDP socket to an IP endpoint.
 * \param   L      The lua state.
 * \lparam  socket The socket.
 * \lparam  ip     the IP endpoint.
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

		printf("-------------------------------------------------------\n");
		printf("sent %d bytes to: %s:%d\n",
		  sent,
		  inet_ntoa(ip->sin_addr),
		  ntohs(ip->sin_port));
		printf("-------------------------------------------------------\n");

	lua_pushinteger(luaVM, sent);
	return( 1 );
}

/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket.
 * \param   L      The lua state.
 * \lparam  socket The socket.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
static int l_recv_from(lua_State *luaVM)
{
	struct udp_socket  *sock;
	int                 rcvd;
	char                buffer[4096];

	struct sockaddr_in  si_cli;
	unsigned int        slen=sizeof(si_cli);


	if (lua_isuserdata(luaVM, 1))
		sock = (struct udp_socket*) lua_touserdata(luaVM, 1);
	else
		return( pusherror(luaVM, "ERROR sendTo(socket,ip,msg) takes socket argument") );

	if ((rcvd = recvfrom(
	  sock->socket,
	  buffer, sizeof(buffer)-1, 0,
	  (struct sockaddr *) &si_cli, &slen)
	  ) == -1)
		return( pusherror(luaVM, "Failed to recieve UDP packet") );

	// return buffer, length, ip, port
	lua_pushlstring(luaVM, buffer, rcvd );
	lua_pushinteger(luaVM, rcvd);
	lua_pushstring(luaVM, inet_ntoa(si_cli.sin_addr) );
	lua_pushinteger(luaVM, ntohs(si_cli.sin_port) );
	return 4;

}


/** -------------------------------------------------------------------------
 * \brief   recieve Datagram from a UDP socket into a byteBuffer.
 * \param   L      The lua state.
 * \lparam  socket The socket.
 * \lparam  byteBuffer The Buffer to recieve into.
 * \lreturn rcvd   number of bytes recieved.
 * \lreturn ip     ip endpoint.
 * \return  The number of results to be passed back to the calling Lua script.
 *-------------------------------------------------------------------------*/
//static int recv_from_into_buffer(lua_State *L)
//{
//	struct udp_socket  *sock;
//	int                 rcvd;
//	struct byteBuffer  *buffer;
//
//	struct sockaddr_in  si_cli;
//	unsigned int        slen=sizeof(si_cli);
//
//
//	if (lua_isuserdata(L, 1))
//		sock = (struct udp_socket*) lua_touserdata(L, 1);
//	else
//		return( pusherror(L, "ERROR sendTo(socket,ip,msg) takes socket argument") );
//
//	if ((rcvd = recvfrom(
//	  sock->socket,
//	  buffer, sizeof(buffer)-1, 0,
//	  (struct sockaddr *) &si_cli, &slen)
//	  ) == -1)
//		return( pusherror(L, "Failed to recieve UDP packet") );
//
//	// return buffer, length, ip, port
//	lua_pushlstring(L, buffer, rcvd );
//	lua_pushinteger(L, rcvd);
//	lua_pushstring(L, inet_ntoa(si_cli.sin_addr) );
//	lua_pushinteger(L, ntohs(si_cli.sin_port) );
//	return( 4 );
//
//}



/**
 * \brief      a system call to sleep (Lua lacks that)
 * \detail     Lua has no build in sleep method.
 * \param      Lua state (provided by the Lua VM when invoked
 * \param      LuaStackParam int  milliseconds to sleep
 * \return     0 return values
 */
int l_sleep(lua_State *luaVM)
{
#ifdef _WIN32
	fd_set dummy;
	int s;
#endif
	u_int4   msec_len = (u_int4) luaL_checkint(luaVM, 1);
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


static const luaL_Reg socket_lib [] =
{
	{"createUdp", l_create_udp_socket},
	{"createIP",  l_create_ip_endpoint},
	{"bindUdp",   l_bind_socket},
	{"sendTo",    l_send_to},
	{"recvFrom",  l_recv_from},
	{"sleep",     l_sleep},
	{NULL,        NULL}
};

LUAMOD_API
int luaopen_net (lua_State *luaVM)
{
	luaL_newlib (luaVM, socket_lib);
	//lua_setglobal(L, "lsock");
	return 1;
}

