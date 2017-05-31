/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_cmn.c
 * \brief     t_net_* functions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_tim_l.h"
#include "t.h"            // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Adds timeval tB to timeval tA and puts value into tX.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * \param  *tX struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_add( struct timeval *tA, struct timeval *tB, struct timeval *tX )
{
	struct timeval tC;

	tC.tv_sec    = tB->tv_sec  + tA->tv_sec ;  // add seconds
	tC.tv_usec   = tB->tv_usec + tA->tv_usec ; // add microseconds
	tC.tv_sec   += tC.tv_usec / 1000000 ;      // add microsecond overflow to seconds
	tC.tv_usec  %= 1000000 ;                   // subtract overflow from microseconds
	tX->tv_sec   = tC.tv_sec;
	tX->tv_usec  = tC.tv_usec;
}


/**--------------------------------------------------------------------------
 * Substract timeval tB from timeval tA and puts value into tX.
 * \param  *tA struct timeval pointer
 * \param  *tB struct timeval pointer
 * \param  *tX struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_sub( struct timeval *tA, struct timeval *tB, struct timeval *tX )
{
	struct timeval tC;

	tC.tv_sec    = (tB->tv_usec > tA->tv_usec)
		? tA->tv_sec - tB->tv_sec - 1
		: tA->tv_sec - tB->tv_sec;
		;
	tC.tv_usec   = tA->tv_usec;
	tC.tv_usec   = (tB->tv_usec > tA->tv_usec)
		? 1000000 - (tB->tv_usec - tA->tv_usec)
		: tA->tv_usec - tB->tv_usec;
	tX->tv_sec   = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 0 : tC.tv_sec;
	tX->tv_usec  = (tC.tv_sec < 0 ||tC.tv_usec < 0)? 1 : tC.tv_usec;
}


/**--------------------------------------------------------------------------
 * Sets tA to time different between tA and now
 * \param  *tv struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_since( struct timeval *tA )
{
	struct timeval tC;

	t_tim_now( &tC, 0 );
	t_tim_sub( &tC, tA, tA );
}


/**--------------------------------------------------------------------------
 * Gets milliseconds worth of timeval.
 * \param  struct timeval *t pointer.
 * \return long            value in milliseconds.
 * --------------------------------------------------------------------------*/
long
t_tim_getms( struct timeval *t )
{
	return t->tv_sec*1000 + t->tv_usec/1000;
}


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct timeval.
 * \param   L      Lua state.
 * \param   int    Position on the stack
 * \param   int    check(boolean): if true error out on fail
 * \return  struct timeval* pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_TIM_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_TIM_TYPE );
	return (NULL==ud) ? NULL : (struct timeval *) ud;
}

