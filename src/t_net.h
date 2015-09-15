/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net.h
 * \brief      socket functions wrapped for lua
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

enum t_net_t {
	T_NET_NONE,
	T_NET_UDP,
	T_NET_TCP,
};

static const char *const t_net_t_lst[] = {
	"NONE",
	"UDP",
	"TCP",
	NULL
};

struct t_net {
	enum t_net_t    t;
	int             fd;    ///< socket handle
};

// Constructors
// t_net_ip4.c
struct sockaddr_in *t_net_ip4_check_ud  ( lua_State *L, int pos, int check );
struct sockaddr_in *t_net_ip4_create_ud ( lua_State *L );
void                t_net_ip4_set       ( lua_State *L, int pos, struct sockaddr_in *ip );

// t_net.c
int           luaopen_t_net_tcp ( lua_State *L );
struct t_net *t_net_check_ud    ( lua_State *L, int pos, int check );
struct t_net *t_net_create_ud   ( lua_State *L, enum t_net_t type, int create );
void          t_net_getdef      ( lua_State *L, int pos, struct t_net **s,
                                  struct sockaddr_in **ip, enum t_net_t t );
int           t_net_close       ( lua_State *L, struct t_net *s );
int           t_net_reuseaddr   ( lua_State *L, struct t_net *s );
int          lt_net_setoption   ( lua_State *L );
int          lt_net_close       ( lua_State *L );
int          lt_net_getfdid     ( lua_State *L );
int          lt_net_getfdinfo   ( lua_State *L );
int           t_net_listen      ( lua_State *L, int pos, enum t_net_t t );
int           t_net_bind        ( lua_State *L, enum t_net_t t );
int           t_net_connect     ( lua_State *L, enum t_net_t t );
int          lt_net__tostring   ( lua_State *L );


// t_net_tcp.c
struct t_net *t_net_tcp_check_ud   ( lua_State *L, int pos, int check );

int           t_net_tcp_recv    ( lua_State *L, struct t_net *s, char* buff, size_t sz );
int           t_net_tcp_send    ( lua_State *L, struct t_net *s, const char* buff, size_t sz );
int           t_net_tcp_accept  ( lua_State *L, int pos );

// t_net_udp.c
int           luaopen_t_net_udp ( lua_State *L );
struct t_net *t_net_udp_check_ud( lua_State *L, int pos, int check );



int           luaopen_t_net_ip4 ( lua_State *L );
void          t_net_ip4_set     ( lua_State *L, int pos, struct sockaddr_in *ip );
struct sockaddr_in  *t_net_ip4_create_ud ( lua_State *L );
struct sockaddr_in  *t_net_ip4_check_ud  ( lua_State *L, int pos, int check );
