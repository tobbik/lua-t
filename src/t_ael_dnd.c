/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_dnd.c
 * \brief     Descriptor node
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#ifdef _WIN32
#include <Windows.h>
#else
#define _POSIX_C_SOURCE 200809L   //fileno()
#endif

//#include <stdlib.h>               // malloc, free
//#include <string.h>               // memset

#include "t_tim.h"
#include "t_net.h"
#include "t_ael_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

/**--------------------------------------------------------------------------
 * Create a new t_ael_evt userdata and push to LuaStack.
 * \param   L    Lua state.
 * \return  evt  struct t_ael_evt * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_ael_dnd
*t_ael_dnd_create_ud( lua_State *L )
{
	struct t_ael_dnd    *dnd;

	dnd = (struct t_ael_dnd *) lua_newuserdata( L, sizeof( struct t_ael_dnd ) );
	dnd->rR     = LUA_REFNIL;
	dnd->wR     = LUA_REFNIL;
	dnd->hR     = LUA_REFNIL;
	dnd->msk    = 0;
	luaL_getmetatable( L, T_AEL_DND_TYPE );
	lua_setmetatable( L, -2 );
	return dnd;
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles.
 * \param   L        Lua state.
 * \param   *dNde    Descriptor node.
 * \param   msk      execute read or write or both.
 * \return  void.
  --------------------------------------------------------------------------*/
void
t_ael_dnd_execute( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk )
{
	int rf = 0;      ///< was read() event fired for this descriptor?
	if (msk & T_AEL_RD & dnd->msk)
	{
#if PRINT_DEBUGS == 1
		//printf( ">>>>> EXECUTE FILE(READ) FOR DESCRIPTOR: %d\n", ael->fdExc[ i ] );
#endif
		t_ael_doFunction( L, dnd->rR, 0 );
		rf = 1;
	}
	if ((msk & T_AEL_WR & dnd->msk) && !rf)
	{
#if PRINT_DEBUGS == 1
		//printf( "<<<<< EXECUTE FILE(WRITE) FOR DESCRIPTOR: %d\n", ael->fdExc[ i ] );
#endif
		t_ael_doFunction( L, dnd->wR, 0 );
	}
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_ael_dnd
 * \param   L      Lua state.
 * \param   int    position on the stack
 * \param   int    check(boolean): if true error out on fail
 * \return  struct t_ael_dnd*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_ael_dnd
*t_ael_dnd_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_AEL_DND_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_AEL_DND_TYPE );
	return (NULL==ud) ? NULL : (struct t_ael_dnd *) ud;
}

/**--------------------------------------------------------------------------
 * Set mask and function reference.
 * \param   L      Lua state.
 * \param   msk    t_ael_msk; execution mask.
 * \param   fR     int; reference for function.
 * \return  void
 * --------------------------------------------------------------------------*/
void
t_ael_dnd_setMaskAndFunction( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk, int fR )
{
	dnd->msk    |= msk;
	if (T_AEL_RD & msk)                //S: ael hdl dir tbl
	{
		if (LUA_REFNIL != dnd->rR)      // if overwriting -> allow for __gc
			luaL_unref( L, LUA_REGISTRYINDEX, dnd->rR );
		dnd->rR = fR;
#if PRINT_DEBUGS == 3
		printf(" ======ADDED MASK(READ): %d(%d) \n", dnd->rR, dnd->msk );
#endif
	}
	else
	{
		if (LUA_REFNIL != dnd->wR)      // if overwriting -> allow for __gc
			luaL_unref( L, LUA_REGISTRYINDEX, dnd->wR );
		dnd->wR = fR;
#if PRINT_DEBUGS == 3
		printf(" ======ADDED MASK(WRITE): %d(%d) \n", dnd->wR, dnd->msk );
#endif
	}
}


/**--------------------------------------------------------------------------
 * Set mask and function reference.
 * \param   L      Lua state.
 * \param   msk    t_ael_msk; execution mask.
 * \param   int    reference for function.
 * \return  void
 * --------------------------------------------------------------------------*/
void
t_ael_dnd_removeMaskAndFunction( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk )
{
	// remove function
	if ((T_AEL_RD & msk & dnd->msk) && LUA_REFNIL != dnd->rR)
	{
#if PRINT_DEBUGS == 3
		printf(" ======REMOVING MASK(READ): %d for %d(%d)\n", dnd->rR, dnd->hR, dnd->msk );
#endif
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->rR );
		dnd->rR = LUA_REFNIL;
	}
	if ((T_AEL_WR & msk & dnd->msk) && LUA_REFNIL != dnd->wR)
	{
#if PRINT_DEBUGS == 3
		printf(" ======REMOVING MASK(WRITE): %d for %d(%d)\n", dnd->wR, dnd->hR, dnd->msk );
#endif
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->wR );
		dnd->wR = LUA_REFNIL;
	}
	// remove from mask
	dnd->msk = dnd->msk & (~msk);
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Free events in allocated spots.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \return  int  # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_ael_dnd__gc( lua_State *L )
{
	struct t_ael_dnd *dnd  = t_ael_dnd_check_ud( L, 1, 1 );

	// TODO: this makes no sense!!! if a value is NOT LUA_REFNIL it should be
	// unrefed. However, changing this messes with everything!!! BANG MY HEAD
	if (LUA_REFNIL == dnd->hR)
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->hR );
	if (LUA_REFNIL == dnd->rR)
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->rR );
	if (LUA_REFNIL == dnd->wR)
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->wR );
	return 0;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_dnd_fm [] = {
	  { NULL,   NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_ael_dnd_cf [] = {
	{ NULL,  NULL }
};

/**--------------------------------------------------------------------------
 * Instance metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_dnd_m [] = {
	// metamethods
	  { "__gc",          lt_ael_dnd__gc }
	, { NULL,   NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Loop library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn string    the library
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_ael_dnd( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_AEL_DND_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_ael_dnd_m, 0 );

	// Push the class onto the stack
	luaL_newlib( L, t_ael_dnd_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_ael_dnd_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
