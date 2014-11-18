/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_lp.c
 * \brief     OOP wrapper for an asyncronous eventloop (xt.Loop)
 *            This covers the generic functions which are usable accross
 *            specific implementations
 *            They mainly handle the creation of data structures, their
 *            bindings to the Lua state and the list/collection opeartions
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */


#include "xt.h"
#include <stdlib.h>               // malloc, free
#include <string.h>               // memset
#include "xt_lp.h"
#include "xt_time.h"


/**--------------------------------------------------------------------------
 * Slot in a timer event into the loops timer event list.
 * \param   xt_lp    Loop Struct.
 * \lreturn xt_lp_tm Timer event struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void xt_lp_instimer( struct xt_lp *lp, struct xt_lp_tm *te )
{
	struct xt_lp_tm *tr;

	if (NULL == lp->tm_head || xt_time_cmp( lp->tm_head->tv, te->tv, > ))
	{
#if PRINT_DEBUGS == 1
		printf( "Make HEAD   {%2ld:%6ld}\t PRE{%2ld:%6ld}\n",
			te->tv->tv_sec,  te->tv->tv_usec,
			(NULL != lp->tm_head) ? lp->tm_head->tv->tv_sec  : 0,
			(NULL != lp->tm_head) ? lp->tm_head->tv->tv_usec : 0);
#endif
		te->nxt     = lp->tm_head;
		lp->tm_head = te;
	}
	else
	{
		tr = lp->tm_head;
		while (NULL != tr->nxt && xt_time_cmp( tr->nxt->tv, te->tv, < ))
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
 * \param   xt_lp    Loop Struct.
 * \lreturn xt_lp_tm Timer event struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void xt_lp_adjusttimer( struct xt_lp *lp, struct timeval *ta )
{
	struct xt_lp_tm *tr = lp->tm_head;
	while (NULL != tr)
	{
		xt_time_sub( tr->tv, ta, tr->tv );
		tr = tr->nxt;
	}
}


/**--------------------------------------------------------------------------
 * Unfolds a lua function and parameters from a table in LUA_REGISTRYINDEX
 *          takes refPosition and gets table onto the stack. The puts function
 *          and paramters onto stack ready to be executed.
 *          LEAVES TABLE ON STACK -> POP after call!
 * \param   luaVM         The lua state.
 * \param   int           Reference positon.
 * \return  int           Number of arguments to be called by f
 * --------------------------------------------------------------------------*/
static inline int xt_lp_getfunc( lua_State *luaVM, int refPos )
{
	int n;      ///< number of arguments+1(function)
	int j;
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, refPos );
	n = lua_rawlen( luaVM, -1 );
	for (j=0; j<n; j++)
		lua_rawgeti( luaVM, 2, j+1 );
	return n-1;
}


/**--------------------------------------------------------------------------
 * Executes a timer function and reorganizes the timer linked list
 * \param   luaVM         The lua state.
 * \param   struct xp_lp  Loop struct.
 * \lparam  userdata      xt.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void xt_lp_executetimer( lua_State *luaVM, struct xt_lp *lp, struct timeval *rt )
{
	struct timeval *tv; ///< timer returned by execution -> if there is
	// the timer to be executed is the tm_head, ALWAYS
	struct xt_lp_tm *te = lp->tm_head;
	lp->tm_head = lp->tm_head->nxt;
	lua_call( luaVM, xt_lp_getfunc( luaVM, te->fR ), 1 );
	xt_time_since( rt );
	xt_lp_adjusttimer( lp, rt );
	tv = (struct timeval *) luaL_testudata( luaVM, -1, "xt.Time" );
	// reorganize linked timer list
	if (NULL == tv)
	{
		luaL_unref( luaVM,LUA_REGISTRYINDEX, te->fR );
		luaL_unref( luaVM,LUA_REGISTRYINDEX, te->tR );
		free (te);
	}
	else
	{
		*te->tv = *tv;
		xt_lp_instimer( lp, te );
	}
	lua_pop( luaVM, 2 );   // pop the one value that lua_call allows to be
	                       // returned and the original reference table
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles
 * \param   luaVM         The lua state.
 * \param   struct xp_lp  Loop struct.
 * \lparam  userdata      xt.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void xt_lp_executehandle( lua_State *luaVM, struct xt_lp *lp, int fd )
{
	int n = xt_lp_getfunc( luaVM, lp->fd_set[ fd ]->fR );
	lua_call( luaVM, n , 0 );
	lua_pop( luaVM, 1 );             // remove the table
}


/**--------------------------------------------------------------------------
 * Construct a xt.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table xt.Loop.
 * \lreturn struct xt_lp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lxt_lp_New( luaVM );
}


/**--------------------------------------------------------------------------
 * Create a new xt.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  int    initial size of fd_event slots in the Loop.
 * \lreturn struct xt_lp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
int lxt_lp_New( lua_State *luaVM )
{
	size_t                                   sz = luaL_checkinteger( luaVM, 1 );
	struct xt_lp  __attribute__ ((unused))  *lp = xt_lp_create_ud( luaVM, sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new xt_lp userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \param   size_t  how many slots for file/socket events to be created.
 * \return  struct xt_lp * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct xt_lp *xt_lp_create_ud( lua_State *luaVM, size_t sz )
{
	struct xt_lp    *lp;

	lp = (struct xt_lp *) lua_newuserdata( luaVM, sizeof( struct xt_lp ) );
	lp->fd_sz   = sz;
	lp->mxfd    = 0;
	lp->tm_head = NULL;
	lp->fd_set  = (struct xt_lp_fd **) malloc( lp->fd_sz * sizeof( struct xt_lp_fd * ) );
	memset( lp->fd_set, 0, lp->fd_sz * sizeof( struct xt_lp_fd * ) );
	xt_lp_create_ud_impl( lp );
	luaL_getmetatable( luaVM, "xt.Loop" );
	lua_setmetatable( luaVM, -2 );
	return lp;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct xt_lp
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct xt_lp*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct xt_lp *xt_lp_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Loop" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Loop` expected" );
	return (struct xt_lp *) ud;
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.
 * \lparam  userdata handle.
 * \lparam  bool     shall this be treated as a reader?
 * \lparam  function to be executed when event handler fires.
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp_addhandle( lua_State *luaVM )
{
	luaL_Stream    *lS;
	struct xt_sck  *sc;
	int             fd = 0;
	int              n = lua_gettop( luaVM ) + 1;    ///< iterator for arguments
	struct xt_lp   *lp = xt_lp_check_ud( luaVM, 1);

	luaL_checktype( luaVM, 4, LUA_TFUNCTION );
	lS = (luaL_Stream *) luaL_testudata( luaVM, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = (struct xt_sck *) luaL_testudata( luaVM, 2, "xt.Socket" );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return xt_push_error( luaVM, "Argument to addHandle must be file or socket" );

	lp->fd_set[ fd ] = (struct xt_lp_fd *) malloc( sizeof( struct xt_lp_tm ) );
	xt_lp_addhandle_impl( lp, fd, lua_toboolean( luaVM, 3 ) );

	lua_createtable( luaVM, n-4, 0 );  // create function/parameter table
	lua_insert( luaVM, 4 );
	//Stack: lp,tv,read/write,TABLE,func,...
	while (n > 4)
		lua_rawseti( luaVM, 4, (n--)-4 );   // add arguments and function (pops each item)
	lp->fd_set[ fd ]->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // pop the function/parameter table
	lua_pop( luaVM, 1 ); // pop the read write boolean
	lp->fd_set[ fd ]->hR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // keep ref to handle so it doesnt gc

	return  0;
}


/**--------------------------------------------------------------------------
 * Remove a Handle event handler from the xt.Loop.
 * \param   luaVM    The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata socket or file.                             // 2
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int lxt_lp_removehandle( lua_State *luaVM )
{
	luaL_Stream     *lS;
	struct xt_sck   *sc;
	int              fd = 0;
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );

	lS = (luaL_Stream *) luaL_testudata( luaVM, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = (struct xt_sck *) luaL_testudata( luaVM, 2, "xt.Socket" );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return xt_push_error( luaVM, "Argument to addHandle must be file or socket" );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, lp->fd_set[ fd ]->fR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, lp->fd_set[ fd ]->hR );
	free( lp->fd_set[ fd ] );
	lp->fd_set[ fd ] = NULL;

	return 0;
}

/**--------------------------------------------------------------------------
 * Add a Timer event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  function to be executed when event handler fires.    // 3
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp_addtimer( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	struct timeval  *tv = xt_time_check_ud( luaVM, 2 );
	int              n  = lua_gettop( luaVM ) + 1;    ///< iterator for arguments
	struct xt_lp_tm *te;

	luaL_checktype( luaVM, 3, LUA_TFUNCTION );
	// Build up the timer element
	te = (struct xt_lp_tm *) malloc( sizeof( struct xt_lp_tm ) );
	te->tv =  tv;
	//xt_lp_addtimer_impl( lp, tv );
	lua_createtable( luaVM, n-3, 0 );  // create function/parameter table
	lua_insert( luaVM, 3 );
	// Stack: lp,tv,TABLE,func,...
	while (n > 3)
		lua_rawseti( luaVM, 3, (n--)-3 );            // add arguments and function (pops each item)
	te->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );  // pop the function/parameter table
	// making the time val part of lua registry guarantees the gc can't destroy it
	te->tR = luaL_ref( luaVM, LUA_REGISTRYINDEX );  // pop the timeval
	// insert into ordered linked list of time events
	xt_lp_instimer( lp, te );

	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a Timer event handler from the xt.Loop.
 * \param   luaVM    The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int lxt_lp_removetimer( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	struct timeval  *tv = xt_time_check_ud( luaVM, 2 );
	struct xt_lp_tm *tp = lp->tm_head;
	struct xt_lp_tm *te = tp->nxt;       ///< previous Timer event

	// if head is node in question
	if (NULL != tp  &&  tp->tv == tv)
	{
		lp->tm_head = te;
		luaL_unref( luaVM,LUA_REGISTRYINDEX, tp->fR );
		luaL_unref( luaVM,LUA_REGISTRYINDEX, tp->tR );
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
		luaL_unref( luaVM,LUA_REGISTRYINDEX, te->fR );
		luaL_unref( luaVM,LUA_REGISTRYINDEX, tp->tR );
		free( te );
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Free events in allocated spots.
 * \param  luaVM   lua Virtual Machine.
 * \lparam table   xt.Loop.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_lp__gc( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	//struct xt_lp_fd *f;
	struct xt_lp_tm *tf, *tr = lp->tm_head;
	size_t           i;       ///< the iterator for all fields

	while (NULL != tr)
	{
		tf = tr;
		//printf( "Start  %p   %d   %d    %p\n", tf, tf->fR, tf->tR, tf->nxt );
		luaL_unref( luaVM, LUA_REGISTRYINDEX, tf->fR ); // remove func/arg table from registry
		luaL_unref( luaVM, LUA_REGISTRYINDEX, tf->tR ); // remove timeval ref from registry
		tr = tr->nxt;
		//printf( "Free   %p   %d   %d    %p\n", tf, tf->fR, tf->tR, tf->nxt );
		free( tf );
	}
	//printf("---------");
	for (i=0; i < lp->fd_sz; i++)
	{
		if (NULL != lp->fd_set[ i ])
		{
			luaL_unref( luaVM, LUA_REGISTRYINDEX, lp->fd_set[ i ]->fR );
			luaL_unref( luaVM, LUA_REGISTRYINDEX, lp->fd_set[ i ]->hR );
			free( lp->fd_set[ i ] );
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the xt.Loop
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp_run( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	lp->run = 1;

	while (lp->run)
	{
		if (xt_lp_poll_impl( luaVM, lp ) < 0)
			return 0;
		// if there are no events left in the loop stop processing
		lp->run =  (NULL==lp->tm_head && lp->mxfd<1) ? 0 : lp->run;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Stop an xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp_stop( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	lp->run = 0;
	return 0;
}

/**--------------------------------------------------------------------------
 * Prints out the Loop.
 * \param   luaVM     The lua state.
 * \lparam  userdata  xt.Loop userdata
 * \lreturn string    formatted string representing xt.Loop.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_lp__tostring( lua_State *luaVM )
{
	struct xt_lp *lp = xt_lp_check_ud( luaVM, 1 );
	lua_pushfstring( luaVM, "xt.Loop(select){%d}: %p", lp->mxfd, lp );
	return 1;
}


/**--------------------------------------------------------------------------
 * Debug print for loops.
 * \param   xt_lp    Loop Struct.
 * \return  int #elements returned to function call(Stack)
 * --------------------------------------------------------------------------*/
static int lxt_lp_showloop( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	struct xt_lp_tm *tr = lp->tm_head;
	int              i  = 0;
	int              n  = lua_gettop( luaVM );
	printf( "LOOP %p TIMER LIST:\n", lp );
	while (NULL != tr)
	{
		printf("\t%d\t{%2ld:%6ld}\t%p   ", ++i,
			tr->tv->tv_sec,  tr->tv->tv_usec,
			tr->tv);
		xt_lp_getfunc( luaVM, tr->fR );
		xt_stackPrint( luaVM, n+1, lua_gettop( luaVM ) );
		lua_pop( luaVM, lua_gettop( luaVM ) - n );
		printf("\n");
		tr = tr->nxt;
	}
	printf( "LOOP %p HANDLE LIST:\n", lp );
	for( i=0; i<lp->mxfd+1; i++)
	{
		if (NULL==lp->fd_set[ i ])
			continue;
		printf("\t%d\t%s   ", i,
			(XT_LP_READ == lp->fd_set[i]->t) ? "READER" : "WRITER");
		xt_lp_getfunc( luaVM, lp->fd_set[i]->fR );
		xt_stackPrint( luaVM, n+1, lua_gettop( luaVM ) );
		lua_pop( luaVM, lua_gettop( luaVM ) - n );
		printf("\n");
	}
	return 0;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_lp_fm [] =
{
	{"__call",    lxt_lp__Call},
	{NULL,   NULL}
};

/**
 * \brief      the Time library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_lp_cf [] =
{
	{"new",       lxt_lp_New},
	{NULL,        NULL}
};


/**
 * \brief      the Timer library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_lp_m [] =
{
	{"addTimer",       lxt_lp_addtimer},
	{"removeTimer",    lxt_lp_removetimer},
	{"addHandle",      lxt_lp_addhandle},
	{"removeHandle",   lxt_lp_removehandle},
	{"run",            lxt_lp_run},
	{"stop",           lxt_lp_stop},
	{"show",           lxt_lp_showloop},
	{NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the Loop library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_xt_lp( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "xt.Loop" );   // stack: functions meta
	luaL_newlib( luaVM, xt_lp_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_lp__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_lp__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	luaL_newlib( luaVM, xt_lp_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( luaVM, xt_lp_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}
