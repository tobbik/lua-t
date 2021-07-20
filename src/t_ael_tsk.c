/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael_tsk.c
 * \brief     Async loop task
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_net.h"
#include "t_ael_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


// helpers
/**--------------------------------------------------------------------------
 * Adjust amount of time in the loops timer event list.
 * Substract tAdj value from each timer in the list.
 * \param   L        Lua state.
 * \lparam  *ael     t_ael; pointer to loop.
 * \lparam  tskHead  t_ael_tsk; Head of task list on stack.
 * \param     tAdj   int; time(ms) value to be substracted from all nodes in
 *                   tasks linked list.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_tsk_adjust( lua_State *L, lua_Integer tAdj )
{
	struct t_ael_tsk *tRun;
	lua_pushvalue( L, -1 );     // copy task head    //S: ael hed hed
	while (! lua_isnil( L, -1 ))                     //S: ael hed tsk
	{
		tRun       = t_ael_tsk_check_ud( L, -1, 0 );
		//printf("ADJUST: %lld  %lld  -- ", tRun->tout, tAdj);
		tRun->tout = (tRun->tout - tAdj > 0) ? tRun->tout - tAdj : 0;
		//printf("%lld  -- ", tRun->tout); t_stackDump(L);
		lua_getiuservalue( L, -1, T_AEL_TSK_NXTIDX ); //S: ael hed tsk tsk
		lua_remove( L, -2);                           //S: ael hed tsk
	}                                                //S: ael hed tsk
	lua_pop( L, 1 );                                 //S: ael hed
}


/**--------------------------------------------------------------------------
 * Execute single task.
 * \param   L        Lua state.
 * \lparam  *ael     t_ael; pointer to loop.
 * \lparam  *tsk     t_ael_tsk; pointer to task to execute.
 * \return  void                                                             …
 * --------------------------------------------------------------------------*/
static void
t_ael_tsk_execute( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tsk )
{
	lua_Integer        ms = 0;     ///< 0 means sentinel to remove from loop

	lua_getiuservalue( L, -1, T_AEL_TSK_FNCIDX );                //S: ael tsk tbl
	t_ael_doFunction( L, 1 );                                    //S: ael tsk ms
	ms = (lua_isinteger( L, -1 )) ? lua_tointeger( L, -1 ) : ms; //S: ael tsk ms
	lua_pop( L, 1 );  // pop the nil or millisecond value        //S: ael tsk
	//printf("EXEC REMOVE: "); t_stackDump(L);
	t_ael_tsk_remove( L, ael, tsk );                             //S: ael tsk

	if (ms < 1 )      // remove from list
		lua_pop( L, 1 );                                          //S: ael
	else              // re-add node to list if function returned a timer
	{
		tsk->tout = ms;                                           //S: ael tsk
		//printf("EXEC INSERT: "); t_stackDump(L);
		t_ael_tsk_insert( L, ael, tsk );                          //S: ael
	}
}


/**--------------------------------------------------------------------------
 * Pop timer list head, execute and re-add if needed.
 * \param   L        Lua state.
 * \lparam  ael      t_ael; the Loop userdata.
 * \param   et       int; execution time in milliseconds
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_tsk_process( lua_State *L, struct t_ael *ael, lua_Integer et )
{
	struct t_ael_tsk *tRun;                               //S: ael
	lua_getiuservalue( L, -1, T_AEL_TSKIDX );             //S: ael hed
	//printf("PROC ADJUST: "); t_stackDump(L);
	t_ael_tsk_adjust( L, et );                            //S: ael hed

	//printf("PROC ADJUSTed: "); t_stackDump(L);
	while (! lua_isnil( L, -1 ))                          //S: ael hed
	{
		tRun = t_ael_tsk_check_ud( L, -1, 0 );
		if (tRun->tout < 1)
		{
			//printf("PROC EXECUTE: "); t_stackDump(L);
			t_ael_tsk_execute( L, ael, tRun );              //S: ael
		}
		else
			break;
		lua_getiuservalue( L, -1, T_AEL_TSKIDX );          //S: ael hed
		//printf("PROC NEXT: "); t_stackDump(L);
	}                                                     //S: ael hed
	//printf("PROC FINALIZE: "); t_stackDump(L);
	lua_pop( L, 1 );                                      //S: ael
}


/**----------------------------------------------------------------------------
 * Slot in a task into the loops linked list of tasks.
 * Ordered insert; walks down linked list and inserts before the next bigger
 * task node.
 * \param    L      Lua state.
 * \lparam  *ael    t_ael; the Loop userdata.
 * \lparam  *tIns   t_ael_tsk; task to be inserted into linked list.
 * \lreturn *ael    t_ael; the Loop userdata.
 * \return   void.
 * --------------------------------------------------------------------------*/
void
t_ael_tsk_insert( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tIns )
{
	struct t_ael_tsk *tRun;                                    //S: ael ins

	lua_getiuservalue( L, -2, T_AEL_TSKIDX );                  //S: ael ins hed
	if (lua_isnil( L, -1 ))                                    //S: ael ins nil
	{
		//printf("INSERT NEW HEAD: "); t_stackDump(L);
		lua_pop( L, 1 );                                        //S: ael ins (is now head)
		lua_setiuservalue( L, -2, T_AEL_TSKIDX );               //S: ael
		ael->tout = tIns->tout;
	}
	else
	{
		tRun = t_ael_tsk_check_ud( L, -1, 0 );                  //S: ael ins hed
		if (tRun->tout >= tIns->tout)  // ins becomes new head; prev head links to ins
		{
			//printf("INSERT HEAD: %lld %lld-", tRun->tout, tIns->tout); t_stackDump(L);
			lua_setiuservalue( L, -2, T_AEL_TSK_NXTIDX );        //S: ael ins (is now head)
			lua_setiuservalue( L, -2, T_AEL_TSKIDX );            //S: ael
			ael->tout = tIns->tout;
		}
		else
		{
			while (LUA_TNIL != lua_getiuservalue( L, -1, T_AEL_TSK_NXTIDX ))
			{
				tRun = t_ael_tsk_check_ud( L, -1, 0 );            //S: ael ins prv nxt
				//printf("ud->num: %d  %p ", ud->num, ud); t_stackDump(L);
				if (tRun->tout >= tIns->tout)
					break;
				lua_remove( L, -2 );                              //S: ael ins nxt
			}
			//printf("INSERT TASK: %lld -", tRun->tout);t_stackDump(L);
			                                                     //S: ael ins prv nxt/nil
			lua_setiuservalue( L, -3, T_AEL_TSK_NXTIDX );        //S: ael ins prv
			lua_rotate( L, -2, 1 );                              //S: ael prv ins
			lua_setiuservalue( L, -2, T_AEL_TSK_NXTIDX );        //S: ael prv
			lua_pop( L, 1 );                                     //S: ael
		}
	}
}


/**----------------------------------------------------------------------------
 * Remove a task from the loops linked list of tasks.
 * \param    L      Lua state.
 * \lparam  *ael    t_ael; the Loop userdata.
 * \lparam  *cnd    t_ael_tsk; Candidate to be removed.
 * \lreturn *ael    t_ael; the Loop userdata.
 * \return   void.
 * --------------------------------------------------------------------------*/
void
t_ael_tsk_remove( lua_State *L, struct t_ael *ael, struct t_ael_tsk *tCnd )
{
	struct t_ael_tsk *tRun;

	lua_getiuservalue( L, -2, T_AEL_TSKIDX );             //S: ael cnd hed
	if (lua_isnil( L, -1 )) // head is nil, cop out
	{ // clean nxt from cnd for good measure
		lua_setiuservalue( L, -2, T_AEL_TSK_NXTIDX );      //S: ael cnd
		return;              // no tasks in list
	}
	tRun = t_ael_tsk_check_ud( L, -1, 0 );                //S: ael cnd hed
	lua_getiuservalue( L, -1, T_AEL_TSK_NXTIDX );         //S: ael cnd hed nxt
	if (tCnd == tRun)      // head is the candidate
	{                      // link next as head
		//printf("REMOVE HEAD: %lld -", tRun->tout);t_stackDump(L);
		if (lua_isnil( L, -1 ))                            //S: ael cnd hed nil
			ael->tout = T_AEL_NOTIMEOUT;
		else
		{
			tRun = t_ael_tsk_check_ud( L, -1, 0 );          //S: ael cnd hed nxt
			ael->tout = tRun->tout;
		}
		lua_setiuservalue( L, -4, T_AEL_TSKIDX );          //S: ael cnd hed
		lua_pop( L, 1 );                                   //S: ael cnd
	}
	else
	{
		while (! lua_isnil( L, -1 ))                       //S: ael cnd prv nxt
		{
			tRun = t_ael_tsk_check_ud( L, -1, 0 );          //S: ael cnd prv run
			lua_getiuservalue( L, -1, T_AEL_TSK_NXTIDX );   //S: ael cnd prv run nxt
			if (tCnd == tRun)
			{                // link nxt to prv
				lua_setiuservalue( L, -3, T_AEL_TSK_NXTIDX );//S: ael cnd prv run
				break;
			}
			else
				lua_remove( L, -3 );                         //S: ael cnd run nxt
		}
		lua_pop( L, 2 );                                   //S: ael cnd
	}
	// remove "nxt" from "cnd"
	lua_pushnil( L );                                     //S: ael cnd nil
	lua_setiuservalue( L, -2, T_AEL_TSK_NXTIDX );         //S: ael cnd
}


/**--------------------------------------------------------------------------
 * Create a new t_ael_ts userdata and push to LuaStack.
 * \param   L    Lua state.
 * \return  tsk  struct t_ael_tsk * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_ael_tsk
*t_ael_tsk_create_ud( lua_State *L, lua_Integer ms )
{
	struct t_ael_tsk    *tsk;

	tsk = (struct t_ael_tsk *) lua_newuserdatauv( L, sizeof( struct t_ael_tsk ), 2 );
	tsk->tout   = ms;
	luaL_getmetatable( L, T_AEL_TSK_TYPE );
	lua_setmetatable( L, -2 );
	return tsk;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_ael_tsk
 * \param   L      Lua state.
 * \param   int    position on the stack
 * \param   int    check(boolean): if true error out on fail
 * \return  struct t_ael_tsk*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_ael_tsk
*t_ael_tsk_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_AEL_TSK_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_AEL_TSK_TYPE );
	return (NULL==ud) ? NULL : (struct t_ael_tsk *) ud;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Remove references.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop.Task userdata instance.                   // 1
 * \return  int  # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_ael_tsk__gc( lua_State *L )
{
	struct t_ael_tsk __attribute__ ((unused)) *tsk  = t_ael_tsk_check_ud( L, 1, 1 );

	//printf( "__GCing: %lldms task\n", tsk->tout );
	return 0;
}

/**--------------------------------------------------------------------------
 * Prints the Task.
 * \param   L      Lua state.
 * \lparam  ud     T.Loop.Task userdata instance.                       // 1
 * \lreturn string formatted string representing T.Loop.Task.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_tsk__tostring( lua_State *L )
{
	struct t_ael_tsk *tsk = t_ael_tsk_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_AEL_TSK_TYPE"{%dms}: %p", tsk->tout, tsk );
	return 1;
}

/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_tsk_fm [] = {
	  { NULL,   NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_ael_tsk_cf [] = {
	  { NULL,  NULL }
};

/**--------------------------------------------------------------------------
 * Instance metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_tsk_m [] = {
	// metamethods
	  { "__gc",          lt_ael_tsk__gc          }
	, { "__tostring",    lt_ael_tsk__tostring    }
	, { NULL,            NULL                    }
};


/**--------------------------------------------------------------------------
 * Pushes the Loop.Task library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn string    the library
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_ael_tsk( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_AEL_TSK_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_ael_tsk_m, 0 );

	// Push the class onto the stack
	luaL_newlib( L, t_ael_tsk_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_ael_tsk_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
