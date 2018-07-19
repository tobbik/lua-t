/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tst.c
 * \brief     lua-t unit testing framework (T.Test)
 *            Test suite implemented as Lua Table
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <string.h>            // strlen
#include <strings.h>           // strncasecmp
#include <stdlib.h>            // malloc

#include "t_tst_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

static int lt_tst__newindex( lua_State *L );

/**--------------------------------------------------------------------------
 * Construct a T.Test and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table Test
 * \lparam  table  optional table of unit test cases (functions)
 * \lreturn table  T.Test Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst__Call( lua_State *L )
{
	lua_remove( L, 1 );                  //S: …         remove T.Test class
	lua_newtable( L );                   //S: … tbl
	luaL_getmetatable( L, T_TST_TYPE );  //S: … tbl met
	lua_setmetatable( L, -2 );           //S: … ste
	//t_getProxyTableIndex( L );           //S: … ste {}
	//lua_newtable( L );                   //S: … ste {} tbl
	//lua_rawset( L, -3 );                 //S: … ste
	if( lua_istable( L, 1 ) )
	{
		lua_pushnil( L );
		while (lua_next( L, 1 ))          //S: tbl ste nme fnc
		{
			lua_pushcfunction( L, lt_tst__newindex );
			lua_insert( L, -4 );           //S: tbl _ni ste nme fnc
			lua_pushvalue( L, -2 );        //S: tbl _ni ste nme fnc nme
			lua_insert( L, 2 );            //S: tbl nme _ni ste nme fnc
			lua_pushvalue( L, -3 );        //S: tbl nme _ni ste nme fnc ste
			lua_insert( L, 2 );            //S: tbl ste nme _ni ste nme fnc
			lua_call( L, 3, 0 );
		}
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a T.Test Suite.
 * \param   L        Lua state.
 * \param   int      position on the stack.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
void
t_tst_check( lua_State *L, int pos )
{
	luaL_checktype( L, pos, LUA_TTABLE );
	if (lua_getmetatable( L, pos ))        // does it have a metatable?
	{
		luaL_getmetatable( L, T_TST_TYPE ); // get correct metatable
		if (! lua_rawequal( L, -1, -2 ))     // not the same?
			t_typeerror( L , pos, T_TST_TYPE );
		lua_pop( L, 2 );
	}
	else
		t_typeerror( L , pos, T_TST_TYPE );
}


/**--------------------------------------------------------------------------
 * Run the beforeAll/afterAll envelope for T.Test suite.
 * \param   L       Lua state.
 * \lparam  table   T.Test Lua table instance.
 * \lparam  cfunc   Function to be executed after envelope() call.
 * --------------------------------------------------------------------------*/
static int
t_tst_callEnvelope( lua_State *L, const char *envelope )
{
	lua_getfield( L, 1, envelope );  //S: ste dne env
	if (! lua_isnil( L, -1 ))
	{
		lua_insert( L, -2 );          //S: ste env dne
		lua_pushvalue( L, 1 );        //S: ste env dne ste
		lua_insert( L, -2 );          //S: ste env ste dne
		if (lua_pcall( L, 2, 0, 0 ))
			return luaL_error( L, "T.Test Suite %s() failed: %s",
				envelope, lua_tostring( L, -1 ) );
	}
	else
	{
		lua_pop( L, 1 );              // pop nil
		lua_pushvalue( L, 1 );        //S: ste dne ste
		lua_call( L, 1, 0 );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Evaluates if a test suite has passed or not.
 * \param   L       Lua state.
 * \lparam  table   T.Test Lua table instance.
 * \lreturn pass    Boolean, has the suite passed.
 * \lreturn pass    Integer, counting all passed  test cases in suite.
 * \lreturn skip    Integer, counting all skipped test cases in suite.
 * \lreturn todo    Integer, counting all todo    test cases in suite.
 * \lreturn since   Long adding up recorded execution times.
 * --------------------------------------------------------------------------*/
static void
t_tst_getMetrics( lua_State *L )
{
	lua_Integer idx;
	lua_Integer pass  = 0,
	            skip  = 0,  ///< marked as ran but haven't finished
	            todo  = 0;  ///< expected to fail

	for (idx=0; idx<luaL_len( L, 1 ); idx++)
	{
		lua_geti( L, 1, idx+1 );                 //S: ste cse
		pass  += (t_tst_cse_hasField( L, -1, "pass", 0 )) ? 1 : 0;
		skip  += (t_tst_cse_hasField( L, -1, "skip", 0 )) ? 1 : 0;
		todo  += (t_tst_cse_hasField( L, -1, "todo", 0 )) ? 1 : 0;
		lua_pop( L, 1 );                         //S: ste
	}
	lua_pushboolean( L, (idx == pass + todo) ? 1 : 0 );
	lua_pushinteger( L, pass );
	lua_pushinteger( L, skip );
	lua_pushinteger( L, todo );
}


/**--------------------------------------------------------------------------
 * Finishes the __call of T.Test suite.  Assembles report.
 * \param   L        Lua state.
 * \upvalue table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
static int
t_tst_callFinalize( lua_State *L )
{
	size_t      len;

	lua_pushvalue( L, lua_upvalueindex( 1 ) );     //S: ste
	t_tst_check( L, 1 );
	len = luaL_len( L, 1 );
	t_tst_getMetrics( L );   //S: ste bool pass skip todo since
	printf( "---------------------------------------------------------\n"
	        "Handled %lu tests in ????? seconds\n\n"
	        "Executed         : %lld\n"
	        "Skipped          : %lld\n"
	        "Expected to fail : %lld\n"
	        "Failed           : %lld\n"
	        "status           : %s\n"
	   , len
	   , len - lua_tointeger( L, -2 )
	   , lua_tointeger( L, -2 )
	   , lua_tointeger( L, -1 )
	   , len - lua_tointeger( L, -3 )
	   , (lua_toboolean( L, -4 )) ? "OK" : "FAIL" );

	return 0;
}


/**--------------------------------------------------------------------------
 * Finish the execution of a T.Test.Case.
 * \param    L        Lua state.
 * \svalue   table    T.Test.Case Lua table instance.
 * \svalue   table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
int
t_tst_done( lua_State *L )
{
	lua_Integer ran = 0;                       //S: cse ste
	lua_Integer idx;

	t_tst_cse_getDescription( L, 1 );
	printf( "Executed Test:  %s\n", lua_tostring( L, -1 ) );
	lua_pop( L, 1 );                           // pop Description
	t_tst_check( L, 2 );
	for (idx=0; idx < luaL_len( L, 2 ); idx++)
	{
		lua_geti( L, 2, idx+1 );                //S: cse ste cseI
		lua_getfield( L, -1, "pass" );
		ran += (lua_isnil( L, -1 )) ? 0 : 1;
		lua_pop( L, 2 );                        // pop nil/pass, cseI
	}
	if (luaL_len( L, 2 ) == ran)
	{
		lua_remove( L, 1 );
		lua_pushvalue( L, 1 );
		lua_pushcclosure( L, t_tst_callFinalize, 1 );
		t_tst_callEnvelope( L, "afterAll" );
		return 1;
	}
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Loops over all cases in Test.Suite and executes them.
 * \param    L      Lua state.
 * \upvalue  table  T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
static int
t_tst_callLoopCases( lua_State *L )
{
	lua_Integer idx;
	lua_pushvalue( L, lua_upvalueindex( 1 ) );     //S: ste
	t_tst_check( L, 1 );         //S: ste
	for (idx=0; idx<luaL_len( L, 1 ); idx++)
	{
		lua_pushcfunction( L, lt_tst_cse__call ); //S: ste fnc
		lua_geti( L, 1, idx+1 );     //S: ste fnc cse
		lua_pushvalue( L, 1 );       //S: ste fnc cse ste
		lua_call( L, 2, 0 );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Call the first test on the table.
 * \param   L        Lua state.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
static int
lt_tst__call( lua_State *L )
{
	lua_Integer idx;
	t_tst_check( L, 1 );         //S: ste
	for (idx=0; idx < luaL_len( L, 1 ); idx++)
	{
		lua_geti( L, 1, idx+1 );
		t_tst_cse_prune( L );
		lua_pop( L, 1 );
	}
	lua_pushvalue( L, 1 );
	lua_pushcclosure( L, t_tst_callLoopCases, 1 );
	t_tst_callEnvelope( L, "beforeAll" );

	t_tst_getMetrics( L );   //S: ste bool pass skip todo since
	lua_pop( L, 4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Gets called for assignement of variables to test instance.
 * This covers three jobs:
 *  - only allow strings as keys
 *  - make sure no one adds numeric elements (internal test order)
 *  - append ^test* named methods to internal test order
 * \param   L      Lua state.
 * \lparam  table  T.Test Lua table instance.
 * \lparam  key    string
 * \lparam  value  LuaType
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst__newindex( lua_State *L )
{
	const char *name;
	t_tst_check( L, 1 );
	name = luaL_checkstring( L, 2 );      //S: ste nme fnc
	luaL_argcheck( L, LUA_TNUMBER != lua_type( L, 1 ), 1, "Can't overwrite numeric indexes" );

	// insert a testcase
	if (0==strncasecmp( name, "test_", 5 ))
	{
		// assigning a new test
		if (lua_isfunction( L, 3 ))
		{
			lua_pushcfunction( L, t_tst_cse_create );
			lua_pushvalue( L, 2 );
			lua_pushvalue( L, 3 );          //S: tbl nme fnc create name fnc
			lua_remove( L, -4 );            //S: tbl nme create name fnc
			lua_call( L, 2, 1 );            //S: tbl nme cse
			lua_pushvalue( L, -1 );         //S: tbl nme cse cse
			lua_rawseti( L, 1,
			lua_rawlen( L, 1 ) + 1 );       //S: tbl nme cse
		}
		else
			return luaL_error( L, "value for test* named element must be a function" );
	}
	// insert as test.name
	lua_rawset( L, 1 );

	return 0;
}


/**--------------------------------------------------------------------------
 * Generates a TAP report.
 * \param   L      Lua state.
 * \lparam  table  T.Test Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst__tostring( lua_State *L )
{
	luaL_Buffer lB;
	int         i, pass, todo;
	lua_Integer t_len;
	size_t      concat = 0;

	t_tst_check( L, 1 );
	t_len  = luaL_len( L, 1 );
	luaL_buffinit( L, &lB );
	lua_pushfstring( L, "1..%d\n", t_len );  //S: ste 1..
	concat++;
	for (i=0; i<t_len; i++)
	{
		lua_geti( L, 1, i+1 );                //S: ste 1.. cse
		todo = t_tst_cse_hasField( L, -1, "todo", 0 );
		lua_insert( L, 2 );   // make sure the test case is at stackpos 2
		// passed: ok/not ok/not run
		lua_getfield( L, 2, "pass" );         //S: ste 1.. cse pss
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );             // pop pass nil
			lua_pushstring( L, "not run - " );
			t_tst_cse_getDescription( L, 2 );
			concat+=2;
		}
		else
		{
			pass = lua_toboolean( L, -1 );
			lua_pop( L, 1 );             // pop pass boolean
			lua_pushfstring( L, "%s %d - ", (pass) ? "ok":"not ok", i+1 );
			t_tst_cse_getDescription( L, 2 );
			concat+=2;
			if (! pass && ! todo)
			{
				t_tst_cse_addTapDiagnostic( L, 2 );
				concat++;
			}
		}
		lua_concat( L, concat );
		concat = 0;
		luaL_addvalue( &lB );
		luaL_addchar( &lB, '\n' );
		lua_remove( L, 2 );          // remove cse S: ste
	}
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns len of T.Test.
 * \param   L     Lua state.
 * \lparam  table T.Test Lua table instance.
 * \lreturn int   Length of T.Test Lua table instance.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst__len( lua_State *L )
{
	t_tst_check( L, 1 );   //S: ste
	lua_pushinteger( L, lua_rawlen( L, 1 ) );
	return 1;
}


/** ---------------------------------------------------------------------------
 * Compares the last two values on the stack (deep table compare; recursive)
 * Work on negative inices ONLY for recursive use
 * \param   L        Lua state
 * \lparam  value    valueA to compare
 * \lparam  value    valueB to compare
 * \return  int/bool 1 or 0
 *--------------------------------------------------------------------------- */
int
t_tst_isReallyEqual( lua_State *L )
{
	// if lua considers them equal ---> true
	// catches value, reference an meta.__eq
	if (lua_compare( L, -2, -1, LUA_OPEQ )) return 1;
	// metamethod prevails
	if (luaL_getmetafield( L, -2, "__eq" )) return lua_compare( L, -2, -1, LUA_OPEQ );
	if (LUA_TTABLE != lua_type( L, -2 ))    return 0;
	if (lua_rawlen( L, -1 ) != lua_rawlen( L, -2)) return 0;
	lua_pushnil( L );           //S: tblA tblB  nil
	while (lua_next( L, -3 ))   //S: tblA tblB  keyA  valA
	{
		lua_pushvalue( L, -2 );  //S: tblA tblB  keyA  valA  keyA
		lua_gettable( L, -4 );   //S: tblA tblB  keyA  valA  valB
		if (! t_tst_isReallyEqual( L ))
		{
			lua_pop( L, 3 );      //S: tblA tblB
			return 0;
		}
		// pop valueA and valueB
		lua_pop( L, 2 );         //S: tblA tblB  keyA
	}
	return 1;
}


/** ---------------------------------------------------------------------------
 * Compares the last two values on the stack (deep table compare; recursive)
 * \param   L        Lua state
 * \lparam  value    valueA to compare
 * \lparam  value    valueB to compare
 * \return  int/bool 1 or 0
 * TODO: push an expressive error message onto the stack
 *--------------------------------------------------------------------------- */
static int
lt_tst_IsReallyEqual( lua_State *L )
{
	lua_pushboolean( L, t_tst_isReallyEqual( L ) );
	return 1;
}


/** ---------------------------------------------------------------------------
 * Test if a T.Test.Suite has run successfully.
 * Note that test case marked as TODO must fail else the suite will fail.
 * \param   L       Lua state
 * \lparam  table   T.Test Lua table instance.
 * \lreturn boolean True, if all test ran successfully, else False.
 * \return  int     # of values pushed onto the stack.
 *--------------------------------------------------------------------------- */
static int
lt_tst_HasPassed( lua_State *L )
{
	t_tst_check( L, 1 );
	t_tst_getMetrics( L );   //S: ste bool pass skip todo since
	lua_pop( L, 5 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tst_fm [] = {
	  { "__call",              lt_tst__Call }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tst_cf [] = {
	  { "equal"              , lt_tst_IsReallyEqual }
	, { "hasPassed"          , lt_tst_HasPassed }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tst_m [] = {
	  { "__call"             , lt_tst__call }
	, { "__tostring"         , lt_tst__tostring }
	, { "__newindex"         , lt_tst__newindex }
	, { "__len"              , lt_tst__len }
	, { NULL                 , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Test library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     Lua state.
 * \lreturn table T.Test library
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_tst( lua_State *L )
{
	luaL_newmetatable( L, T_TST_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_tst_m, 0 );
	lua_pop( L, 1 );

	// Push the class onto the stack
	// this is avalable as T.Test.<member>
	luaL_newlib( L, t_tst_cf );
	luaopen_t_tst_cse( L );
	lua_setfield( L, -2, T_TST_CSE_NAME );
	luaL_newlib( L, t_tst_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

