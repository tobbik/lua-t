/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_socket.h
 * socket functions wrapped for lua
 *
 * data definitions
 */

#define CRC16             0xA001
#define MAX_PKT_BYTES     1500

enum sock_type {
	UDP,
	TCP
};

struct udp_socket {
	int                socket;    // socket
	//t_timeout tm;
};

struct tdp {
	int                socket;    // socket
	//enum sock_type     type;      // UDP, TCP ...
	//t_timeout tm;
};


// Constructors
// l_net_ipendpoint.c
int luaopen_net_ipendpoint (lua_State *luaVM);
struct sockaddr_in *check_ud_ipendpoint (lua_State *luaVM, int pos);
struct sockaddr_in *create_ud_ipendpoint (lua_State *luaVM);

