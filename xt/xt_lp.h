enum xt_lp_t {
	XT_LP_TIMER,
	XT_LP_SOCKET,
	XT_LP_FILE
};


struct xt_lp {
	enum xt_lp_t    t;     ///< loop item type
	int             fd;    ///< descriptor
	int           (*t_proc) (lua_State *luaVM);
	struct xt_lp   *prv;   ///< previous item in loop
	struct xt_lp   *nxt;   ///< next     item in loop
};


struct xt_lp = *xt_lp;
