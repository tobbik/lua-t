// t_tim.c
struct timeval *t_tim_check_ud ( lua_State *luaVM, int pos, int check );
struct timeval *t_tim_create_ud( lua_State *luaVM );


// helpers
void     t_tim_add  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     t_tim_sub  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void     t_tim_since( struct timeval *tA );
long     t_tim_getms( struct timeval *tA );

/**--------------------------------------------------------------------------
 * Compare timeval a to timeval b.
 * \param  *a  struct timeval pointer
 * \param  *b  struct timeval pointer
 * \param  CMP comparator
 * \return 1 if a CMP b is true
 * e       0 if a CMP b is false
 * --------------------------------------------------------------------------*/
#define t_tim_cmp( a, b, CMP )        \
	(((a)->tv_sec  ==  (b)->tv_sec)   ?   \
	 ((a)->tv_usec CMP (b)->tv_usec)  :   \
	 ((a)->tv_sec  CMP (b)->tv_sec))
