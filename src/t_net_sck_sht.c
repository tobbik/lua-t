/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck_sht.c
 * \brief     OOP wrapper for network socketshutdown options
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Class library function definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_sck_sht_cf [] =
{
	  { NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket Shutdown options library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_net_sck_sht( lua_State *L )
{
	luaL_newlib( L, t_net_sck_sht_cf );

#ifdef SHUT_RD
	lua_pushinteger( L, SHUT_RD );        // No more receptions.
	lua_setfield( L, -2, "SHUT_RD" );
#endif
#ifdef SHUT_WR
	lua_pushinteger( L, SHUT_WR );        // No more transmissions.
	lua_setfield( L, -2, "SHUT_WR" );
#endif
#ifdef SHUT_RDWR
	lua_pushinteger( L, SHUT_RDWR );      // No more receptions or transmissions.
	lua_setfield( L, -2, "SHUT_RDWR" );
#endif

	return 1;
}

