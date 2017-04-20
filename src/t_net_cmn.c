/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_cmn.c
 * \brief     t_net_* functions shared between all modules
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t.h"
#include "t_net_cmn.h"

/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_net.
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \return  struct t_net_sck*  pointer to the struct t_net_sck.
 * --------------------------------------------------------------------------*/
struct t_net_sck
*t_net_sck_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_SCK_TYPE );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`"T_NET_SCK_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_net_sck *) ud;
}



