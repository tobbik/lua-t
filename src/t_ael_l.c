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

#define PRINT_DEBUGS 0

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

	if (NULL == *tHead || t_tim_cmp( (*tHead)->tv, tIns->tv, > ))
	{
#if PRINT_DEBUGS == 1
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
#if PRINT_DEBUGS == 1
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
	while (NULL != tRun)
	{
		t_tim_sub( tRun->tv, tAdj, tRun->tv );
		tRun = tRun->nxt;
	}
}


/**----------------------------------------------------------------------------
 * Unfolds a Lua function and parameters from a table in LUA_REGISTRYINDEX
 * Takes refPosition and gets table onto the stack.  Leaves function and
 * paramters onto stack ready to be executed.
 * Stack before: {fnc, p1, p2, p3, ... }
 * Stack after:   fnc  p1  p2  p3  ...
 * \param   L     Lua state.
 * \param   int   Reference positon.
 * \return  int   Number of arguments to be called by function.
 * --------------------------------------------------------------------------*/
static inline int
t_ael_getFunction( lua_State *L, int refPos )
{
	int n;      ///< number of arguments + 1(function)
	int p;      ///< position of pickled function and args
	int i;
	lua_rawgeti( L, LUA_REGISTRYINDEX, refPos );
	p = lua_gettop( L );
	n = lua_rawlen( L, p );
	for (i=0; i<n; i++)
		lua_rawgeti( L, p, i+1 );
	lua_remove( L, p );
	return n-1;  // return number of args
}


/**--------------------------------------------------------------------------
 * Pop timer list head, execute and readd if needed.
 * \param   L        Lua state.
 * \param   **tHead  t_ael_tnd; pointer to Head of timer linked list pointer.
 * \param   *rt      struct timeval; measure how long since last poll fall through
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_executeHeadTimer( lua_State *L, struct t_ael_tnd **tHead, struct timeval *rt )
{
	struct timeval   *tv;                  ///< timer returned by execution -> if there is
	
	struct t_ael_tnd *tExc = *tHead;       ///< timer to execute is tHead, ALWAYS
	int    n;                              ///< number of arguments to function call

	*tHead = (*tHead)->nxt;
	n      = t_ael_getFunction( L, tExc->fR );
	lua_call( L, n, 1 );  // if NO T.Time is return it's a nil
	t_tim_since( rt );
	t_ael_adjustTimers( tHead, rt );
	tv = t_tim_check_ud( L, -1, 0 );
	if (NULL == tv)   // remove from list
	{
		luaL_unref( L, LUA_REGISTRYINDEX, tExc->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tExc->tR );
		free( tExc );
	}
	else              // re-add node to list if function returned a timer
	{
		// re-use timer node which still has function and parameters pickled
		// but use new tv reference if tv wasn't reused
		if (tv != tExc->tv)
		{
			luaL_unref( L, LUA_REGISTRYINDEX, tExc->tR );
			tExc->tR = luaL_ref( L, LUA_REGISTRYINDEX );
			tExc->tv = tv;
		}
		t_ael_insertTimer( tHead, tExc );
	}
	lua_pop( L, 1 );   // pop the one value that lua_call allows for
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles.
 * \param   L        Lua state.
 * \param   *dNde    Descriptor node.
 * \param   msk      execute read or write or both.
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_executehandle( lua_State *L, struct t_ael_fd *ev, enum t_ael_msk msk )
{
	int n;

	//printf( "%d    %d    %d    %d\n", ev, rR, wR, msk );
	if (NULL != ev && msk & T_AEL_RD & ev->msk)
	{
		n = t_ael_getFunction( L, ev->rR );
		lua_call( L, n , 0 );
	}
	if (NULL != ev && msk & T_AEL_WR & ev->msk)
	{
		n = t_ael_getFunction( L, ev->wR );
		lua_call( L, n , 0 );
	}
}


/**--------------------------------------------------------------------------
 * Construct a T.Loop and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table t.Loop.
 * \lreturn int    int initial descriptor capacity of Loop.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_ael__Call( lua_State *L )
{
	size_t                                 sz  = luaL_checkinteger( L, 2 );
	struct t_ael __attribute__ ((unused)) *ael = t_ael_create_ud( L, sz );

	lua_remove( L, 1 );   // remove CLASS table
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new t_ael userdata and push to LuaStack.
 * \param   L    Lua state.
 * \param   sz   size_t; how many slots for file/socket events to be created.
 * \return  ael  struct t_ael * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_ael
*t_ael_create_ud( lua_State *L, size_t sz )
{
	struct t_ael    *ael;
	size_t           n;

	ael = (struct t_ael *) lua_newuserdata( L, sizeof( struct t_ael ) );
	ael->fdCount = sz;
	ael->fdMax   = 0;
	ael->tmHead  = NULL;
	ael->fdSet   = (struct t_ael_fd **) malloc( (ael->fdCount+1) * sizeof( struct t_ael_fd * ) );
	for (n=0; n<=ael->fdCount; n++) ael->fdSet[ n ] = NULL;
	p_ael_create_ud_impl( L, ael );
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
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_AEL_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_ael *) ud;
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                          // 1
 * \lparam  ud   T.Net.Socket or LUA_FILEHANDLE userdata instance.  // 2
 * \lparam  bool true if observe incoming; else outgoing?           // 3
 * \lparam  func to be executed when event handler fires.           // 4
 * \lparam  ...  parameters to function when executed.              // 5 ...
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_ael_addhandle( lua_State *L )
{
	luaL_Stream      *lS;
	struct t_net_sck *sck;
	int               fd  = 0;
	int               n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	enum t_ael_msk    msk;

	t_getTypeByName( L, 3, NULL, t_ael_directionList );
	luaL_argcheck( L, ! lua_isnil( L, 3 ), 3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );

	luaL_checktype( L, 4, LUA_TFUNCTION );
	lS = (luaL_Stream *) luaL_testudata( L, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sck = t_net_sck_check_ud( L, 2, 0 );
	if (NULL != sck)
		fd = sck->fd;

	if (0 == fd)
		return luaL_error( L, "Argument to addHandle must be file or socket" );

	if (NULL == ael->fdSet[ fd ])
	{
		ael->fdSet[ fd ] = (struct t_ael_fd *) malloc( sizeof( struct t_ael_fd ) );
		ael->fdSet[ fd ]->msk = T_AEL_NO;
	}

	p_ael_addhandle_impl( L, ael, fd, msk );
	ael->fdSet[ fd ]->msk |= msk;
	ael->fdMax = (fd > ael->fdMax) ? fd : ael->fdMax;

	lua_createtable( L, n-4, 0 );  // create function/parameter table
	lua_insert( L, 4 );                            //S: ael hdl dir tbl fnc …
	while (n > 4)
		lua_rawseti( L, 4, (n--)-4 );   // add arguments and function (pops each item)
	// pop the function reference table and assign as read or write function
	if (T_AEL_RD & msk)
		ael->fdSet[ fd ]->rR = luaL_ref( L, LUA_REGISTRYINDEX );
	else
		ael->fdSet[ fd ]->wR = luaL_ref( L, LUA_REGISTRYINDEX );
	lua_pop( L, 1 ); // pop the read write boolean
	ael->fdSet[ fd ]->hR = luaL_ref( L, LUA_REGISTRYINDEX );      // keep ref to handle so it doesnt gc

	return  0;
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
int
lt_ael_removehandle( lua_State *L )
{
	luaL_Stream      *lS;
	struct t_net_sck *sck;
	int               fd  = 0;
	int                i;
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	enum t_ael_msk    msk;

	t_getTypeByName( L, 3, NULL, t_ael_directionList );
	luaL_argcheck( L, ! lua_isnil( L, 3 ), 3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );

	lS = (luaL_Stream *) luaL_testudata( L, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sck = t_net_sck_check_ud( L, 2, 0 );
	if (NULL != sck)
		fd = sck->fd;

	if (0 == fd)
		return luaL_error( L, "Argument to addHandle must be file or socket" );
	if (-1 == fd)
		return luaL_error( L, "Can't remove closed descriptor" );
	if (NULL == ael->fdSet[ fd ] || T_AEL_NO & ael->fdSet[ fd ]->msk)
		return luaL_error( L, "Descriptor is not observed in loop" );
	// remove function
	if (T_AEL_RD & msk & ael->fdSet[ fd ]->msk)
	{
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ fd ]->rR );
		ael->fdSet[ fd ]->wR = LUA_REFNIL;
	}
	else
	{
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ fd ]->wR );
		ael->fdSet[ fd ]->wR = LUA_REFNIL;
	}
	p_ael_removehandle_impl( L, ael, fd, msk );
	// remove from mask
	ael->fdSet[ fd ]->msk = ael->fdSet[ fd ]->msk & (~msk);
	// remove from loop if no observed at all anymore
	if (T_AEL_NO == ael->fdSet[ fd ]->msk )
	{
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ fd ]->hR );
		free( ael->fdSet[ fd ] );
		ael->fdSet[ fd ] = NULL;

		// reset the maxFd
		if (fd == ael->fdMax)
		{
			for (i = ael->fdMax-1; i >= 0; i--)
				if (NULL != ael->fdSet[ i ] && T_AEL_NO != ael->fdSet[ i ]->msk) break;
			ael->fdMax = i;
		}
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Add a Timer event handler to the T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \lparam  ud   T.Time userdata instance.                   // 2
 * \lparam  func to be executed when event handler fires.    // 3
 * \lparam  ...  parameters to function when executed.       // 4 ...
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_addtimer( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct timeval   *tv  = t_tim_check_ud( L, 2, 1 );
	int               n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	struct t_ael_tnd *tNew;

	luaL_checktype( L, 3, LUA_TFUNCTION );
	// Build up the timer element
	tNew = (struct t_ael_tnd *) malloc( sizeof( struct t_ael_tnd ) );
	tNew->tv =  tv;
	//p_ael_addtimer_impl( ael, tv );
	lua_createtable( L, n-3, 0 );  // create function/parameter table
	lua_insert( L, 3 );
	// Stack: ael,tv,TABLE,func,...
	while (n > 3)
		lua_rawseti( L, 3, (n--)-3 );            // add arguments and function (pops each item)
	tNew->fR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the function/parameter table
	// making the time val part of lua registry guarantees the gc can't destroy it
	tNew->tR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the timeval
	// insert into ordered linked list of time events
	t_ael_insertTimer( &(ael->tmHead), tNew );

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
	size_t            i;       ///< the iterator for all fields

	while (NULL != tRun)
	{
		tFre = tRun;
		//printf( "Start  %p   %d   %d    %p\n", tFre, tFre->fR, tFre->tR, tFre->nxt );
		luaL_unref( L, LUA_REGISTRYINDEX, tFre->fR ); // remove func/arg table from registry
		luaL_unref( L, LUA_REGISTRYINDEX, tFre->tR ); // remove timeval ref from registry
		tRun = tRun->nxt;
		//printf( "Free   %p   %d   %d    %p\n", tFre, tFre->fR, tFre->tR, tFre->nxt );
		free( tFre );
	}
	//printf("---------");
	for (i=0; i < ael->fdCount; i++)
	{
		if (NULL != ael->fdSet[ i ])
		{
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ i ]->rR );
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ i ]->wR );
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fdSet[ i ]->hR );
			free( ael->fdSet[ i ] );
		}
	}
	p_ael_free_impl( ael );
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
	struct t_ael    *ael = t_ael_check_ud( L, 1, 1 );
	ael->run = 1;

	while (ael->run)
	{
		if (p_ael_poll_impl( L, ael ) < 0)
			return t_push_error( L, "Failed to continue" );
		// if there are no events left in the loop stop processing
		ael->run = (NULL==ael->tmHead && ael->fdMax<1) ? 0 : ael->run;
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
	lua_pushfstring( L, T_AEL_TYPE"{%d:%d}: %p", ael->fdCount, ael->fdMax, ael );
	return 1;
}


/**--------------------------------------------------------------------------
 * Debug print for loops.
 * \param   t_ael    Loop Struct.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_ael_showloop( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tnd *tr  = ael->tmHead;
	int               i   = 0;
	int               n   = lua_gettop( L );
	printf( T_AEL_TYPE" %p TIMER LIST:\n", ael );
	while (NULL != tr)
	{
		printf( "\t%d\t{%2ld:%6ld}\t%p   ", ++i,
			tr->tv->tv_sec,  tr->tv->tv_usec,
			tr->tv );
		t_ael_getFunction( L, tr->fR );
		t_stackPrint( L, n+1, lua_gettop( L ), 1 );
		lua_pop( L, lua_gettop( L ) - n );
		printf( "\n" );
		tr = tr->nxt;
	}
	printf( T_AEL_TYPE" %p HANDLE LIST:\n", ael );
	for( i=0; i<ael->fdMax+1; i++)
	{
		if (NULL==ael->fdSet[ i ])
			continue;
		if (T_AEL_RD & ael->fdSet[i]->msk)
		{
			printf( "%5d  [R]  ", i );
			t_ael_getFunction( L, ael->fdSet[i]->rR );
			t_stackPrint( L, n+1, lua_gettop( L ), 1 );
			lua_pop( L, lua_gettop( L ) - n );
			printf( "\n" );
		}
		if (T_AEL_WR & ael->fdSet[i]->msk)
		{
			printf( "%5d  [W]  ", i );
			t_ael_getFunction( L, ael->fdSet[i]->wR );
			t_stackPrint( L, n+1, lua_gettop( L ), 1 );
			lua_pop( L, lua_gettop( L ) - n );
			printf( "\n" );
		}
	}
	return 0;
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
	, { "__gc",          lt_ael__gc }
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
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	luaL_newlib( L, t_ael_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_ael_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
