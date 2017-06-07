/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_ifs.c
 * \brief     OOP wrapper for network interfaces.  The main limitation here it's
 *            not IPv6 yet
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// this header order makes __USE_MISC visible and hence all the POSIX stuff
#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * Construct a t.Net.Interface and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table t.Net.Interface
 * \lreturn table  t.Net.Interface Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ifc_Get( lua_State *L )
{
	return t_net_ifc_create( L, luaL_checkstring( L, -1 ) );
}


/**--------------------------------------------------------------------------
 * List all available interfaces.
 * \param   L      Lua state.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ifc_List( lua_State *L )
{
	return t_net_ifc_create( L, NULL );
}


/**--------------------------------------------------------------------------
 * Create an t.Net.Interface Lua table and push to LuaStack.
 * \param   L      Lua state.
 * \lreturn table  t.Net.Interface Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_net_ifc_create( lua_State *L, const char *if_name )
{
	return p_net_ifc_get( L, if_name );
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a T.Net.Interface
 * \param   L        Lua state.
 * \param   int      position on the stack.
 * \lparam  table    t.Net.Interface Lua table instance.
 * \lreturn void
 * --------------------------------------------------------------------------*/
void
t_net_ifc_check( lua_State *L, int pos )
{
	luaL_checktype( L, pos, LUA_TTABLE );
   if (lua_getmetatable( L, pos ))            // does it have a metatable?
	{
		luaL_getmetatable( L, T_NET_IFC_TYPE ); // get correct metatable */
		if (! lua_rawequal( L, -1, -2 ))        // not the same?
			t_typeerror( L, pos, T_NET_IFC_TYPE );
		lua_pop( L, 2);
	}
	else
		t_typeerror( L, pos, T_NET_IFC_TYPE );
}


/**--------------------------------------------------------------------------
 * __toString helper to get memory address t.Net.Interface.
 * \param   L      Lua state.
 * \lreturn table  t.Net.Interface Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_ifc_ToString( lua_State *L )
{
	t_net_ifc_check( L, 1 );
	lua_getfield( L, -1, "name" );
	lua_pushfstring( L, T_NET_IFC_TYPE"{%s}: %p", lua_tostring( L, -1 ), lua_topointer( L, -2 ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_ifc_cf [] =
{
	  { "list"       , lt_net_ifc_List }
	, { "get"        , lt_net_ifc_Get }
	, { "tostring"   , lt_net_ifc_ToString }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Pushes the t.Net.Interface library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_net_ifc( lua_State *L )
{
	// T.Net.Interface class
	luaL_newlib( L, t_net_ifc_cf );
	return 1;
}

