/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_sck.h
 * \brief      socket functions wrapped for lua
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

enum t_sck_t {
	T_SCK_NONE,
	T_SCK_UDP,
	T_SCK_TCP,
};

static const char *const t_sck_t_lst[] = {
	"NONE",
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
struct sockaddr_in *t_ipx_check_ud  ( lua_State *L, int pos, int check );
struct sockaddr_in *t_ipx_create_ud ( lua_State *L );
void                t_ipx_set       ( lua_State *L, int pos, struct sockaddr_in *ip );

// t_sck.c
int           luaopen_t_sck    ( lua_State *L );
struct t_sck *t_sck_check_ud   ( lua_State *L, int pos, int check );
struct t_sck *t_sck_create_ud  ( lua_State *L, enum t_sck_t type, int create );

int           t_sck_recv    ( lua_State *L, struct t_sck *sck, char* buff, size_t sz );
int           t_sck_send    ( lua_State *L, struct t_sck *sck, const char* buff, size_t sz );
int           t_sck_close   ( lua_State *L, struct t_sck *sck );
int           t_sck_reuseaddr ( lua_State *L, struct t_sck *sck );
int           lt_sck_bind   ( lua_State *L );
int           lt_sck_connect( lua_State *L );
int           t_sck_listen  ( lua_State *L, int pos );
int           t_sck_accept  ( lua_State *L, int pos );
