/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.c
 * \brief     OOP wrapper for an asyncronous event loop (T.Loop)
 *            This covers the generic functions which are usable accross
 *            specific implementations
 *            They mainly handle the creation of data structures, their
 *            bindings to the Lua state and the list/collection operations
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#ifdef _WIN32
#include <Windows.h>
#else
#define _POSIX_C_SOURCE 200809L   //fileno()
#endif

#include <stdlib.h>               // malloc, free
#include <string.h>               // memset

#include "t_tim.h"
#include "t_net.h"
#include "t_ael_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**----------------------------------------------------------------------------
 * Get descriptor handle from the stack.
 * Ordered insert; walks down linked list and inserts before the next bigger
 * *timer node.
 * \param   **tHead    t_ael_tnd; pointer to Head of timer linked list pointer.
 * \param    *tIns     t_ael_tnd; Timer Node to be inserted in linked list.
 * \return    void.
 * --------------------------------------------------------------------------*/
static inline int
t_ael_getHandle( lua_State *L, int pos, int check )
{
	struct t_net_sck *sck = t_net_sck_check_ud( L, pos, 0 );
	luaL_Stream      *lS;
	int               fd  = 0;

	if (NULL != sck)
		fd = sck->fd;
	else
	{
		// this is less likely -> else path
		lS = (luaL_Stream *) luaL_testudata( L, pos, LUA_FILEHANDLE );
		if (NULL != lS)
			fd = fileno( lS->f );
	}

	luaL_argcheck( L, !check || fd != -1, pos, "descriptor mustn't be closed" );
	luaL_argcheck( L, !check || fd > 0  , pos, "Expected a Lua file or t.Net.Socket" );
	return fd;
}


/**----------------------------------------------------------------------------
 * Slot in a timer event into the loops timer event list.
 * Ordered insert; walks down linked list and inserts before the next bigger
 * *timer node.
 * \param   **tHead    t_ael_tnd; pointer to Head of timer linked list pointer.
 * \param    *tIns     t_ael_tnd; Timer Node to be inserted in linked list.
 * \return    void.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_insertTimer( struct t_ael_tnd **tHead, struct t_ael_tnd *tIns )
{
	struct t_ael_tnd *tRun;

	if (NULL == *tHead || t_tim_cmp( (*tHead)->tv, tIns->tv, >= ))
	{
#if PRINT_DEBUGS == 2
		printf( "Make HEAD   {%2ld:%6ld}\t PRE{%2ld:%6ld}\n",
			tIns->tv->tv_sec,  tIns->tv->tv_usec,
			(NULL != (*tHead)) ? (*tHead)->tv->tv_sec  : 0,
			(NULL != (*tHead)) ? (*tHead)->tv->tv_usec : 0);
#endif
		tIns->nxt = *tHead;
		*tHead    =  tIns;
	}
	else
	{
		tRun = *tHead;
		while (NULL != tRun->nxt && t_tim_cmp( tRun->nxt->tv, tIns->tv, < ))
			tRun = tRun->nxt;
#if PRINT_DEBUGS == 2
		printf( "Make NODE   {%2ld:%6ld}\tPAST{%2ld:%6ld}\n",
			tIns->tv->tv_sec,  tIns->tv->tv_usec,
			tRun->tv->tv_sec,  tRun->tv->tv_usec);
#endif
		tIns->nxt = tRun->nxt;
		tRun->nxt = tIns;
	}
}


/**--------------------------------------------------------------------------
 * Adjust amount of time in the loops timer event list.
 * Substract tAdj value from each timer in the list.
 * \param   **tHead  t_ael_tnd; pointer to Head of timer linked list pointer.
 * \param     tAdj   timeval; time value to be substracted from all nodes in
 *                   timer linked list.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_adjustTimers( struct t_ael_tnd **tHead, struct timeval *tAdj )
{
	struct t_ael_tnd *tRun = *tHead;
	if (0==tAdj->tv_sec && 0==tAdj->tv_usec) return;

	while (NULL != tRun)
	{
#if PRINT_DEBUGS == 3
		printf( "  ooooooo ADJUSTING: {%2ld:%6ld}  by  {%2ld:%6ld}  ",
		       tRun->tv->tv_sec, tRun->tv->tv_usec, tAdj->tv_sec, tAdj->tv_usec );
#endif
		t_tim_sub( tRun->tv, tAdj, tRun->tv ); // if tAdj > tRun->tv t_tim_sub( )
		                                       // adjusts tRun->tv to { 0, 0 }

#if PRINT_DEBUGS == 3
		printf( "  -> {%2ld:%6ld}\n", tRun->tv->tv_sec, tRun->tv->tv_usec );
#endif
		tRun = tRun->nxt;
	}
}


/**----------------------------------------------------------------------------
 * Takes refPosition and gets table onto the stack.  Executes function.
 * Stack before: {fnc, p1, p2, p3, … }
 * Stack after:   fnc  p1  p2  p3  …
 * \param   L     Lua state.
 * \param   pos   int; Reference positon on the stack.
 * \param   exc   int; Reference positon.
 * \return  int   Number of arguments to be called by function.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_doFunction( lua_State *L, int pos, int exc )
{
	int n;      ///< number of arguments + 1(function)
	int p;      ///< position of pickled function and args
	int i;

	if (LUA_REFNIL != pos)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, pos );
		p = lua_gettop( L );
		n = lua_rawlen( L, p );
		for (i=0; i<n; i++)
			lua_rawgeti( L, p, i+1 );
		lua_remove( L, p );       // remove table with pickled stack
		if (exc > -1)
			lua_call( L, n-1, exc );
	}
}


/**--------------------------------------------------------------------------
 * Execute single timer node.
 * \param   L        Lua state.
 * \param   *tmNde   t_ael_tnd; pointer to timer to execute.
 * \return  *tmNde   Returns timer node if new time is put in loop, else NULL.
 * --------------------------------------------------------------------------*/
struct t_ael_tnd
*t_ael_executeTimerNode( lua_State *L, struct t_ael_tnd **tmHead, struct t_ael_tnd *tnd )
{
	struct timeval   *tv;              ///< timer returned by execution -> if there is one

	t_ael_doFunction( L, tnd->fR, 1 );
	tv = t_tim_check_ud( L, -1, 0 );
	if (NULL == tv)                    // remove from list
	{
		luaL_unref( L, LUA_REGISTRYINDEX, tnd->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tnd->tR );
		free( tnd );
		tnd = NULL;
	}
	else              // re-add node to list if function returned a timer
	{
		// re-use timer node which still has function and parameters pickled
		// but use new tv reference if tv wasn't reused
		if (tv != tnd->tv)
		{
			luaL_unref( L, LUA_REGISTRYINDEX, tnd->tR );
			tnd->tR = luaL_ref( L, LUA_REGISTRYINDEX );
			tnd->tv = tv;
		}
		t_ael_insertTimer( tmHead, tnd );
	}
	lua_pop( L, 1 );   // pop the one value that lua_call allows for being returned
	return tnd;
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles.
 * \param   L        Lua state.
 * \param   *dNde    Descriptor node.
 * \param   msk      execute read or write or both.
 * \return  void.
  --------------------------------------------------------------------------*/
void
t_ael_executehandle( lua_State *L, struct t_ael_dnd *dnd, enum t_ael_msk msk )
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
 * Pop timer list head, execute and re-add if needed.
 * \param   L        Lua state.
 * \param   **tHead  t_ael_tnd; pointer to Head of timer linked list pointer.
 * \param   *rt      struct timeval; measure how long since last poll fell through
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_processTimers( lua_State *L, struct t_ael_tnd **tmHead, struct timeval *rt )
{
	struct t_ael_tnd *tmRun = *tmHead;   ///< timer to execute is tHead, ALWAYS!
	struct t_ael_tnd __attribute__ ((unused)) *tmExc;

	t_ael_adjustTimers( tmHead, rt );

	// execute every timer that has a value lesser than {0:001000} (1ms)
	// using 1ms removes jitter
	while (tmRun && 0==tmRun->tv->tv_sec && tmRun->tv->tv_usec < 1000)
	{
		*tmHead = (*tmHead)->nxt;           // forward head counter
		tmExc   = t_ael_executeTimerNode( L, tmHead, tmRun );
		//if (tmExc) ...
		//t_tim_since( rt );              // TODO: measure execution time
		tmRun = *tmHead;
	}
}


/**--------------------------------------------------------------------------
 * Construct a T.Loop and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table t.Loop.
 * \lparam  sz,fix int,bool; descriptor capacity of Loop or automatic mode.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_ael__Call( lua_State *L )
{
	struct t_ael __attribute__ ((unused)) *ael;
	lua_remove( L, 1 );   // remove CLASS table

	ael = t_ael_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new t_ael userdata and push to LuaStack.
 * \param   L    Lua state.
 * \param   sz   size_t; how many slots for file/socket events to be created.
 * \return  ael  struct t_ael * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_ael
*t_ael_create_ud( lua_State *L )
{
	struct t_ael    *ael;

	ael = (struct t_ael *) lua_newuserdata( L, sizeof( struct t_ael ) );
	ael->tmHead  = NULL;
	ael->fdCount = 0;
	lua_newtable( L );
	ael->dR = luaL_ref( L, LUA_REGISTRYINDEX );
	ael->sR = p_ael_create_ud_impl( L );
	luaL_getmetatable( L, T_AEL_TYPE );
	lua_setmetatable( L, -2 );
	return ael;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_ael
 * \param   L      Lua state.
 * \param   int    position on the stack
 * \param   int    check(boolean): if true error out on fail
 * \return  struct t_ael*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_ael
*t_ael_check_ud ( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_AEL_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_AEL_TYPE );
	return (NULL==ud) ? NULL : (struct t_ael *) ud;
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the T.Loop.
 * \param   L      Lua state.
 * \lparam  ud     T.Loop userdata instance.                          // 1
 * \lparam  ud     T.Net.Socket or LUA_FILEHANDLE userdata instance.  // 2
 * \lparam  string r,rd,read incoming, w,wr,write outgoing            // 3
 * \lparam  func   to be executed when event handler fires.           // 4
 * \lparam  …      parameters to function when executed.              // 5 …
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_addhandle( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	int               fd  = t_ael_getHandle( L, 2, 1 );
	struct t_ael_dnd *dnd;
	int               n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	enum t_ael_msk    msk;

	luaL_argcheck( L, NULL != t_getTypeByName( L, 3, NULL, t_ael_directionList ),
	      3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );
	luaL_checktype( L, 4, LUA_TFUNCTION );

	// get/create dnd userdata
	lua_rawgeti( L, LUA_REGISTRYINDEX, ael->dR );
	lua_rawgeti( L, -1, fd );
	if (lua_isnil( L, -1 ))
	{
		dnd = t_ael_dnd_create_ud( L );          //S: ael hnd dir fnc … nds nil dnd
		lua_rawseti( L, -3, fd );
		(ael->fdCount)++;
	}
	else
		dnd = t_ael_dnd_check_ud( L, -1, 1 );    //S: ael hnd dir fnc … nds dnd
	lua_pop( L, 2 );   // pop nil/dnd and nodes //S: ael hnd dir fnc …

	// implementatuion specific handling
	p_ael_addhandle_impl( L, ael, dnd, fd, msk );

	// create function reference
	lua_createtable( L, n-4, 0 );               // create function/parameter table
	lua_insert( L, 4 );                         //S: ael hdl dir tbl fnc …
	while (n > 4)
		lua_rawseti( L, 4, (n--)-4 );            // add arguments and function (pops each item)

	t_ael_dnd_setMaskAndFunction( L, dnd, msk, luaL_ref( L, LUA_REGISTRYINDEX ) );

	lua_pop( L, 1 );                            // pop the read/write indicator string/integer
	if (LUA_REFNIL == dnd->hR)
	{
		dnd->hR = luaL_ref( L, LUA_REGISTRYINDEX );      // keep ref to handle so it doesnt gc
#if PRINT_DEBUGS == 3
		printf(" ======ADDED HANDLE(SOCKET): %d(%d) \n", dnd->hR, fd );
#endif
	}

	lua_pushboolean( L, 1 );
	return  1;
}


/**--------------------------------------------------------------------------
 * Remove a Handle event handler from the T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                          // 1
 * \lparam  ud   T.Net.Socket or LUA_FILEHANDLE userdata instance.  // 2
 * \lparam  bool true stop observe incoming; else outgoing?         // 3
 * \return  int  # of values pushed onto the stack.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int
lt_ael_removehandle( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	int                fd = t_ael_getHandle( L, 2, 1 );
	struct t_ael_dnd *dnd;
	enum t_ael_msk    msk;

	luaL_argcheck( L, NULL != t_getTypeByName( L, 3, NULL, t_ael_directionList ),
	      3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );

	// get/create dnd userdata
	lua_rawgeti( L, LUA_REGISTRYINDEX, ael->dR );
	lua_rawgeti( L, -1, fd );
	if (lua_isnil( L, -1 ))
		return luaL_error( L, "Descriptor must be observed in Loop" );
	else
		dnd = t_ael_dnd_check_ud( L, -1, 1 );    //S: ael hnd dir nds dnd
	lua_pop( L, 1 );   // pop dnd               //S: ael hnd dir nds

	p_ael_removehandle_impl( L, ael, dnd, fd, msk );

	t_ael_dnd_removeMaskAndFunction( L, dnd, msk );

	if (T_AEL_NO == dnd->msk )
	{
#if PRINT_DEBUGS == 3
		printf(" ======REMOVING HANDLE(SOCKET): %d \n", dnd->hR );
#endif
		luaL_unref( L, LUA_REGISTRYINDEX, dnd->hR );
		lua_pushnil( L );
		lua_rawseti( L, -2, fd );
		(ael->fdCount)--;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Add a Timer event handler to the T.Loop.
 * \param   L   Lua state.
 * \lparam  ud  T.Loop userdata instance.                   // 1
 * \lparam  ud  T.Time userdata instance.                   // 2
 * \lparam  fnc to be executed when event handler fires.    // 3
 * \lparam  …   parameters to function when executed.       // 4 …
 * \return  int # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_addtimer( lua_State *L )
{
	struct t_ael     *ael  = t_ael_check_ud( L, 1, 1 );
	struct timeval   *tv   = t_tim_check_ud( L, 2, 1 );
	int               n    = lua_gettop( L ) + 1;    ///< iterator for arguments
	int               r    = 0;                      ///< replace, don't create
	struct t_ael_tnd *tnd;
	struct t_ael_tnd *tRun = ael->tmHead;

	luaL_checktype( L, 3, LUA_TFUNCTION );

	// try to find the timer in the list first and overwrite instead of re-add
	while (NULL != tRun && NULL != tRun->nxt && tRun->tv != tv)
		tRun = tRun->nxt;
	if (tRun && tv == tRun->tv)
	{
		tnd = tRun;
		luaL_unref( L, LUA_REGISTRYINDEX, tnd->fR ); // remove old ref -> no leaks
		r = 1;
	}
	else
	{
		// Build up the timer element
		tnd     = (struct t_ael_tnd *) malloc( sizeof( struct t_ael_tnd ) );
		tnd->tv = tv;
	}
	//p_ael_addtimer_impl( ael, tv );
	lua_createtable( L, n-3, 0 );  // create function/parameter table
	lua_insert( L, 3 );
	// Stack: ael,tv,tbl,clb,…
	while (n > 3)
		lua_rawseti( L, 3, (n--)-3 );             // add args and callback (pops each item)
	tnd->fR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the function/parameter table
	if (! r)
	{
		// making the time val part of lua registry guarantees the gc can't destroy it
		tnd->tR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the timeval
		// insert into ordered linked list of time events
		t_ael_insertTimer( &(ael->tmHead), tnd );
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a Timer event handler from the T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \lparam  ud   T.Time userdata instance.                   // 2
 * \return  int  # of values pushed onto the stack.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int
lt_ael_removetimer( lua_State *L )
{
	struct t_ael     *ael   = t_ael_check_ud( L, 1, 1 );
	struct timeval   *tv    = t_tim_check_ud( L, 2, 1 );
	struct t_ael_tnd *tCnd  = ael->tmHead;
	struct t_ael_tnd *tRun  = tCnd->nxt;       ///< previous Timer event

	// if head is node in question
	if (NULL != tCnd  &&  tCnd->tv == tv)
	{
		ael->tmHead = tRun;
		luaL_unref( L, LUA_REGISTRYINDEX, tCnd->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tCnd->tR );
		free( tCnd );
		return 0;
	}
	// walk down the linked list
	while (NULL != tRun && tRun->tv != tv)
	{
		tCnd = tRun;
		tRun = tRun->nxt;
	}
	// Last or found timer
	if (NULL!=tRun && tRun->tv == tv)
	{
		tCnd->nxt = tRun->nxt;
		luaL_unref( L, LUA_REGISTRYINDEX, tRun->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tCnd->tR );
		free( tRun );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Free events in allocated spots.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \return  int  # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_ael__gc( lua_State *L )
{
	struct t_ael     *ael  = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tnd *tFre;
	struct t_ael_tnd *tRun = ael->tmHead;

	while (NULL != tRun)
	{
		tFre = tRun;
		luaL_unref( L, LUA_REGISTRYINDEX, tFre->fR ); // remove func/arg table from registry
		luaL_unref( L, LUA_REGISTRYINDEX, tFre->tR ); // remove timeval ref from registry
		tRun = tRun->nxt;
		free( tFre );
	}
	// walk down dR table an unref functions and handles
	p_ael_free_impl( L, ael->sR );
	luaL_unref( L, LUA_REGISTRYINDEX, ael->sR ); // unref impl state data
	luaL_unref( L, LUA_REGISTRYINDEX, ael->dR ); // unref nodes table
	return 0;
}


/**--------------------------------------------------------------------------
 * Set up a poll call for all events in the T.Loop
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                       // 1
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_run( lua_State *L )
{
	struct t_ael     *ael   = t_ael_check_ud( L, 1, 1 );
	struct timeval    tv;      ///< measure time for one iteration
	int               n;       ///< how many file events?

	ael->run                = 1;
	while (ael->run)
	{
		t_tim_now( &tv, 0 );
		if ((n = p_ael_poll_impl( L, ael )) < 0)
			return t_push_error( L, "Failed to continue the loop" );

#if PRINT_DEBUGS == 3
		printf( "oooooooooooooooooooooo POLL RETURNED: %d oooooooooooooooooooo\n", n );
#endif

		// execute timer events
		t_tim_since( &tv );
		t_ael_processTimers( L, &(ael->tmHead), &tv );

		// if there are no events left in the loop stop processing
		ael->run = (NULL==ael->tmHead && ael->fdCount<1) ? 0 : ael->run;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Stop an T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                       // 1
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_stop( lua_State *L )
{
	struct t_ael *ael = t_ael_check_ud( L, 1, 1 );
	ael->run = 0;
	return 0;
}


/**--------------------------------------------------------------------------
 * Prints out the Loop.
 * \param   L      Lua state.
 * \lparam  ud     T.Loop userdata instance.                       // 1
 * \return  int    #stack items returned by function call.
 * \lreturn string formatted string representing T.Loop.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael__tostring( lua_State *L )
{
	struct t_ael *ael = t_ael_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_AEL_TYPE"{%d}: %p", ael->fdCount, ael );
	return 1;
}


/**--------------------------------------------------------------------------
 * How many descriptors is allocated for?
 * \param   L      Lua state.
 * \lparam  ud     T.Loop userdata instance.                       // 1
 * \lreturn int    How many descriptors is space for in T.Loop.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael__len( lua_State *L )
{
	struct t_ael *ael = t_ael_check_ud( L, 1, 1 );
	lua_pushinteger( L, ael->fdCount );
	return 1;
}


/**--------------------------------------------------------------------------
 * Debug print for loops.
 * \param   t_ael    Loop Struct.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_showloop( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tnd *tr  = ael->tmHead;
	struct t_ael_dnd *dnd;
	int               i   = 0;
	int               n   = lua_gettop( L );
	printf( T_AEL_TYPE" %p TIMER LIST:\n", ael );
	while (NULL != tr)
	{
		printf( "\t%d\t{%2ld:%6ld}\t%p   ", ++i,
			tr->tv->tv_sec,  tr->tv->tv_usec,
			tr->tv );
		t_ael_doFunction( L, tr->fR, -1 );
		t_stackPrint( L, n+1, lua_gettop( L ), 1 );
		lua_pop( L, lua_gettop( L ) - n );
		printf( "\n" );
		tr = tr->nxt;
	}
	printf( T_AEL_TYPE" %p HANDLE LIST:\n", ael );
	lua_rawgeti( L, LUA_REGISTRYINDEX, ael->dR );
	lua_pushnil( L );
	while (lua_next( L, -2 ))
	{
		dnd = t_ael_dnd_check_ud( L, -1, 1 );
		if (T_AEL_NO == dnd->msk)
			continue;
		if (T_AEL_RD & dnd->msk)
		{
			printf( "%5d  [R]  ", i );
			t_ael_doFunction( L, dnd->rR, -1 );
			t_stackPrint( L, n+1, lua_gettop( L ), 1 );
			lua_pop( L, lua_gettop( L ) - n );
			printf( "\n" );
		}
		if (T_AEL_WR & dnd->msk)
		{
			printf( "%5d  [W]  ", i );
			t_ael_doFunction( L, dnd->wR, -1 );
			t_stackPrint( L, n+1, lua_gettop( L ), 1 );
			lua_pop( L, lua_gettop( L ) - n );
			printf( "\n" );
		}
		lua_pop( L, 1 );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Get Element from Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                                 // 1
 * \lparam  ud   T.Net.Socket, T.Time or LUA_FILEHANDLE userdata instance. // 2
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_ael__index( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct timeval   *tv;
	struct t_ael_tnd *tnd = ael->tmHead;
	struct t_ael_dnd *dnd ;
	int               fd  = 0;

	if (LUA_TSTRING == lua_type( L, 2 )) // return method: run, stop, addHandle, …
	{
		lua_getmetatable( L, 1 );
		lua_pushvalue( L, 2 );
		lua_gettable( L, -2 );
	}
	else
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, ael->dR );
		fd = t_ael_getHandle( L, 2, 0 );
		if (! fd)  // last chance might be a t.Time
		{
			if (NULL != (tv  = t_tim_check_ud( L, 2, 0 )))
			{
				while (NULL != tnd && NULL != tnd->nxt && tnd->tv != tv)
					tnd = tnd->nxt;
				if (tnd && tv == tnd->tv)
					lua_rawgeti( L, LUA_REGISTRYINDEX, tnd->fR );
				else
					lua_pushnil( L );
			}
			else
				lua_pushnil( L );
		}
		else
		{
			lua_rawgeti( L, -1, fd );
			if (! lua_isnil( L, -1 ))
			{
				dnd = t_ael_dnd_check_ud( L, -1, 1 );
				lua_createtable( L, 0, 2 );
				if (LUA_REFNIL != dnd->rR)
				{
					lua_rawgeti( L, LUA_REGISTRYINDEX, dnd->rR );
					lua_setfield( L, -2, "read" );
				}
				if (LUA_REFNIL != dnd->wR)
				{
					lua_rawgeti( L, LUA_REGISTRYINDEX, dnd->wR );
					lua_setfield( L, -2, "write" );
				}
				// TODO: remove dR ref
			}
		}
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_fm [] = {
	  { "__call",         lt_ael__Call }
	, { NULL,   NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_ael_cf [] = {
	{ NULL,  NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_m [] = {
	// metamethods
	  { "__tostring",    lt_ael__tostring }
	, { "__len",         lt_ael__len }
	, { "__gc",          lt_ael__gc }
	, { "__index",       lt_ael__index }
	// instance methods
	, { "addTimer",       lt_ael_addtimer }
	, { "removeTimer",    lt_ael_removetimer }
	, { "addHandle",      lt_ael_addhandle }
	, { "removeHandle",   lt_ael_removehandle }
	, { "run",            lt_ael_run }
	, { "stop",           lt_ael_stop }
	, { "show",           lt_ael_showloop }
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
LUA_API int
luaopen_t_ael( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_AEL_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_ael_m, 0 );
	luaopen_t_ael_dnd( L );
	lua_setfield( L, -2, T_AEL_DND_NAME );

	// Push the class onto the stack
	luaL_newlib( L, t_ael_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_ael_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
