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

#include <sys/time.h>             // timersub

#include "t_net.h"
#include "t_ael_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

#define timesub(a, b, result)                           \
	do {                                                 \
	  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;      \
	  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;   \
	  if ((result)->tv_usec < 0) {                       \
	    --(result)->tv_sec;                              \
	    (result)->tv_usec += 1000000;                    \
	  }                                                  \
	} while (0)

/**----------------------------------------------------------------------------
 * Get descriptor handle from the stack.
 * Discriminate if Socket or file handle
 * \param   L     Lua state.
 * \param   pos   int; Reference positon on the stack.
 * \param   exc   int; check for type safety
 * \return  check Number of arguments to be called by function.
 * \return  int   Descriptor number.
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
 * Takes refPosition and gets table onto the stack.  Executes function.
 * Stack before: {fnc, p1, p2, p3, … }
 * Stack after:   fnc  p1  p2  p3  …
 * \param   L     Lua state.
 * \param   pos   int; Reference positon on the stack.
 * \param   exc   int; Should it be executed?
 * \return  int   Number of arguments to be called by function.
 * --------------------------------------------------------------------------*/
void
t_ael_doFunction( lua_State *L, int exc )
{
	int n;      ///< number of arguments + 1(function)
	int p;      ///< position of pickled function and args
	int i;

	if (lua_isnil( L, -1 ))
		lua_pop( L, 1);
	else
	{
		p = lua_gettop( L );
		n = lua_rawlen( L, p );
		for (i=0; i<n; i++)
			lua_rawgeti( L, p, i+1 );              //S: … tbl fnc arg arg arg
		lua_remove( L, p );  // remove table      //S: …     fnc arg arg arg
		if (exc > -1)
			lua_call( L, n-1, exc );
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

	ael = (struct t_ael *) lua_newuserdatauv( L, sizeof( struct t_ael ), 3 );
	ael->fdCount = 0;
	ael->tout    = T_AEL_NOTIMEOUT;
	lua_newtable( L );                               //S: ael tbl
	lua_setiuservalue( L, 1, T_AEL_DSCIDX );         //S; ael
	p_ael_create_ud_impl( L );                       //S: ael ste
	lua_setiuservalue( L, -2, T_AEL_STEIDX );        //S: ael
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
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );      //S: ael hdl dir fnc …
	int               fd  = t_ael_getHandle( L, 2, 1 );     //S: ael hdl dir fnc …
	struct t_ael_dnd *dnd;
	int               n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	enum t_ael_msk    msk;

	luaL_argcheck( L, t_getLoadedValue( L, 1, 3, "t."T_AEL_IDNT ),
	      3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );                        //S: ael hdl msk fnc …
	luaL_checktype( L, 4, LUA_TFUNCTION );

	// get/create dnd userdata
	lua_getiuservalue( L, 1, T_AEL_DSCIDX );     //S: ael hdl msk fnc … nds
	//printf("FD: %d  ", fd); t_stackDump(L);
	lua_rawgeti( L, -1, fd );                    //S: ael hdl msk fnc … nds ???
	if (lua_isnil( L, -1 ))
	{
		lua_pop( L, 1 );
		dnd = t_ael_dnd_create_ud( L );           //S: ael hdl msk fnc … nds dnd
		lua_pushvalue( L, -1 );                   //S: ael hdl msk fnc … nds dnd dnd
		lua_rawseti( L, -3, fd );                 //S: ael hdl msk fnc … nds dnd
		(ael->fdCount)++;
	}
	else
		dnd = t_ael_dnd_check_ud( L, -1, 1 );     //S: ael hdl msk fnc … nds dnd
	lua_rotate( L, 2, 1 );                       //S: ael dnd hdl msk fnc … nds
	lua_pop( L, 1 );                             //S: ael dnd hdl msk fnc …

	// implementation specific handling
	p_ael_addhandle_impl( L, 1, dnd, fd, msk );

	// create function reference
	lua_createtable( L, n-4, 0 );                //S: ael dnd hdl msk fnc … tbl
	lua_rotate( L, 5, 1 );                       //S: ael dnd hdl msk tbl fnc …
	while (n > 4)
		lua_rawseti( L, 5, (n--)-4 );             // add arguments and function (pops each item)
                                                //S: ael dnd hdl msk tbl
	dnd->msk    |= msk;
	if (T_AEL_RD & msk)                          //S: ael dnd hdl msk tbl
		lua_setiuservalue( L, 2, T_AEL_DSC_FRDIDX );
	else
		lua_setiuservalue( L, 2, T_AEL_DSC_FWRIDX );

	lua_pop( L, 1 );                             //S: ael dnd hdl       // pop the msk
	lua_setiuservalue( L, -2, T_AEL_DSC_HDLIDX );//S: ael dnd hdl       // hdl uservalue of dnd

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
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );   //S: ael hdl dir
	int                fd = t_ael_getHandle( L, 2, 1 );  //S: ael hdl dir
	struct t_ael_dnd *dnd;
	enum t_ael_msk    msk;

	luaL_argcheck( L, t_getLoadedValue( L, 1, 3, "t."T_AEL_IDNT ),
	      3, "must specify direction" );
	msk = luaL_checkinteger( L, 3 );                     //S: ael hdl msk

	// get dnd userdata
	lua_getiuservalue( L, 1, T_AEL_DSCIDX );             //S: ael hdl msk nds
	lua_rawgeti( L, -1, fd );
	if (lua_isnil( L, -1 ))
	{
		lua_pushboolean( L, 0 );
		lua_pushstring( L, "Descriptor not observed in Loop -> ignoring" );
		return 2;
		//return luaL_error( L, "Descriptor must be observed in Loop" );
	}
	else
		dnd = t_ael_dnd_check_ud( L, -1, 1 );             //S: ael hnd msk nds dnd

	p_ael_removehandle_impl( L, 1, dnd, fd, msk );       //S: ael hnd msk nds dnd

	dnd->msk = dnd->msk & (~msk);
	lua_pushnil( L );                                    //S: ael hnd msk nds dnd nil
	if (T_AEL_NO != dnd->msk)
		// if dnd->msk still writeable then set read function to nil!
		lua_setiuservalue( L, -1 , (T_AEL_WR & dnd->msk) ? T_AEL_DSC_FRDIDX : T_AEL_DSC_FWRIDX );
	else
	{
		lua_rawseti( L, -3, fd );                         //S: ael hnd msk nds dnd
		(ael->fdCount)--;
	}
	lua_pop( L, 2 );                                     //S: ael hnd msk
	return 0;
}


/**--------------------------------------------------------------------------
 * Create a Task handler, add to T.Loop.
 * \param   L   Lua state.
 * \lparam  ael t_ael; T.Loop userdata instance.                   // 1
 * \lparam  ms  int; milliseconds until execution                  // 2
 * \lparam  fnc function; to be executed when event handler fires. // 3
 * \lparam  …   parameters to function when executed.              // 4 …
 * \return  int # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_addtask( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );  //S: ael ms fnc …
	int               n   = lua_gettop( L ) + 1;    ///< iterator for arguments
	struct t_ael_tsk *tsk = t_ael_tsk_create_ud( L, luaL_checkinteger( L, 2 ) ); //S: ael ms fnc … tsk

	lua_replace( L, 2 );                         //S: ael tsk fnc …
	luaL_checktype( L, 3, LUA_TFUNCTION );
	lua_createtable( L, n-3, 0 );                //S: ael tsk fnc … tbl
	lua_rotate( L, 3, 1 );                       //S: ael tsk tbl fnc …
	while (n > 3)      // add args and fnc (pops each item) reversely (fnc is last)
		lua_rawseti( L, 3, (n--)-3 );
	                                             //S: ael tsk tbl
	lua_setiuservalue( L, -2, T_AEL_TSK_FNCIDX );//S: ael tsk
	lua_pushvalue( L, -1 );                      //S: ael tsk tsk
	lua_rotate( L, -3, 1 );                      //S: tsk ael tsk
	t_ael_tsk_insert( L, ael, tsk );             //S: tsk ael
	lua_rotate( L, -2, 1 );                      //S: ael tsk

	return 1;
}


/**--------------------------------------------------------------------------
 * Remove a Timer event handler from the T.Loop.
 * \param   L    Lua state.
 * \lparam  ud   T.Loop userdata instance.                   // 1
 * \lparam  ud   T.Loop.Task userdata instance.              // 2
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_canceltask( lua_State *L )
{
	struct t_ael      *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tsk *tCnd = t_ael_tsk_check_ud( L, 2, 1 ); //S: ael cnd

	t_ael_tsk_remove( L, ael, tCnd );                        //S: ael cnd
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
	printf("Running AEL __gc\n");

	// walk down nodes table an unref functions and handles
	lua_getiuservalue( L, 1, T_AEL_DSCIDX );     //S: ael nds
	p_ael_free_impl( L, 1 );                     //S: ael nds
	ael->fdCount = 0;
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
	struct t_ael      *ael = t_ael_check_ud( L, 1, 1 );
	struct timeval tvs,tve;      ///< measure time for one iteration
	int                  n;      ///< how many file events?

	ael->run                = 1;
	while (ael->run)
	{
		gettimeofday( &tvs, 0 );

		if ((n = p_ael_poll_impl( L, ael->tout, 1 )) < 0)          //S: ael
			return t_push_error( L, 1, 1, "Failed to continue the loop" );

#if PRINT_DEBUGS == 3
		printf( "oooooooooooooooooooooo POLL RETURNED: %d oooooooooooooooooooo\n", n );
#endif

		gettimeofday( &tve, 0 );

		// execute timer events
		timesub( &tve, &tvs, &tve );
		t_ael_tsk_process( L, ael, (tve.tv_sec*1000 + tve.tv_usec/1000) );

		// if there are no events left in the loop -> stop processing
		//printf("RUN__ed: %lld -- ", ael->tout); t_stackDump(L);
		ael->run = (ael->tout == T_AEL_NOTIMEOUT && ael->fdCount < 1) ? 0 : ael->run;
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
 * \lreturn string formatted string representing T.Loop.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael__tostring( lua_State *L )
{
	struct t_ael *ael = t_ael_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_AEL_TYPE"{%d}[%d]: %p", ael->fdCount, ael->tout, ael );
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
#ifdef DEBUG          //t_stackPrint is t_dbg
static int
lt_ael_showloop( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_tsk *tsk;
	struct t_ael_dnd *dnd;
	int               i   = 0;
	int               n   = lua_gettop( L );
	int               fd;
	int               fail=40;

	printf( T_AEL_TYPE"{%d}[%lld]: %p TIMER LIST:\n", ael->fdCount, ael->tout, ael );
	lua_getiuservalue( L, 1 ,T_AEL_TSKIDX );         //S: ael tsk
	while (! lua_isnil( L, -1 ) && fail-- > 0)
	{
		tsk = t_ael_tsk_check_ud( L, -1, 0 );
		printf( "\t%d\t{%5lld}  ", ++i, tsk->tout );
		lua_getiuservalue( L, -1, T_AEL_TSK_FNCIDX ); //S: ael tsk tbl
		t_ael_doFunction( L, -1 );
		t_stackPrint( L, n+1, lua_gettop( L ), 0 );   //S: ael tsk fnc …
		printf( "\n" );
		lua_getiuservalue( L, 2, T_AEL_TSK_NXTIDX );  //S: ael prv fnc … nxt
		lua_rotate( L, 2, 1 );                        //S: ael nxt prv fnc …
		lua_pop( L, lua_gettop( L ) - 2 );            //S: ael nxt
	}
	lua_pop( L, 1 );
	printf( T_AEL_TYPE" %p HANDLE LIST:\n", ael );
	lua_getiuservalue( L, 1, T_AEL_DSCIDX );         //S: ael nds
	lua_pushnil( L );
	n = lua_gettop(L);
	while (lua_next( L, -2 ))
	{
		dnd = t_ael_dnd_check_ud( L, -1, 1 );
		fd  = luaL_checkinteger( L, -2 );
		if (T_AEL_NO == dnd->msk)
			continue;
		if (T_AEL_RD & dnd->msk)
		{
			printf( "%5d  [R]  ", fd );
			lua_getiuservalue( L, -1, T_AEL_DSC_FRDIDX );     //S: ael dnd tbl
			t_ael_doFunction( L, -1 );                        //S: ael dnd fnc …
			t_stackPrint( L, n+2, lua_gettop( L ), 0 );
			lua_pop( L, lua_gettop( L ) - n -1 );
			printf( "\n" );
		}
		if (T_AEL_WR & dnd->msk)
		{
			printf( "%5d  [W]  ", fd );
			lua_getiuservalue( L, -1, T_AEL_DSC_FWRIDX );     //S: ael dnd tbl
			t_ael_doFunction( L, -1 );                        //S: ael dnd fnc …
			t_stackPrint( L, n+2, lua_gettop( L ), 0 );
			lua_pop( L, lua_gettop( L ) - n -1 );
			printf( "\n" );
		}
		lua_pop( L, 1 );
	}
	return 0;
}
#endif


/**--------------------------------------------------------------------------
 * Revoe every item and reference from the loop.
 * \param   t_ael    Loop Struct.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ael_clean( lua_State *L )
{
	struct t_ael     *ael = t_ael_check_ud( L, 1, 1 );
	struct t_ael_dnd *dnd;
	//int               i   = 0;
	//int               n   = lua_gettop( L );

	printf( T_AEL_TYPE" %p cleaning TIMER LIST:\n", ael );
	lua_pushnil( L );
	lua_setiuservalue( L, 1, T_AEL_TSKIDX );
	ael->tout = T_AEL_NOTIMEOUT;

	// walk down nodes table an unref functions and handles
	printf( T_AEL_TYPE" %p cleaning HANDLE LIST:\n", ael );
	lua_getiuservalue( L, 1, T_AEL_DSCIDX );         //S: ael nds
	lua_pushnil( L );
	while (lua_next( L, -2 ))
	{
		dnd = t_ael_dnd_check_ud( L, -1, 1 );
		t_stackDump(L);
		p_ael_removehandle_impl( L, 1, dnd, luaL_checkinteger( L, -2 ), T_AEL_RW );
		lua_pushnil( L );
		lua_rawseti( L, -4, luaL_checkinteger( L, -3 ) );
		(ael->fdCount)--;
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
	struct t_ael __attribute__ ((unused)) *ael = t_ael_check_ud( L, 1, 1 );
	int               fd  = 0;

	if (LUA_TSTRING == lua_type( L, 2 )) // return method: run, stop, addHandle, …
	{
		lua_getmetatable( L, 1 );
		lua_pushvalue( L, 2 );
		lua_gettable( L, -2 );
	}
	else
	{
	   lua_getiuservalue( L, 1, T_AEL_DSCIDX );         //S: ael idx nds
		fd = t_ael_getHandle( L, 2, 0 );
		if (! fd)  // last chance, it might be a t.Loop.Task
		{
			lua_pushvalue( L, 2 );
			lua_rawget( L, -2 );
		}
		else
			lua_rawgeti( L, -1, fd );                     //S: ael idx nds dnd
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
 * Instance metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ael_m [] = {
	// metamethods
	  { "__tostring",    lt_ael__tostring     }
	, { "__len",         lt_ael__len          }
	, { "__gc",          lt_ael__gc           }
	, { "__index",       lt_ael__index        }
	// instance methods
	, { "addTask",       lt_ael_addtask       }
	, { "cancelTask",    lt_ael_canceltask    }
	, { "addHandle",     lt_ael_addhandle     }
	, { "removeHandle",  lt_ael_removehandle  }
	, { "run",           lt_ael_run           }
	, { "stop",          lt_ael_stop          }
	, { "clean",         lt_ael_clean         }
#ifdef DEBUG
	, { "show",          lt_ael_showloop      }
#endif
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
luaopen_t_ael( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	luaL_newmetatable( L, T_AEL_TYPE );           // stack: functions meta
	luaL_setfuncs( L, t_ael_m, 0 );
	luaopen_t_ael_dnd( L );
	lua_setfield( L, -2, T_AEL_DND_NAME );
	luaopen_t_ael_tsk( L );
	lua_setfield( L, -2, T_AEL_TSK_NAME );

	// Push the class onto the stack
	luaL_newlib( L, t_ael_cf );
	// directions
	lua_pushinteger( L, T_AEL_RD );     // Observe handle only for being readable
	lua_setfield( L, -2, "READ" );
	lua_pushstring( L, "READ" );
	lua_rawseti( L, -2, T_AEL_RD );
	lua_pushinteger( L, T_AEL_WR );
	lua_setfield( L, -2, "WRITE" );     // Observe handle only for being writable
	lua_pushstring( L, "WRITE" );
	lua_rawseti( L, -2, T_AEL_WR );
	lua_pushinteger( L, T_AEL_RW );     // Observe handle only for being readable AND writable
	lua_setfield( L, -2, "READWRITE" );
	lua_pushstring( L, "READWRITE" );
	lua_rawseti( L, -2, T_AEL_RW );

	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_ael_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
