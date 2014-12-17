/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_elp.c
 * \brief     OOP wrapper for an asyncronous eventloop (T.Loop)
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
#include "t_elp.h"
#include "t_tim.h"


/**--------------------------------------------------------------------------
 * Slot in a timer event into the loops timer event list.
 * \param   t_elp    Loop Struct.
 * \lreturn t_elp_tm Timer event struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_elp_instimer( struct t_elp *elp, struct t_elp_tm *te )
{
	struct t_elp_tm *tr;

	if (NULL == elp->tm_head || t_tim_cmp( elp->tm_head->tv, te->tv, > ))
	{
#if PRINT_DEBUGS == 1
		printf( "Make HEAD   {%2ld:%6ld}\t PRE{%2ld:%6ld}\n",
			te->tv->tv_sec,  te->tv->tv_usec,
			(NULL != elp->tm_head) ? elp->tm_head->tv->tv_sec  : 0,
			(NULL != elp->tm_head) ? elp->tm_head->tv->tv_usec : 0);
#endif
		te->nxt     = elp->tm_head;
		elp->tm_head = te;
	}
	else
	{
		tr = elp->tm_head;
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
 * \param   t_elp    Loop Struct.
 * \lreturn t_elp_tm Timer event struct.
 * \return  void.
 * --------------------------------------------------------------------------*/
static inline void
t_elp_adjusttimer( struct t_elp *elp, struct timeval *ta )
{
	struct t_elp_tm *tr = elp->tm_head;
	while (NULL != tr)
	{
		t_tim_sub( tr->tv, ta, tr->tv );
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
static inline int
t_elp_getfunc( lua_State *luaVM, int refPos )
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
 * \lparam  userdata      T.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void 
t_elp_executetimer( lua_State *luaVM, struct t_elp *elp, struct timeval *rt )
{
	struct timeval  *tv;                  ///< timer returned by execution -> if there is
	
	struct t_elp_tm *te = elp->tm_head;   ///< timer to execute is tm_head, ALWAYS
	int    l;                             ///< length of arguments to call 
	elp->tm_head = elp->tm_head->nxt;
	l = t_elp_getfunc( luaVM, te->fR );
	lua_call( luaVM, l, 1 );
	t_tim_since( rt );
	t_elp_adjusttimer( elp, rt );
	tv = (struct timeval *) luaL_testudata( luaVM, -1, "T.Time" );
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
		t_elp_instimer( elp, te );
	}
	lua_pop( luaVM, 2 );   // pop the one value that lua_call allows to be
	                       // returned and the original reference table
}


/**--------------------------------------------------------------------------
 * Executes a handle event function for the file/socket handles
 * \param   luaVM         The lua state.
 * \param   struct xp_lp  Loop struct.
 * \lparam  userdata      t.Loop.
 * \return  void.
 * --------------------------------------------------------------------------*/
void
t_elp_executehandle( lua_State *luaVM, struct t_elp *elp, int fd )
{
	int n = t_elp_getfunc( luaVM, elp->fd_set[ fd ]->fR );
	lua_call( luaVM, n , 0 );
	lua_pop( luaVM, 1 );             // remove the table
}


/**--------------------------------------------------------------------------
 * Create a new t.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  int    initial size of fd_event slots in the Loop.
 * \lreturn struct t_elp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_elp_New( lua_State *luaVM )
{
	size_t                                 sz  = luaL_checkinteger( luaVM, 1 );
	struct t_elp __attribute__ ((unused)) *elp = t_elp_create_ud( luaVM, sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a t.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS table t.Loop.
 * \lreturn struct t_elp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lt_elp__Call( lua_State *luaVM )
{
	lua_remove( luaVM, 1 );
	return lt_elp_New( luaVM );
}


/**--------------------------------------------------------------------------
 * Create a new t_elp userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \param   size_t  how many slots for file/socket events to be created.
 * \return  struct t_elp * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct t_elp
*t_elp_create_ud( lua_State *luaVM, size_t sz )
{
	struct t_elp    *elp;

	elp = (struct t_elp *) lua_newuserdata( luaVM, sizeof( struct t_elp ) );
	elp->fd_sz   = sz;
	elp->mxfd    = 0;
	elp->tm_head = NULL;
	elp->fd_set  = (struct t_elp_fd **) malloc( elp->fd_sz * sizeof( struct t_elp_fd * ) );
	memset( elp->fd_set, 0, elp->fd_sz * sizeof( struct t_elp_fd * ) );
	t_elp_create_ud_impl( elp );
	luaL_getmetatable( luaVM, "T.Loop" );
	lua_setmetatable( luaVM, -2 );
	return elp;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_elp
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \return  struct t_elp*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_elp
*t_elp_check_ud ( lua_State *luaVM, int pos, int check )
{
	void *ud = luaL_checkudata( luaVM, pos, "T.Loop" );
	luaL_argcheck( luaVM, (ud != NULL  || !check), pos, "`T.Loop` expected" );
	return (NULL==ud) ? NULL : (struct t_elp *) ud;
}


/**--------------------------------------------------------------------------
 * Add an File/Socket event handler to the T.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata T.Loop.
 * \lparam  userdata handle.
 * \lparam  bool     shall this be treated as a reader?
 * \lparam  function to be executed when event handler fires.
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
int
lt_elp_addhandle( lua_State *luaVM )
{
	luaL_Stream   *lS;
	struct t_sck  *sc;
	int            fd  = 0;
	int            n   = lua_gettop( luaVM ) + 1;    ///< iterator for arguments
	struct t_elp  *elp = t_elp_check_ud( luaVM, 1, 1 );

	luaL_checktype( luaVM, 4, LUA_TFUNCTION );
	lS = (luaL_Stream *) luaL_testudata( luaVM, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = (struct t_sck *) luaL_testudata( luaVM, 2, "T.Socket" );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return t_push_error( luaVM, "Argument to addHandle must be file or socket" );

	elp->fd_set[ fd ] = (struct t_elp_fd *) malloc( sizeof( struct t_elp_fd ) );
	t_elp_addhandle_impl( elp, fd, lua_toboolean( luaVM, 3 ) );

	lua_createtable( luaVM, n-4, 0 );  // create function/parameter table
	lua_insert( luaVM, 4 );
	//Stack: elp,fd,read/write,TABLE,func,...
	while (n > 4)
		lua_rawseti( luaVM, 4, (n--)-4 );   // add arguments and function (pops each item)
	elp->fd_set[ fd ]->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // pop the function/parameter table
	lua_pop( luaVM, 1 ); // pop the read write boolean
	elp->fd_set[ fd ]->hR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // keep ref to handle so it doesnt gc

	return  0;
}


/**--------------------------------------------------------------------------
 * Remove a Handle event handler from the T.Loop.
 * \param   luaVM    The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata socket or file.                             // 2
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int
lt_elp_removehandle( lua_State *luaVM )
{
	luaL_Stream    *lS;
	struct t_sck   *sc;
	int             fd  = 0;
	struct t_elp   *elp = t_elp_check_ud( luaVM, 1, 1 );

	lS = (luaL_Stream *) luaL_testudata( luaVM, 2, LUA_FILEHANDLE );
	if (NULL != lS)
		fd = fileno( lS->f );

	sc = (struct t_sck *) luaL_testudata( luaVM, 2, "T.Socket" );
	if (NULL != sc)
		fd = sc->fd;

	if (0 == fd)
		return t_push_error( luaVM, "Argument to addHandle must be file or socket" );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ fd ]->fR );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ fd ]->hR );
	free( elp->fd_set[ fd ] );
	elp->fd_set[ fd ] = NULL;

	return 0;
}

/**--------------------------------------------------------------------------
 * Add a Timer event handler to the T.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  function to be executed when event handler fires.    // 3
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_elp_addtimer( lua_State *luaVM )
{
	struct t_elp    *elp = t_elp_check_ud( luaVM, 1, 1 );
	struct timeval  *tv  = t_tim_check_ud( luaVM, 2 );
	int              n   = lua_gettop( luaVM ) + 1;    ///< iterator for arguments
	struct t_elp_tm *te;

	luaL_checktype( luaVM, 3, LUA_TFUNCTION );
	// Build up the timer element
	te = (struct t_elp_tm *) malloc( sizeof( struct t_elp_tm ) );
	te->tv =  tv;
	//t_elp_addtimer_impl( elp, tv );
	lua_createtable( luaVM, n-3, 0 );  // create function/parameter table
	lua_insert( luaVM, 3 );
	// Stack: elp,tv,TABLE,func,...
	while (n > 3)
		lua_rawseti( luaVM, 3, (n--)-3 );            // add arguments and function (pops each item)
	te->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );  // pop the function/parameter table
	// making the time val part of lua registry guarantees the gc can't destroy it
	te->tR = luaL_ref( luaVM, LUA_REGISTRYINDEX );  // pop the timeval
	// insert into ordered linked list of time events
	t_elp_instimer( elp, te );

	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a Timer event handler from the T.Loop.
 * \param   luaVM    The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \return  #stack items returned by function call.
 * TODO: optimize!
 * --------------------------------------------------------------------------*/
static int
lt_elp_removetimer( lua_State *luaVM )
{
	struct t_elp    *elp = t_elp_check_ud( luaVM, 1, 1 );
	struct timeval  *tv  = t_tim_check_ud( luaVM, 2 );
	struct t_elp_tm *tp  = elp->tm_head;
	struct t_elp_tm *te  = tp->nxt;       ///< previous Timer event

	// if head is node in question
	if (NULL != tp  &&  tp->tv == tv)
	{
		elp->tm_head = te;
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
 * \lparam table   T.Loop.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int
lt_elp__gc( lua_State *luaVM )
{
	struct t_elp    *elp     = t_elp_check_ud( luaVM, 1, 1 );
	struct t_elp_tm *tf, *tr = elp->tm_head;
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
	for (i=0; i < elp->fd_sz; i++)
	{
		if (NULL != elp->fd_set[ i ])
		{
			luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ i ]->fR );
			luaL_unref( luaVM, LUA_REGISTRYINDEX, elp->fd_set[ i ]->hR );
			free( elp->fd_set[ i ] );
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Set up a select call for all events in the T.Loop
 * \param   luaVM  The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_elp_run( lua_State *luaVM )
{
	struct t_elp    *elp = t_elp_check_ud( luaVM, 1, 1 );
	elp->run = 1;

	while (elp->run)
	{
		if (t_elp_poll_impl( luaVM, elp ) < 0)
			return 0;
		// if there are no events left in the loop stop processing
		elp->run =  (NULL==elp->tm_head && elp->mxfd<1) ? 0 : elp->run;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Stop an T.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata T.Loop.                                    // 1
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int
lt_elp_stop( lua_State *luaVM )
{
	struct t_elp    *elp = t_elp_check_ud( luaVM, 1, 1 );
	elp->run = 0;
	return 0;
}

/**--------------------------------------------------------------------------
 * Prints out the Loop.
 * \param   luaVM     The lua state.
 * \lparam  userdata  T.Loop userdata
 * \lreturn string    formatted string representing T.Loop.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
lt_elp__tostring( lua_State *luaVM )
{
	struct t_elp *elp = t_elp_check_ud( luaVM, 1, 1 );
	lua_pushfstring( luaVM, "T.Loop(select){%d}: %p", elp->mxfd, elp );
	return 1;
}


/**--------------------------------------------------------------------------
 * Debug print for loops.
 * \param   t_elp    Loop Struct.
 * \return  int #elements returned to function call(Stack)
 * --------------------------------------------------------------------------*/
static int
lt_elp_showloop( lua_State *luaVM )
{
	struct t_elp    *elp = t_elp_check_ud( luaVM, 1, 1 );
	struct t_elp_tm *tr  = elp->tm_head;
	int              i   = 0;
	int              n   = lua_gettop( luaVM );
	printf( "LOOP %p TIMER LIST:\n", elp );
	while (NULL != tr)
	{
		printf("\t%d\t{%2ld:%6ld}\t%p   ", ++i,
			tr->tv->tv_sec,  tr->tv->tv_usec,
			tr->tv);
		t_elp_getfunc( luaVM, tr->fR );
		t_stackPrint( luaVM, n+1, lua_gettop( luaVM ) );
		lua_pop( luaVM, lua_gettop( luaVM ) - n );
		printf("\n");
		tr = tr->nxt;
	}
	printf( "LOOP %p HANDLE LIST:\n", elp );
	for( i=0; i<elp->mxfd+1; i++)
	{
		if (NULL==elp->fd_set[ i ])
			continue;
		printf("\t%d\t%s   ", i,
			(t_elp_READ == elp->fd_set[i]->t) ? "READER" : "WRITER");
		t_elp_getfunc( luaVM, elp->fd_set[i]->fR );
		t_stackPrint( luaVM, n+1, lua_gettop( luaVM ) );
		lua_pop( luaVM, lua_gettop( luaVM ) - n );
		printf("\n");
	}
	return 0;
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg t_elp_fm [] =
{
	{"__call",    lt_elp__Call},
	{NULL,   NULL}
};

/**
 * \brief      the Time library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg t_elp_cf [] =
{
	{"new",       lt_elp_New},
	{NULL,        NULL}
};


/**
 * \brief      the Timer library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg t_elp_m [] =
{
	{"addTimer",       lt_elp_addtimer},
	{"removeTimer",    lt_elp_removetimer},
	{"addHandle",      lt_elp_addhandle},
	{"removeHandle",   lt_elp_removehandle},
	{"run",            lt_elp_run},
	{"stop",           lt_elp_stop},
	{"show",           lt_elp_showloop},
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
LUA_API int
luaopen_t_elp( lua_State *luaVM )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( luaVM, "T.Loop" );   // stack: functions meta
	luaL_newlib( luaVM, t_elp_m );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lt_elp__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lt_elp__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack

	// Push the class onto the stack
	luaL_newlib( luaVM, t_elp_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( luaVM, t_elp_fm );
	lua_setmetatable( luaVM, -2 );
	return 1;
}
