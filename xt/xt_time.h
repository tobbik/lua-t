// xt_time.c
struct timeval *xt_time_check_ud ( lua_State *luaVM, int pos );
struct timeval *xt_time_create_ud( lua_State *luaVM );
int             lxt_time_New     ( lua_State *luaVM );


// helpers
void     xt_time_add  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     xt_time_sub  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     xt_time_since( struct timeval *tA );
long     xt_time_getms( struct timeval *tA );
