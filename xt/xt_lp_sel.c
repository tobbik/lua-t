// Select specific implementation of Loop features
//
//
//
#include "l_xt.h"
#include "xt_lp.h"
#include "xt_time.h"


/**--------------------------------------------------------------------------
 * construct a xt.Loop and return it.
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
 * create a new xt.Loop and return it.
 * \param   luaVM  The lua state.
 * \lparam  int    initial size of fd_event slots in the Loop.
 * \lreturn struct xt_lp userdata.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
int lxt_lp_New( lua_State *luaVM )
{
	size_t                                   sz = luaL_checkint( luaVM, 1 );
	struct xt_lp  __attribute__ ((unused))  *lp = xt_lp_create_ud( luaVM, sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new xt_lp userdata and push to LuaStack.
 * \param   luaVM  The lua state.
 * \return  struct xt_lp * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct xt_lp *xt_lp_create_ud( lua_State *luaVM, size_t sz )
{
	struct xt_lp    *lp;

	lp = (struct xt_lp *) lua_newuserdata( luaVM, sizeof( struct xt_lp ) );
	lp->fd_sz   = sz;
	lp->mxfd    = 0;
	lp->tm_head = NULL;
	lp->fd_set  = malloc( lp->fd_sz * sizeof( struct xt_lp_f * ) );
	FD_ZERO( &lp->rfds );
	FD_ZERO( &lp->wfds );
	FD_ZERO( &lp->rfds_w );
	FD_ZERO( &lp->wfds_w );
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
	if (lua_toboolean( luaVM, 3 ))
	{
		lp->fd_set[ fd ]->t = XT_LP_READ;
		FD_SET( fd, &lp->rfds );
	}
	else
	{
		lp->fd_set[ fd ]->t = XT_LP_WRIT;
		FD_SET( fd, &lp->wfds );
	}
	lp->mxfd = (fd > lp->mxfd) ? fd : lp->mxfd;

	lua_createtable( luaVM, n-4, 0 );  // create function/parameter table
	lua_insert( luaVM, 4 );
	//Stack: lp,tv,rp,TABLE,func,...
	while (n > 4)
		lua_rawseti( luaVM, 4, (n--)-4 );   // add arguments and function
	lp->fd_set[ fd ]->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // pop the function/parameter table

	return  0;

}


/**--------------------------------------------------------------------------
 * Add an Timer event handler to the xt.Loop.
 * \param   luaVM  The lua state.
 * \lparam  userdata xt.Loop.                                    // 1
 * \lparam  userdata timeval.                                    // 2
 * \lparam  bool     shall this be treated as an interval?       // 3
 * \lparam  function to be executed when event handler fires.    // 4
 * \lparam  ...    parameters to function when executed.
 * \return  #stack items returned by function call.
 * --------------------------------------------------------------------------*/
static int lxt_lp_addtimer( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	struct timeval  *tv = xt_time_check_ud( luaVM, 2 );
	int              r  = lua_toboolean( luaVM, 3 );
	int              n  = lua_gettop( luaVM ) + 1;    ///< iterator for arguments
	struct xt_lp_tm *te;
	struct xt_lp_tm *tr;  ///< Time event runner for Linked List iteration

	luaL_checktype( luaVM, 4, LUA_TFUNCTION );
	// Build up the timer element
	te = (struct xt_lp_tm *) malloc( sizeof( struct xt_lp_tm ) );
	te->tv = *tv;
	te->it = (r) ? tv : NULL;
	te->t  = (r) ? XT_LP_MULT : XT_LP_ONCE;
	te->id = ++lp->mxfd;
	lua_createtable( luaVM, n-4, 0 );  // create function/parameter table
	lua_insert( luaVM, 4 );
	//Stack: lp,tv,rp,TABLE,func,...
	while (n > 4)
		lua_rawseti( luaVM, 4, (n--)-4 );   // add arguments and function
	te->fR = luaL_ref( luaVM, LUA_REGISTRYINDEX );      // pop the function/parameter table
	// insert into ordered linked list of time events
	if (NULL == lp->tm_head)
		lp->tm_head = te;
	else
	{
		tr = lp->tm_head;
		while (NULL != tr->nxt && xt_time_cmp( &tr->tv, &te->tv ))
			tr = tr->nxt;
		te->nxt = tr->nxt;
		tr->nxt = te;
	}

	return 1;
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
	int              i,n,r;
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	struct xt_lp_tm *te;
	lp->run=1;

	while (lp->run)
	{
		if (NULL != lp->tm_head)
		{
			te = lp->tm_head;
			//printf("%d   %d  \n", te->id, te->fR);
			lp->tm_head = lp->tm_head->nxt;
		}

		memcpy( &lp->rfds_w, &lp->rfds, sizeof( fd_set ) );
		memcpy( &lp->wfds_w, &lp->wfds, sizeof( fd_set ) );

		r = select( lp->mxfd, &lp->rfds_w, &lp->wfds_w, NULL, &te->tv );
		printf("%d\n", r);

		if (0==r) // deal with timer
		{
			// unpack and execute func/parm table
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, te->fR );
			n = lua_rawlen( luaVM, 2 );
			for (i=0; i<n; i++)
				lua_rawgeti( luaVM, 2, i+1 );
			lua_remove( luaVM, 2 );             // remove the table
			lua_call( luaVM, n-1, LUA_MULTRET );
		}
		else
		{
			for( i=0; r>0 && i < lp->mxfd; i++ )
			{
				if (NULL==lp->fd_set[ i ])
					continue;
				else
				{
					r--;
					lua_rawgeti( luaVM, LUA_REGISTRYINDEX, lp->fd_set[ i ]->fR );
					n = lua_rawlen( luaVM, 2 );
					for (i=0; i<n; i++)
						lua_rawgeti( luaVM, 2, i+1 );
					lua_remove( luaVM, 2 );             // remove the table
					lua_call( luaVM, n-1, LUA_MULTRET );
				}
			}
		}

	}

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
 * Garbage Collector. Free events in allocated spots.
 * \param  luaVM   lua Virtual Machine.
 * \lparam table   xt.Loop.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_lp__gc( lua_State *luaVM )
{
	struct xt_lp    *lp = xt_lp_check_ud( luaVM, 1 );
	//struct xt_lp_fd *f;
	struct xt_lp_tm *t;
	size_t           i;       ///< the iterator for all fields

	while (NULL != lp->tm_head)
	{
		t = lp->tm_head;
		luaL_unref( luaVM, LUA_REGISTRYINDEX, t->fR ); // remove func/arg table from registry
		lp->tm_head = t->nxt;
		free( t );
	}
	for (i=0; i < lp->fd_sz; i++)
	{
		if (NULL != lp->fd_set[ i ])
		{
			free( lp->fd_set[ i ] );
		}
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
	{"addHandle",      lxt_lp_addhandle},
	{"run",            lxt_lp_run},
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
