/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_t_hndl.h
 * socket functions wrapped for lua
 *
 * data definitions
 */

enum t_sck_t {
	T_SCK_UDP,
	T_SCK_TCP,
};

static const char *const t_sck_t_lst[] = {
	"UDP",
	"TCP",
	NULL
};

struct t_sck {
	enum t_sck_t    t;
	int             fd;    ///< socket handle
};

// Constructors
// t_ipx.c
struct sockaddr_in *t_ipx_check_ud  ( lua_State *luaVM, int pos, int check );
struct sockaddr_in *t_ipx_create_ud ( lua_State *luaVM );
void                t_ipx_set       ( lua_State *luaVM, int pos, struct sockaddr_in *ip );

// t_sck.c
int           luaopen_t_sck    ( lua_State *luaVM );
struct t_sck *t_sck_check_ud   ( lua_State *luaVM, int pos, int check );
struct t_sck *t_sck_create_ud  ( lua_State *luaVM, enum t_sck_t type );

int           t_sck_recv_tdp( lua_State *luaVM, struct t_sck *sck, char* buff, size_t sz );
int           lt_sck_bind   ( lua_State *luaVM );
int           lt_sck_connect( lua_State *luaVM );
int           t_sck_listen  ( lua_State *luaVM, int pos );
int           t_sck_accept  ( lua_State *luaVM, int pos );
