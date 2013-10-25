/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_socket.h
 * socket functions wrapped for lua
 *
 * data definitions
 */

#define CRC16             0xA001
#define MAX_BUF_SIZE      4096

enum xt_hndl_t {
	UDPHND,
	TCPHND,
	FILHND
};

static const char *const xt_hndl_t_lst[] = {
	"UDPHND",
	"TCPHND",
	"FILHND",
	NULL
};

struct xt_hndl {
	enum xt_hndl_t     hd_t;
	int                fd;    // socket, file handle, stream ...
	//t_timeout tm;
};


// Constructors
// l_net_timer.c
int luaopen_net_timer (lua_State *luaVM);
struct timeval *check_ud_timer (lua_State *luaVM, int pos);
struct timeval *create_ud_timer (lua_State *luaVM, int ms);

// l_net_ipendpoint.c
int luaopen_net_ipendpoint (lua_State *luaVM);
struct sockaddr_in *check_ud_ipendpoint (lua_State *luaVM, int pos);
struct sockaddr_in *create_ud_ipendpoint (lua_State *luaVM);

// l_net_socket.c
int luaopen_net_socket (lua_State *luaVM);
struct xt_hndl *check_ud_socket (lua_State *luaVM, int pos);
struct xt_hndl *create_ud_socket (lua_State *luaVM, enum xt_hndl_t type);


