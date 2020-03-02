/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_sck_sht.c
 * \brief     OOP wrapper for network socket shutdown options
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
#define DF_SD( sdn )               \
   lua_pushinteger( L, sdn );      \
   lua_setfield( L, -2, #sdn "" ); \
   lua_pushstring( L, #sdn "" );   \
   lua_rawseti( L, -2, sdn );

#ifdef SHUT_RD
	DF_SD( SHUT_RD );        // No more receptions.
#endif
#ifdef SHUT_WR
	DF_SD( SHUT_WR );        // No more transmissions.
#endif
#ifdef SHUT_RDWR
	DF_SD( SHUT_RDWR );      // No more receptions or transmissions.
#endif

#undef DF_SD

	return 1;
}

