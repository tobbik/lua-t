/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tst.c
 * \brief     lua-t unit testing framework (T.Test)
 *            provides T.Test and T.Test.Case
 *            implemented as Lua Table
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <string.h>            // strlen
#include <strings.h>           // strncasecmp
#include <stdlib.h>            // malloc

#include "t.h"
#include "t_tim.h"

#define T_TST_CAS_NAME     "Case"

// forward declaration eliminates need for header file
// l_t_tst.c
static void t_tst_check_ud( lua_State *L, int pos);


/**--------------------------------------------------------------------------
 * push a string formatting a stack element
 * \param   L  The lua state.
 * \param   pos    integer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static void
fmt_stack_item( lua_State *L, int pos)
{
	int t;
	if (!luaL_callmeta( L, pos, "__tostring" ))
	{
		t = lua_type( L, pos );
		switch (t)
		{
			case LUA_TSTRING:    /* strings */
				lua_pushvalue( L, pos );
				break;
			case LUA_TBOOLEAN:   /* booleans */
				if (lua_toboolean( L, pos )) lua_pushliteral( L, "true" );
				else lua_pushliteral( L, "false" );
				break;
			case LUA_TNUMBER:    /* numbers */
				lua_pushfstring( L, "%f", lua_tonumber( L, pos ) );
				break;
			default:	            /* other values */
				lua_pushfstring( L, "%s", lua_typename( L, pos ) );
				break;
		}
	}
}


/**--------------------------------------------------------------------------
 * construct a Test and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table Test
 * \lparam  string name of the unit test
 * \lparam  string description of the unit test
 * \lreturn table  T.Test Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_tst__Call( lua_State *L )
{
	lua_remove( L, 1 );
	lua_newtable( L );
	if( lua_gettop( L ) ==2 && lua_isstring( L, 2 ) )
		lua_pushvalue( L, 2 );
	else
		lua_pushliteral( L, "anonymous" );
	lua_setfield( L, -2 , "_suitename" );
	luaL_getmetatable( L, T_TST_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   L        Lua state.
 * \param   int      position on the stack.
 * \lparam  table    T.Test Lua table instance.
 * \lreturn table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
void
t_tst_check_ud( lua_State *L, int pos )
{
	luaL_checktype( L, pos, LUA_TTABLE );
   if (lua_getmetatable( L, pos ))  /* does it have a metatable? */
	{
		luaL_getmetatable( L, T_TST_TYPE );   /* get correct metatable */
		if (!lua_rawequal( L, -1, -2 ))      /* not the same? */
			t_push_error( L, "wrong argument, `"T_TST_TYPE"` expected" );
		lua_pop( L, 2);
	}
	else
		t_push_error( L, "wrong argument, `"T_TST_TYPE"` expected" );
}


/**--------------------------------------------------------------------------
 * Creates a traceback from a function call
 * \param   L    The lua state.
 * \lparam  either assert result table or generig string
 * \lreturn a t_tst result Failure description table
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
traceback( lua_State *L )
{
	if (LUA_TSTRING == lua_type( L, 1 ))
	{
		lua_newtable( L );
		lua_insert( L, 1 );
		lua_setfield( L, 1, "message" );
	}
	if (LUA_TTABLE == lua_type( L, 1 ))
	{
		luaL_where( L, 2 );
		lua_setfield( L, 1, "location" );
		luaL_traceback( L, L, NULL, 1 );
		lua_setfield( L, 1, "traceback" );
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * Executes the test suite.
 * \param   L    The lua state.
 * \lparam  test suite instance table.
 * \lreturn boolean true if all passed, otherwise false
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst__call( lua_State *L )
{
	size_t          all  = 1,
	                pass = 0,
	                skip = 0;
	struct timeval *tm;

	t_tst_check_ud ( L, 1 );
	lua_pushstring( L, "_time" );
	tm =  t_tim_create_ud( L, 0 );          // Stack: 2
	lua_rawset( L, 1 );

	lua_getfield( L, 1, "setUp" );
	lua_pushvalue( L, 1 );
	if (lua_pcall( L, 1, 0, 0 ))
	{
		t_push_error( L, "Test setup failed %s", lua_tostring( L, -1 ) );
	}
	for ( all=1 ;all < lua_rawlen( L, 1 )+1 ; all++ )
	{
		lua_rawgeti(L, 1, all );
		lua_getfield( L, 2, "skip" );
		skip =( lua_isnoneornil( L, -1 )) ? skip : skip+1;
		lua_pop( L, 1 );
		luaL_getmetafield( L, -1, "__call" );
		lua_pushvalue( L, -2 );         // push table with test function (has __call method)
		lua_pushvalue( L, 1 );          // push test suite table (aka. 'self' in test functions
		lua_pushinteger( L, all );
		lua_call( L, 3, 1 );
		pass =( lua_toboolean( L, -1 )) ? pass+1 : pass;
		lua_pop( L, 2 );     // pop the test case table and return result
	}
	lua_getfield( L, 1, "tearDown" );
	lua_pushvalue( L, 1 );
	if (lua_pcall( L, 1, 0, 0 ))
		t_push_error( L, "Test tearDown failed %s", lua_tostring( L, -1 ) );

	t_tim_since( tm );
	--all;
	printf( "---------------------------------------------------------\n"
	        "Executed %lu tests in %03f seconds\n\n"
	        "Skipped : %lu\n"
	        "Failed  : %lu\n"
	        "status  : %s\n",
	   all, t_tim_getms(tm)/1000.0,
		skip,
		all-pass,
		(all==pass)? "OK":"FAIL" );

	lua_pushboolean( L, (all==pass) ? 1 : 0 );

	return 1;
}


/**----------------------------------------------------------------------------
 * Inspects source for special lines in the comments
 * Use lua debug facilities to determine lines of code and read the code from
 * the source files
 *   - extracts #TODO, #SKIP and #DESC from the comments
 * \param  L The Lua state.
 * --------------------------------------------------------------------------*/
static void
t_get_fn_source( lua_State *L )
{
	lua_Debug  ar;
	FILE      *f;
	int        r  = 0; ///< current line count
	char      *fnd;
	char      *p;
	luaL_Buffer lB;
	luaL_buffinit( L, &lB );
	lua_State *L1 = L;
	lua_pushstring( L, ">S" );
	lua_pushvalue( L, 3 );
	lua_xmove( L, L1, 1 );
	lua_getinfo( L1, ">S", &ar );
	lua_pop( L, 1 ); // pop the ">S"
	lua_pushinteger( L, ar.linedefined );
	lua_setfield( L, -2, "line" );

	f = fopen( ar.short_src, "r" );
	while (r < ar.lastlinedefined)
	{
		p = luaL_prepbuffer( &lB );
		sprintf( p, "\n    %4d: ", r+1 );
		if (fgets( p+11, LUAL_BUFFERSIZE, f ) == NULL)
			break;  // eof?
		//TODO: reasonable line end check
		if (++r < ar.linedefined)
			continue;
		luaL_addsize( &lB, strlen( p )-1 );
		if (NULL != (fnd=strstr( p, "-- #TODO:" )))
		{
			lua_pushlstring( L, fnd+9, strlen( fnd+9 )-1 );
			lua_setfield( L, -3, "todo" );
		}
		if (NULL != (fnd=strstr( p, "-- #SKIP:" )))
		{
			lua_pushlstring( L, fnd+9, strlen( fnd+9 )-1 );
			lua_setfield( L, -3, "skip" );
		}
		if (NULL != (fnd=strstr( p,"-- #DESC:" )))
		{
			lua_pushlstring( L, fnd+9, strlen( fnd+9 )-1 );
			lua_setfield( L, -3, "desc" );
		}
	}
	luaL_pushresult( &lB );
	lua_setfield( L, -2, "src" );

	fclose (f);
}


/**--------------------------------------------------------------------------
 * Gets called for assignement of variables to test instance
 * This covers three jobs:
 *  - make sure no one adds numeric elements (internal test order)
 *  - no overwrite or create of _* functions
 *  - append ^test_* named methods to internal test order
 * \param   L    The lua state.
 * \lparam  table test instance
 * \lparam  key   string
 * \lparam  value LuaType
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst__newindex( lua_State *L )
{
	const char *name;
	t_tst_check_ud( L, 1 );                        // on Stack [1]
	name = luaL_checkstring( L, 2 );               // on Stack [2]

	// throw out illegal assignments
	if (lua_isnumber( L, 2 ))
	{
		return t_push_error( L, "Can't overwrite or create numeric indices" );
	}
	if ('_' == *name)
	{
		return t_push_error( L, "Can't overwrite or create internal test methods" );
	}

	// insert a testcase
	if (0==strncasecmp( name, "test", 4 ))
	{
		// assigning a new test
		if (lua_isfunction( L, 3 ))
		{
			// TODO: find a more elegant way without that much stack gymnastics
			lua_newtable( L );                // Stack 4: empty table
			t_get_fn_source( L );
			luaL_getmetatable( L, T_TST_TYPE"."T_TST_CAS_NAME );
			lua_setmetatable( L, 4 );

			lua_pushvalue( L , 2 );           // copy name from 2 to 5
			lua_setfield( L, 4, "name" );

			lua_insert( L, 3 );               // move new table to Stack 3
			lua_setfield( L, 3, "f" );

			lua_pushvalue( L, 3 );            // make copy of table for test [n]
			lua_rawseti( L,   1, lua_rawlen(L, 1 ) +1 ); // insert as test [n]
		}
		else
		{
			return t_push_error( L, "test* named elements must be methods" );
		}
	}
	// insert as test.name
	lua_rawset( L, 1 );

	return 0;
}


/** ---------------------------------------------------------------------------
 * Compares the last two values on the stack (deep table compare; recursive)
 * Work on negative inices ONLY for recursive use
 * \param   L lua_State
 * \lparam  value 1
 * \lparam  value 2
 * \return  boolean int 1 or 0
 * TODO: push an expressive error message onto the stack
 *--------------------------------------------------------------------------- */
static int
is_really_equal( lua_State *L )
{
	// if lua considers them equal ---> true
	// catches value, reference an meta.__eq
	if (lua_compare( L, -2, -1, LUA_OPEQ )) return 1;
	// metamethod prevails
	if (luaL_getmetafield( L, -2, "__eq" )) return lua_compare( L, -2, -1, LUA_OPEQ );
	if (LUA_TTABLE != lua_type( L, -2 ))    return 0;
	if (lua_rawlen( L, -1 ) != lua_rawlen( L, -2)) return 0;
	lua_pushnil( L );           //S: tableA tableB  nil
	while (lua_next( L, -3 ))   //S: tableA tableB  keyA  valueA
	{
		lua_pushvalue( L, -2 );  //S: tableA tableB  keyA  valueA  keyA
		lua_gettable( L, -4 );   //S: tableA tableB  keyA  valueA  valueB
		if (! is_really_equal( L ) )
		{
			lua_pop( L, 3 );      //S: tableA tableB
			return 0;
		}
		// pop valueA and valueB
		lua_pop( L, 2 );         //S: tableA tableB  keyA
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Compares items for equality
 * Based on the following tests (in order, fails for first non equal)
 *     - same type
 *     - same reference (for tables, functions and userdata)
 *     - same metatable (for tables and userdata)
 *     - runs __eq in metatable if present
 *     - same content   (table deep inspection)
 * \param   L       The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst_equal( lua_State *L )
{
	if (lua_gettop( L )<2 || lua_gettop( L )>3)
	{
		return t_push_error( L, T_TST_TYPE"._equals expects two or three arguments" );
	}
	if (3==lua_gettop( L ))
	{
		lua_insert( L, 1 );
	}
	// Deep table comparison
	if (is_really_equal( L ))
	{
		lua_pushboolean( L, 1 );
		lua_insert( L, 3 );
		return lua_gettop( L ) -2;
	}
	// this is the error case
	if (2==lua_gettop( L ))
	{
		lua_pushboolean( L, 0 );
		return 1;
	}
	else
	{
		lua_newtable( L );
		lua_pushvalue( L, 1 );
		lua_setfield( L, -2, "message" );
		fmt_stack_item( L, 2 );
		lua_setfield( L, -2, "expected" );
		fmt_stack_item( L, 3 );
		lua_setfield( L, -2, "got" );
		lua_pushliteral( L, "value expected not equal to value got" );
		lua_setfield( L, -2, "assert" );
		return lua_error( L );
	}
}


/**--------------------------------------------------------------------------
 * Compares items for non equality
 * Based on the following tests (in order, fails for first non equal)
 *          - same type
 *          - same reference (for tables, functions and userdata)
 *          - same metatable (for tables and userdata)
 *          - runs __eq in metatable if present
 *          - same content   (table deep inspection)
 * \param   L    The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst_equal_not( lua_State *L )
{
	if (lua_gettop( L )<2 || lua_gettop( L )>3)
	{
		return t_push_error( L, T_TST_TYPE"._equals expects two or three arguments" );
	}
	if (3==lua_gettop( L ))
	{
		lua_insert( L, 1 );
	}
	// Deep table comparison
	if (! is_really_equal( L ))
	{
		lua_pushboolean( L, 1 );
		lua_insert( L, 3 );
		return lua_gettop( L ) -2;
	}
	// this is the error case
	if (2==lua_gettop( L ))
	{
		lua_pushboolean( L, 0 );
		return 1;
	}
	else
	{
		lua_newtable( L );
		lua_pushvalue( L, 1 );
		lua_setfield( L, -2, "message" );
		fmt_stack_item( L, 2 );
		lua_setfield( L, -2, "expected" );
		fmt_stack_item( L, 3 );
		lua_setfield( L, -2, "got" );
		lua_pushliteral( L, "value expected is equal to value got" );
		lua_setfield( L, -2, "assert" );
		return lua_error( L );
	}
}


/**--------------------------------------------------------------------------
 * Compares lua values (lesser than)
 * \param   L    The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst_lt( lua_State *L )
{
	if (lua_gettop( L )<2 || lua_gettop( L )>3)
	{
		return t_push_error( L, T_TST_TYPE"._lt expects two or three arguments" );
	}
	// compare types, references, metatable.__eq and values
	if (lua_compare( L, 1, 2, LUA_OPLT ))
	{
		lua_pushboolean( L, 1 );
		lua_insert( L, 3 );
		return lua_gettop( L ) -2;
	}
	else
	{
		if (2==lua_gettop( L ))
		{
			lua_pushboolean( L, 0 );
			return 1;
		}
		else
		{
			lua_newtable( L );
			lua_pushvalue( L, 3 );
			lua_setfield( L, -2, "message" );
			fmt_stack_item( L, 1 );
			lua_setfield( L, -2, "expected" );
			fmt_stack_item( L, 2 );
			lua_setfield( L, -2, "got" );
			lua_pushliteral( L, "value expected not lesser than value got" );
			lua_setfield( L, -2, "assert" );
			return lua_error( L );
		}
	}
}


/**--------------------------------------------------------------------------
 * Add diagnostic output information for one test into a TAP line
 * luaL_prepbuffer can create userdata on the stack - this function
 * accounts for that by calling luaL_prepbuffer before everything else
 * Expects on stack:
 *        1. test case
 *        2. boolean(false) for failed test
 *        3. (userdata) possible by-product of luaL_Buffer
 * \param   L the Lua state.
 * \param  *lB    an already initialized Lua Buffer.
 * \param   p     the position of the test on the stack
 * \param   m     the name of the diagnostic field
 * --------------------------------------------------------------------------*/
static void
add_tap_diagnostics( lua_State *L, luaL_Buffer *lB, const char *m )
{
	char *a = luaL_prepbuffer( lB );
	lua_getfield( L, 1, m );
	if (! lua_isnoneornil( L, -1 ))
	{
		sprintf( a, "\n%-10s : %s", m, lua_tostring( L, -1 ) );
		luaL_gsub( L, a, "\n","\n    " );
		luaL_addsize( lB, 0 );
		luaL_addvalue( lB );
	}
	else
	{
		luaL_addsize( lB, 0 );
	}
	lua_pop( L, 1 );
}


/**--------------------------------------------------------------------------
 * Formats a test case into a TAP line.
 * \param   L the Lua state.
 * \lparam  t_tst_case  table with the on test_* function.
 * \lparam  t_tst       test suite table.
 * --------------------------------------------------------------------------*/
static int
t_tst_case__tostring( lua_State *L )
{
	luaL_Buffer lB;
	luaL_buffinit( L, &lB );
	lua_getfield( L, 1, "pass" );   // Stack: 2
	if( lua_isnil( L, -1) )
		luaL_addstring( &lB, "not run" );
	else
		luaL_addstring( &lB,(lua_toboolean( L, -1 )) ? "ok ":"not ok " );
	lua_getfield( L, 1, "ord" );   // Stack: 3
	luaL_addvalue( &lB );
	luaL_addstring( &lB, " - " );
	// print desc or name
	lua_getfield( L, 1, "desc" );    // Stack: 3
	if( lua_isnoneornil( L, -1 ))
	{
		lua_pop( L, 1 );              // pop nil desc
		lua_getfield( L, 1, "name" ); // Stack: 4
		luaL_addvalue( &lB );              // add name and pop it
	}
	else
	{
		luaL_addvalue( &lB );              // add desc and pop it
	}
	// Add skip info
	lua_getfield( L, 1, "skip" );    // Stack: 3
	if (! lua_isnoneornil( L, -1 ))
	{
		luaL_addstring( &lB, " #SKIP: " );
		luaL_addvalue( &lB );
	}
	else
	{    // Add todo information?
		lua_pop( L, 1 );    // pop skip
		lua_getfield( L, 1, "todo" );    // Stack: 3
		if (! lua_isnoneornil( L, -1 ))
		{
			luaL_addstring( &lB, " #TODO: " );
			luaL_addvalue( &lB );
		}
		else
		{
			lua_pop( L, 1 );   // pop todo nil
		}
	}
	luaL_addchar( &lB, '\n' );
	// Add diagnostics
	if (! lua_toboolean( L, 2 ))
	{
		luaL_addstring( &lB, "    ---" );
		add_tap_diagnostics( L, &lB, "name" );
		add_tap_diagnostics( L, &lB, "message" );
		add_tap_diagnostics( L, &lB, "assert" );
		add_tap_diagnostics( L, &lB, "expected" );
		add_tap_diagnostics( L, &lB, "got" );
		add_tap_diagnostics( L, &lB, "location" );
		add_tap_diagnostics( L, &lB, "traceback" );
		add_tap_diagnostics( L, &lB, "src" );
		luaL_addstring( &lB, "\n    ...\n" );
	}
	lua_pop( L, lua_gettop( L )-1 );
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Generates a TAP report.
 * \param   L    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst__tostring( lua_State *L )
{
	luaL_Buffer lB;
	int         i=1, t_len;
	t_tst_check_ud ( L, 1 );
	t_len           = luaL_len( L, 1 );
	luaL_buffinit( L, &lB );
	luaL_addstring( &lB, "1.." );
	lua_pushinteger( L, t_len );
	luaL_addvalue( &lB );
	luaL_addchar( &lB, '\n' );
	for (; i < t_len+1; i++)
	{
		lua_rawgeti( L, 1, i );
		luaL_getmetafield( L, -1, "__tostring" );
		lua_pushvalue( L, 2 );  // push table with test function (has __tostring method)
		lua_call( L, 1, 1 );
		luaL_addvalue( &lB );   // adds return value to buffer and pops it
		lua_pop( L, 1 );        // pop the test case table
	}
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * __call metatable for a single test function.
 * Stack:
 *          1 Test function table
 *          2 Test Suite table
 *          3 order execution number
 * \param   L    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int
t_tst_case__call( lua_State *L )
{
	int n;           ///< execution order number
	n = luaL_optinteger( L, 3, 0 );
	lua_pushcfunction( L, traceback ); // Stack: 4
	lua_getfield( L, 2, "_suitename" );// Stack: 5
	lua_getfield( L, 1, "name" );      // Stack: 6
	lua_getfield( L, 1, "line" );      // Stack: 7
	printf( "%2d - %s.%s:%lld  ... ",
	   n                   , lua_tostring( L, 5 ),
	   lua_tostring( L, 6 ), luaL_checkinteger( L, 7 ) );
	lua_pushinteger( L, n );
	lua_setfield( L, 1, "ord" );
	fflush( stdout );
	lua_pop( L, 3 );                   // pop _suitname,name,line
	lua_getfield( L, 1, "skip" );      // Stack: 5
	if (! lua_isnoneornil( L, -1 ))
	{
		printf( "# SKIP:%s\n", lua_tostring( L, -1 ) );
		return 0;
	}
	else
	{
		lua_pop( L, 1 );                // pop skip
		lua_getfield( L, 1, "f" );      // Stack: 5
	}

	lua_pushvalue( L, 2 );             // push suite as argument for t:test()
	if (lua_pcall( L, 1, 0, 4 ))
	{
		// Stack: 5  Error table
		printf( "fail\n" );
		lua_pushnil( L );
		while (lua_next( L, -2 ))   // copy error elements to test table 
		{
			lua_setfield( L, 1, lua_tostring( L, -2 ) );
		}
		lua_pop( L, 2 );        // pop the error table and traceback
		lua_pushboolean( L, 0 );
		lua_setfield( L, 1, "pass" );
		lua_pushboolean( L, 0 );
		return 1;
	}
	else
	{
		printf( "ok\n" );
		lua_pop( L, 1 );        // pop the traceback function
		lua_pushboolean( L, 1 );
		lua_setfield( L, 1, "pass" );
		lua_pushboolean( L, 1 );
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tst_fm [] = {
	  { "__call",              t_tst__Call }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tst_m [] = {
	  { "run",                 t_tst__call }
	, { "_eq",                 t_tst_equal }
	, { "_eq_not",             t_tst_equal_not }
	, { "_lt",                 t_tst_lt }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tst_cf [] = {
	  { "_eq",                 t_tst_equal }
	, { "_eq_not",             t_tst_equal_not }
	, { "_lt",                 t_tst_lt }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Test library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_tst( lua_State *L )
{
	// internal metatable that allows the it to be called
	luaL_newmetatable( L, T_TST_TYPE"."T_TST_CAS_NAME );   // stack: functions meta
	lua_pushcfunction( L, t_tst_case__call );
	lua_setfield( L, -2, "__call" );
	lua_pushcfunction( L, t_tst_case__tostring );
	lua_setfield( L, -2, "__tostring" );
	lua_pop( L, 1);        // remove metatable from stack

	// just make metatable known to be able to register and check type
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_TST_TYPE );   // stack: functions meta
	luaL_newlib( L, t_tst_m );
	lua_setfield( L, -2, "__index" );
	lua_pushcfunction( L, t_tst__newindex );
	lua_setfield( L, -2, "__newindex" );
	lua_pushcfunction( L, t_tst__call );
	lua_setfield( L, -2, "__call" );
	lua_pushcfunction( L, t_tst__tostring );
	lua_setfield( L, -2, "__tostring" );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Test.new() ...
	luaL_newlib( L, t_tst_cf );
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib( L, t_tst_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

