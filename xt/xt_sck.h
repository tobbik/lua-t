/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * l_xt_hndl.h
 * socket functions wrapped for lua
 *
 * data definitions
 */

enum xt_sck_t {
	XT_SCK_UDP,
	XT_SCK_TCP,
};

static const char *const xt_sck_t_lst[] = {
	"UDP",
	"TCP",
	NULL
};

struct xt_sck {
	enum xt_sck_t    t;
	int              fd;    ///< socket handle
};

// Constructors
// xt_ip.c
struct sockaddr_in *xt_ip_check_ud  (lua_State *luaVM, int pos, int check);
struct sockaddr_in *xt_ip_create_ud (lua_State *luaVM);
int                 xt_ip_set       (lua_State *luaVM, int pos, struct sockaddr_in *ip);

// xt_sck.c
int            luaopen_xt_sck    ( lua_State *luaVM );
struct xt_sck *xt_sck_check_ud   ( lua_State *luaVM, int pos );
struct xt_sck *xt_sck_create_ud  ( lua_State *luaVM, enum xt_sck_t type);

