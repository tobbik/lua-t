/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tim.h
 * \brief     data types for timeval (T.Time) related data types
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


//  _____                      
// |_   _|   _ _ __   ___  ___ 
//   | || | | | '_ \ / _ \/ __|
//   | || |_| | |_) |  __/\__ \
//   |_| \__, | .__/ \___||___/
// 		|___/|_|              

// there is NO "struct t_tim" type because it is perfectly fine to reuse
// struct timeval from time.h



// Constructors
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
 * \param  CMP comparator  (any of ==, <, >)
 * \return 1 if a CMP b is true
 * e       0 if a CMP b is false
 * --------------------------------------------------------------------------*/
#define t_tim_cmp( a, b, CMP )        \
	(((a)->tv_sec  ==  (b)->tv_sec)   ?   \
	 ((a)->tv_usec CMP (b)->tv_usec)  :   \
	 ((a)->tv_sec  CMP (b)->tv_sec))
