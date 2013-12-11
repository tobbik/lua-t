// xt_time.c
struct timeval *xt_time_check_ud  (lua_State *luaVM, int pos);
struct timeval *xt_time_create_ud (lua_State *luaVM);
int             xt_time_new       (lua_State *luaVM);


// helpers
void     xt_time_Add    (struct timeval *tA, struct timeval *tB, struct timeval *tX);
void     xt_time_Sub    (struct timeval *tA, struct timeval *tB, struct timeval *tX);
void     xt_time_Since  (struct timeval *tA);
long     xt_time_Get_ms (struct timeval *tA);
