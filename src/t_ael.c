/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ael.c
 * \brief     OOP wrapper for an asyncronous event loop (T.Loop)
 *            This covers the generic functions which are usable accross
 *            specific implementations
 *            They mainly handle the creation of data structures, their
 *            bindings to the Lua state and the list/collection opeartions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t.h"
#include <stdlib.h>               // malloc, free
#include <string.h>               // memset
#include "t_ael.h"
#include "t_tim.h"


/**----------------------------------------------------------------------------
 * Slot in a timer event into the loops timer event list.
 * \detail  Ordered insert; walks down linked list and inserts before the next
 *          bigger * timer
 * \param   t_ael    Loop Struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_instimer( struct t_ael *ael, struct t_ael_tm *te )
{
	struct t_ael_tm *tr;

	if (NULL == ael->tm_head || t_tim_cmp( ael->tm_head->tv, te->tv, > ))
	{
#if PRINT_DEBUGS == 1
		printf( "Make HEAD   {%2ld:%6ld}\t PRE{%2ld:%6ld}\n",
			te->tv->tv_sec,  te->tv->tv_usec,
			(NULL != ael->tm_head) ? ael->tm_head->tv->tv_sec  : 0,
			(NULL != ael->tm_head) ? ael->tm_head->tv->tv_usec : 0);
#endif
		te->nxt     = ael->tm_head;
		ael->tm_head = te;
	}
	else
	{
		tr = ael->tm_head;
		while (NULL != tr->nxt && t_tim_cmp( tr->nxt->tv, te->tv, < ))
			tr = tr->nxt;
#if PRINT_DEBUGS == 1
		printf( "Make NODE   {%2ld:%6ld}\tPAST{%2ld:%6ld}\n",
			te->tv->tv_sec,  te->tv->tv_usec,
			tr->tv->tv_sec,  tr->tv->tv_usec);
#endif
		te->nxt = tr->nxt;
		tr->nxt = te;
	}
}


/**--------------------------------------------------------------------------
 * Adjust amount of time in the loops timer event list.
 *          substract ta from each timer in the list.
 * \param   t_ael    Loop Struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_ael_adjusttimer( struct t_ael *ael, struct timeval *ta )
{
	struct t_ael_tm *tr = ael->tm_head;
	while (NULL != tr)
	{
		t_tim_sub( tr->tv, ta, tr->tv );
		tr = tr->nxt;
	}
}


/**----------------------------------------------------------------------------
 * Unfolds a lua function and parameters from a table in LUA_REGISTRYINDEX
 * \detail  Takes refPosition and gets table onto the stack. The puts function
 *          and paramters onto stack ready to be executed.
 *          LEAVES TABLE ON STACK -> POP after call!
 * \param   L         The lua state.
 * \param   int           Reference positon.
 * \return  int           Number of arguments to be called by f
 * --------------------------------------------------------------------------*/
static inline int
t_ael_getfunc( lua_State *L, int refPos )
{
	int n;      ///< number of arguments+1(function)
	int j;
	lua_rawgeti( L, LUA_REGISTRYINDEX, refPos );
	n = lua_rawlen( L, -1 );
	for (j=0; j<n; j++)
		lua_rawgeti( L, 2, j+1 );
	return n-1;
}


/**--------------------------------------------------------------------------
 * Executes a timer function and reorganizes the timer linked list
 * \param   L         The lua state.
 * \param   struct xp_lp  Loop struct.
 * \lparam  userdata      T.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_executetimer( lua_State *L, struct t_ael *ael, struct timeval *rt )
{
	struct timeval  *tv;                  ///< timer returned by execution -> if there is
	
	struct t_ael_tm *te = ael->tm_head;   ///< timer to execute is tm_head, ALWAYS
	int    n;                             ///< length of arguments to call

	ael->tm_head = ael->tm_head->nxt;
	n = t_ael_getfunc( L, te->fR );
	lua_call( L, n, 1 );
	t_tim_since( rt );
	t_ael_adjusttimer( ael, rt );
	tv = t_tim_check_ud( L, -1, 0 );
	// reorganize linked timer list
	if (NULL == tv)
	{
		luaL_unref( L, LUA_REGISTRYINDEX, te->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, te->tR );
		free (te);
	}
	else
	{
		*te->tv = *tv;
		t_ael_instimer( ael, te );
	}
	lua_pop( L, 2 );   // pop the one value that lua_call allows to be
	                   // returned and the original reference table
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles
 * \param   L             The lua state.
 * \param   struct xp_lp  Loop struct.
 * \lparam  userdata      t.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_ael_executehandle( lua_State *L, struct t_ael *ael, int fd, enum t_ael_t t )
{
	int n;

	//printf( "%d    %d    %d    %d\n", fd,  ael->fd_set[ fd ]->rR ,  ael->fd_set[ fd ]->wR, t );
	if( t & T_AEL_RD )
	{
		n = t_ael_getfunc( L, ael->fd_set[ fd ]->rR );
		lua_call( L, n , 0 );
		lua_pop( L, 1 );             // remove the table
	}
	// since read func can gc the socket, fd_set[fd] can be NULL
	if( NULL != ael->fd_set[ fd ] && t & T_AEL_WR )
	{
		n = t_ael_getfunc( L, ael->fd_set[ fd ]->wR );
		lua_call( L, n , 0 );
		lua_pop( L, 1 );             // remove the table
	}
}


/**--------------------------------------------------------------------------
 * Create a new t.Loop and return it.
 * \param   L  The lua state.
 * \lparam  int    initial size of fd_event slots in the Loop.
 * \lreturn struct t_ael userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_ael_New( lua_State *L )
{
	size_t                                 sz  = luaL_checkinteger( L, 1 );
	struct t_ael __attribute__ ((unused)) *ael = t_ael_create_ud( L, sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a t.Loop and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table t.Loop.
 * \lreturn struct t_ael userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lt_ael__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_ael_New( L );
}


/**--------------------------------------------------------------------------
 * Create a new t_ael userdata and push to LuaStack.
 * \param   L  The lua state.
 * \param   size_t  how many slots for file/socket events to be created.
 * \return  struct t_ael * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct t_ael
*t_ael_create_ud( lua_State *L, size_t sz )
{
	struct t_ael    *ael;
	size_t           n;

	ael = (struct t_ael *) lua_newuserdata( L, sizeof( struct t_ael ) );
	ael->fd_sz   = sz;
	ael->max_fd  = 0;
	ael->tm_head = NULL;
	ael->fd_set  = (struct t_ael_fd **) malloc( (ael->fd_sz+1) * sizeof( struct t_ael_fd * ) );
	for (n=0; n<=ael->fd_sz; n++) ael->fd_set[ n ] = NULL;
	t_ael_create_ud_impl( ael );
	luaL_getmetatable( L, "T.Loop" );
	lua_setmetatable( L, -2 );
	return ael;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_ael
 * \param   L    The lua state.
 * \param   int      position on the stack
 * \param   int      check(boolean): if true error out on fail
 * \return  struct t_ael*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_ael
*t_ael_check_ud ( lua_State *L, int pos, int check )
{
	void *ud = luaL_checkudata( L, pos, "T.Loop" );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`T.Loop` expected" );
	return (NULL==ud) ? NULL : (struct t_ael *) ud;
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the T.Loop.
 * \param   L  The lua state.
 * \lparam  userdata T.Loop.
 * \lparam  userdata handle.
 * \lparam  bool     shall this be treated as a reader?
 * \lparam  function to be executed when event handler fires.
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
int
lt_ael_addhandle( lua_State *L )
{
	luaL_Stream   *lS;
	struct t_net  *sc;
	int            fd  = 0;
	int            n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	struct t_ael  *ael = t_ael_check_ud( L, 1, 1 );
	enum t_ael_t   t   = lua_toboolean( L, 3 ) ? T_AEL_RD :T_AEL_WR;

	luaL_checktype( L, 4, LUA_TFUNCTION );
	lS = (luaL_Stream *) luaL_testudata( L, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = t_net_check_ud( L, 2, 0 );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return t_push_error( L, "Argument to addHandle must be file or socket" );

	if (NULL == ael->fd_set[ fd ])
	{
		ael->fd_set[ fd ] = (struct t_ael_fd *) malloc( sizeof( struct t_ael_fd ) );
		ael->fd_set[ fd ]->t = T_AEL_NO;
	}

	ael->fd_set[ fd ]->t |= t;

	ael->max_fd = (fd > ael->max_fd) ? fd : ael->max_fd;
	t_ael_addhandle_impl( ael, fd, t );

	lua_createtable( L, n-4, 0 );  // create function/parameter table
	lua_insert( L, 4 );
	//Stack: ael,fd,read/write,TABLE,func,...
	while (n > 4)
		lua_rawseti( L, 4, (n--)-4 );   // add arguments and function (pops each item)
	// pop the function reference table and assign as read or write function
	if (T_AEL_RD & t)
		ael->fd_set[ fd ]->rR = luaL_ref( L, LUA_REGISTRYINDEX );
	else
		ael->fd_set[ fd ]->wR = luaL_ref( L, LUA_REGISTRYINDEX );
	lua_pop( L, 1 ); // pop the read write boolean
	ael->fd_set[ fd ]->hR = luaL_ref( L, LUA_REGISTRYINDEX );      // keep ref to handle so it doesnt gc

	return  0;
}


/**--------------------------------------------------------------------------
 * Remove a Handle event handler from the T.Loop.
 * \param   L    The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata socket or file.                            // 2
 * \lparam  bool     remove read event trigger?                 // 3
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
int
lt_ael_removehandle( lua_State *L )
{
	luaL_Stream   *lS;
	struct t_net  *sc;
	int            fd  = 0;
	struct t_ael  *ael = t_ael_check_ud( L, 1, 1 );
	luaL_checktype( L, 3, LUA_TBOOLEAN );
	enum t_ael_t   t   = lua_toboolean( L, 3 ) ? T_AEL_RD :T_AEL_WR;

	lS = (luaL_Stream *) luaL_testudata( L, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = t_net_check_ud( L, 2, 0 );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return t_push_error( L, "Argument to addHandle must be file or socket" );
	// remove function
	if (T_AEL_RD & t)
	{
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ fd ]->rR );
	}
	else
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ fd ]->wR );
	t_ael_removehandle_impl( ael, fd, t );
	// remove from mask
	ael->fd_set[ fd ]->t = ael->fd_set[ fd ]-> t & (~t);
	// remove from loop if empty
	if (T_AEL_NO == ael->fd_set[ fd ]->t )
	{
		luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ fd ]->hR );
		free( ael->fd_set[ fd ] );
		ael->fd_set[ fd ] = NULL;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Add a Timer event handler to the T.Loop.
 * \param   L  The lua state.
 * \lparam  userdata T.Loop.                                     // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  function to be executed when event handler fires.    // 3
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_ael_addtimer( lua_State *L )
{
	struct t_ael    *ael = t_ael_check_ud( L, 1, 1 );
	struct timeval  *tv  = t_tim_check_ud( L, 2, 1 );
	int              n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	struct t_ael_tm *te;

	luaL_checktype( L, 3, LUA_TFUNCTION );
	// Build up the timer element
	te = (struct t_ael_tm *) malloc( sizeof( struct t_ael_tm ) );
	te->tv =  tv;
	//t_ael_addtimer_impl( ael, tv );
	lua_createtable( L, n-3, 0 );  // create function/parameter table
	lua_insert( L, 3 );
	// Stack: ael,tv,TABLE,func,...
	while (n > 3)
		lua_rawseti( L, 3, (n--)-3 );            // add arguments and function (pops each item)
	te->fR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the function/parameter table
	// making the time val part of lua registry guarantees the gc can't destroy it
	te->tR = luaL_ref( L, LUA_REGISTRYINDEX );  // pop the timeval
	// insert into ordered linked list of time events
	t_ael_instimer( ael, te );

	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a Timer event handler from the T.Loop.
 * \param   L    The lua state.
 * \lparam  userdata T.Loop.                                     // 1
 * \lparam  userdata timeval.                                    // 2
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int
lt_ael_removetimer( lua_State *L )
{
	struct t_ael    *ael = t_ael_check_ud( L, 1, 1 );
	struct timeval  *tv  = t_tim_check_ud( L, 2, 1 );
	struct t_ael_tm *tp  = ael->tm_head;
	struct t_ael_tm *te  = tp->nxt;       ///< previous Timer event

	// if head is node in question
	if (NULL != tp  &&  tp->tv == tv)
	{
		ael->tm_head = te;
		luaL_unref( L, LUA_REGISTRYINDEX, tp->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tp->tR );
		free( tp );
		return 0;
	}
	while (NULL != te && te->tv != tv)
	{
		tp = te;
		te = te->nxt;
	}

	if (NULL!=te && te->tv == tv)
	{
		tp->nxt = te->nxt;
		luaL_unref( L, LUA_REGISTRYINDEX, te->fR );
		luaL_unref( L, LUA_REGISTRYINDEX, tp->tR );
		free( te );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Free events in allocated spots.
 * \param  L   lua Virtual Machine.
 * \lparam table   T.Loop.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_ael__gc( lua_State *L )
{
	struct t_ael    *ael     = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tm *tf, *tr = ael->tm_head;
	size_t           i;       ///< the iterator for all fields

	while (NULL != tr)
	{
		tf = tr;
		//printf( "Start  %p   %d   %d    %p\n", tf, tf->fR, tf->tR, tf->nxt );
		luaL_unref( L, LUA_REGISTRYINDEX, tf->fR ); // remove func/arg table from registry
		luaL_unref( L, LUA_REGISTRYINDEX, tf->tR ); // remove timeval ref from registry
		tr = tr->nxt;
		//printf( "Free   %p   %d   %d    %p\n", tf, tf->fR, tf->tR, tf->nxt );
		free( tf );
	}
	//printf("---------");
	for (i=0; i < ael->fd_sz; i++)
	{
		if (NULL != ael->fd_set[ i ])
		{
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ i ]->rR );
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ i ]->wR );
			luaL_unref( L, LUA_REGISTRYINDEX, ael->fd_set[ i ]->hR );
			free( ael->fd_set[ i ] );
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   L  The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_ael_run( lua_State *L )
{
	struct t_ael    *ael = t_ael_check_ud( L, 1, 1 );
	ael->run = 1;

	while (ael->run)
	{
		if (t_ael_poll_impl( L, ael ) < 0)
		{
			return t_push_error( L, "Failed to continue" );
		}
		// if there are no events left in the loop stop processing
		ael->run = (NULL==ael->tm_head && ael->max_fd<1) ? 0 : ael->run;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Stop an T.Loop.
 * \param   L  The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \return  #stack items returned by function call.
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
 * \param   L     The lua state.
 * \lparam  userdata  T.Loop userdata
 * \lreturn string    formatted string representing T.Loop.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_ael__tostring( lua_State *L )
{
	struct t_ael *ael = t_ael_check_ud( L, 1, 1 );
	lua_pushfstring( L, "T.Loop{%d:%d}: %p", ael->fd_sz, ael->max_fd, ael );
	return 1;
}


/**--------------------------------------------------------------------------
 * Debug print for loops.
 * \param   t_ael    Loop Struct.
 * \return  int #elements returned to function call(Stack)
 * --------------------------------------------------------------------------*/
int
lt_ael_showloop( lua_State *L )
{
	struct t_ael    *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tm *tr  = ael->tm_head;
	int              i   = 0;
	int              n   = lua_gettop( L );
	printf( "LOOP %p TIMER LIST:\n", ael );
	while (NULL != tr)
	{
		printf( "\t%d\t{%2ld:%6ld}\t%p   ", ++i,
			tr->tv->tv_sec,  tr->tv->tv_usec,
			tr->tv );
		t_ael_getfunc( L, tr->fR );
		t_stackPrint( L, n+1, lua_gettop( L ) );
		lua_pop( L, lua_gettop( L ) - n );
		printf( "\n" );
		tr = tr->nxt;
	}
	printf( "LOOP %p HANDLE LIST:\n", ael );
	for( i=0; i<ael->max_fd+1; i++)
	{
		if (NULL==ael->fd_set[ i ])
			continue;
		if (T_AEL_RD & ael->fd_set[i]->t)
		{
			printf( "%5d  [R]  ", i );
			t_ael_getfunc( L, ael->fd_set[i]->rR );
			t_stackPrint( L, n+2, lua_gettop( L ) );
			lua_pop( L, lua_gettop( L ) - n );
			printf( "\n" );
		}
		if (T_AEL_WR & ael->fd_set[i]->t)
		{
			printf( "%5d  [W]  ", i );
			t_ael_getfunc( L, ael->fd_set[i]->wR );
			t_stackPrint( L, n+2, lua_gettop( L ) );
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
	{ "__call",         lt_ael__Call },
	{ NULL,   NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_ael_cf [] = {
	{ "new",            lt_ael_New },
	{ NULL,  NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_m [] = {
	// metamethods
	{ "__tostring",    lt_ael__tostring },
	{ "__gc",          lt_ael__gc },
	// instance methods
	{ "addTimer",       lt_ael_addtimer },
	{ "removeTimer",    lt_ael_removetimer },
	{ "addHandle",      lt_ael_addhandle },
	{ "removeHandle",   lt_ael_removehandle },
	{ "run",            lt_ael_run },
	{ "stop",           lt_ael_stop },
	{ "show",           lt_ael_showloop },
	{ NULL,   NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Loop library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_ael( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, "T.Loop" );   // stack: functions meta
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
