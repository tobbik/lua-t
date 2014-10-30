// xt_time.c
struct timeval *xt_time_check_ud ( lua_State *luaVM, int pos );
struct timeval *xt_time_create_ud( lua_State *luaVM );
int             lxt_time_New     ( lua_State *luaVM );


// helpers
void     xt_time_add  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     xt_time_sub  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     xt_time_since( struct timeval *tA );
long     xt_time_getms( struct timeval *tA );

/**--------------------------------------------------------------------------
 * Compare timeval a to timeval b.
 * \param  *a  struct timeval pointer
 * \param  *b  struct timeval pointer
 * \param  CMP comparator
 * \return 1 if a CMP b is true
 * e       0 if a CMP b is false
 * --------------------------------------------------------------------------*/
#define xt_time_cmp( a, b, CMP )        \
	(((a)->tv_sec  ==  (b)->tv_sec)   ?   \
	 ((a)->tv_usec CMP (b)->tv_usec)  :   \
	 ((a)->tv_sec  CMP (b)->tv_sec))
