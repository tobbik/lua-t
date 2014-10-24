#include "xt_sck.h"


enum xt_lp_t {
	XT_LP_READ,
	XT_LP_WRIT,
	XT_LP_ONCE,
	XT_LP_MULT,
};


struct xt_lp_fd {
	enum xt_lp_t    t;
	int             fd;    ///< descriptor
	int             fR;    ///< function reference in LUA_REGISTRYINDEX
};


struct xt_lp_tm {
	enum xt_lp_t        t;
	int                 id;
	struct timeval      tv;    ///< time to elapse until fire
	struct timeval     *it;    ///< time interval between fire
	int                 fR;    ///< function reference in LUA_REGISTRYINDEX
	struct xt_lp_tm    *nxt;   ///< next pointer for linked list
};

/// xt_lp implementation for select based loops
struct xt_lp {
	fd_set           rfds;
	fd_set           wfds;
	int              mxfd;
	size_t           mx_sz;
	struct xt_lp_fd *fd_head;
	struct xt_lp_tm *tm_head;
};


// xt_lp.c
struct xt_lp *xt_lp_check_ud ( lua_State *luaVM, int pos );
struct xt_lp *xt_lp_create_ud( lua_State *luaVM );
int           lxt_lp_New     ( lua_State *luaVM );



