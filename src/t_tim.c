/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      src/t_tim.c
 * \brief     t_tim_* functions available to all t_* modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_tim_l.h"
#include "t.h"            // t_typeerror

#ifdef DEBUG
#include "t_dbg.h"
#endif

/**--------------------------------------------------------------------------
 * Sets tA to time difference between tA and now
 * \param  *tv struct timeval pointer
 * --------------------------------------------------------------------------*/
void
t_tim_since( struct timeval *tA )
{
	struct timeval tC;

	gettimeofday( &tC, 0 );
	timersub( &tC, tA, tA );
}


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////

/**--------------------------------------------------------------------------
 * Create an timer userdata and push to LuaStack.
 * \param   L      Lua state.
 * \return  struct timeval* pointer to userdata at Stack position.
 * --------------------------------------------------------------------------*/
struct timeval
*t_tim_create_ud( lua_State *L, struct timeval *tvr )
{
	struct timeval *tv;

	tv = (struct timeval *) lua_newuserdata( L, sizeof( struct timeval ) );
	tv->tv_sec  = (NULL != tvr) ? tvr->tv_sec  : 0;
	tv->tv_usec = (NULL != tvr) ? tvr->tv_usec : 0;

	luaL_getmetatable( L, T_TIM_TYPE );
	lua_setmetatable( L, -2 );
	return tv;
}


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

