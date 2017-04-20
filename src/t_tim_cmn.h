/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim_cmn.h
 * \brief     t_tim_* types and unctions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>    // gettimeofday()
#endif


// Constructors
// t_tim.c
struct timeval *t_tim_check_ud ( lua_State *L, int pos, int check );

// helpers
void t_tim_add  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void t_tim_sub  ( struct timeval *tA, struct timeval *tB, struct timeval *tX );
void t_tim_since( struct timeval *tA );
long t_tim_getms( struct timeval *tA );

/**--------------------------------------------------------------------------
 * Compare timeval a to timeval b.
 * reimplements time.h/timercmp to avoid platform incompatibilities.
 * \param  *a  struct timeval pointer
 * \param  *b  struct timeval pointer
 * \param  CMP comparator  (any of ==, <, >)
 * \return 1 if a CMP b is true
 *         0 if a CMP b is false
 * --------------------------------------------------------------------------*/
#define t_tim_cmp( a, b, CMP )        \
	(((a)->tv_sec  ==  (b)->tv_sec)   ?   \
	 ((a)->tv_usec CMP (b)->tv_usec)  :   \
	 ((a)->tv_sec  CMP (b)->tv_sec))

#define t_tim_now( tv, tz ) gettimeofday( tv, tz )
#define t_tim_is( L, pos ) (NULL != t_tim_check_ud( L, pos, 0 ))
