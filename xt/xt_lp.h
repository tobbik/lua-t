#include "l_xt_hndl.h"
#include "xt_lp.h"


enum xt_lp_t {
	XT_LP_READ,
	XT_LP_WRIT,
	XT_LP_ONCE,
	XT_LP_MULT,
}


/// xt_lp implementation for select based loops
struct xt_lp {
	fd_set           rfds;
	fd_set           wfds;
	int              mxfd;
	struct xt_lp_tm *tm_head;
};


struct xt_lp_fd {
	enum xt_lp_t    t;
	int             fd;    ///< descriptor
	int           (*r_proc) (lua_State *luaVM);
	int           (*w_proc) (lua_State *luaVM);
	int             aR;    ///< argument reference in LUA_REGISTRYINDEX
};


struct xt_lp_tm {
	enum xt_lp_t        t;
	struct timeval      tv;    ///< time to elapse until fire
	struct timeval      it;    ///< time interval between fire
	int               (*t_proc) (lua_State *luaVM);
	int                 aR;    ///< argument reference in LUA_REGISTRYINDEX
	struct xt_lp_tmev  *nxt;   ///< next pointer for linked list
};


