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

#include "t.h"
#include "t_tst.h"
#include "t_tim.h"

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
	t_getProxyTableIndex( L );           //S: … ste {}
	lua_newtable( L );                   //S: … ste {} tbl
	lua_rawset( L, -3 );                 //S: … ste
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
 * Check a value on the stack for being a T.Test. Replace T.Test with proxy table.
 * \param   L        Lua state.
 * \param   int      position on the stack.
 * \param   int      hardCheck; error out if not a T.Test.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
int
t_tst_check( lua_State *L, int pos, int check )
{
	int isTest = t_checkTableType( L, pos, check, T_TST_TYPE );
	if (isTest)
		t_getProxyTable( L, pos );
	return isTest;
}


/**--------------------------------------------------------------------------
 * Finishes the __call of the table.
 * \param   L        Lua state.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
static int
t_tst__callFinish( lua_State *L )
{
	t_tst_check( L, 1, 1 );
	size_t          i;
	size_t			 pass = 0,
						 skip = 0,  ///< marked as ran but haven't finished
						 todo = 0;  ///< expected to fail

	for (i=0; i<lua_rawlen( L, 1 ); i++)
	{
		lua_rawgeti( L, 1, i );          //S: ste tbl cse
		pass += (t_tst_cse_hasField( L, "pass", 0 )) ? 1 : 0;
		skip += (t_tst_cse_hasField( L, "skip", 0 )) ? 1 : 0;
		todo += (t_tst_cse_hasField( L, "todo", 0 )) ? 1 : 0;
		lua_pop( L, 1 );                 //S: ste tbl
	}
	printf( "---------------------------------------------------------\n"
	        "Handled %ld tests in %.3f seconds\n\n"
	        "Executed         : %ld\n"
	        "Skipped          : %ld\n"
	        "Expected to fail : %ld\n"
	        "Failed           : %ld\n"
	        "status           : %s\n"
	   //, i, t_tim_getms( &tm )/1000.0
	   , i, 3.0000
	   , i - skip
	   , skip
	   , todo
	   , i-pass
	   , (i==pass + todo) ? "OK" : "FAIL" );

	lua_pushboolean( L, (i==pass + todo) ? 1 : 0 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Execute the next test in line.
 * \param   L        Lua state.
 * \upval   int      next test to pop from the list.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
int
t_tst_exec( lua_State *L )
{
	lua_pushvalue( L, lua_upvalueindex( 1 ) );
	int idx = lua_tointeger( L, lua_upvalueindex( 2 ) ) + 1;
	lua_pushvalue( L, 1 );          //S: ste ste
	t_tst_check( L, 2, 1 );         //S: ste tbl
	printf( "EXECUTE INDEX: %d\n", idx );
	if (idx>(int) lua_rawlen( L, -1 ))
	{
		lua_pushcfunction( L, t_tst__callFinish ); //S: ste tbl fnc
		lua_insert( L, -3 );
		lua_pop( L, 1 );
		lua_call( L, 1, 0 );
	}
	else
	{
		lua_pushinteger( L, idx );
		lua_pushcclosure( L, lt_tst_cse__call, 1 ); //S: ste tbl fnc
		lua_rawgeti( L, 2, idx );       //s: ste tbl fnc cse
		lua_remove( L, 2 );             //S: ste fnc cse
		lua_pushvalue( L, 1 );          //s: ste fnc cse ste
		lua_call( L, 2, 1 );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Call the first test on the table.
 * \param   L        Lua state.
 * \upval   int      next test to pop from the list.
 * \lparam  table    T.Test Lua table instance.
 * --------------------------------------------------------------------------*/
static int
lt_tst__call( lua_State *L )
{
	lua_pushvalue( L, 1 );
	t_tst_check( L, 2, 1 );                //S: ste tbl
	if (lua_rawlen( L, -1 ) < 1)
		return 0;
	lua_pop( L, 1 );
	lua_pushinteger( L, 0 );               //S: ste idx
	lua_pushcclosure( L, t_tst_exec, 2 );  //S: exc

	lua_call( L, 0, 0 );
	return 0;
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
	t_tst_check( L, 1, 1 );
	name = luaL_checkstring( L, 2 );      //S: ste nme fnc
	luaL_argcheck( L, LUA_TNUMBER != lua_type( L, 1 ), 1, "Can't overwrite numeric indexes" );

	// insert a testcase
	if (0==strncasecmp( name, "test", 4 ))
	{
		// assigning a new test
		if (lua_isfunction( L, 3 ))
		{
			lua_pushcfunction( L, t_tst_cse_create );
			lua_pushvalue( L, 2 );
			lua_pushvalue( L, 3 );          //S: ste nme fnc create name fnc
			lua_remove( L, -4 );            //S: ste nme create name fnc
			lua_call( L, 2, 1 );            //S: ste nme cse
			lua_pushvalue( L, -1 );         //S: ste nme cse cse
			lua_rawseti( L, 1,
			lua_rawlen( L, 1 ) + 1 );       //S: ste nme cse
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
	int         i, pass, todo, t_len;
	size_t      concat = 0;

	t_tst_check( L, 1, 1 );
	t_len  = lua_rawlen( L, 1 );
	luaL_buffinit( L, &lB );
	lua_pushfstring( L, "1..%d\n", t_len );  //S: tbl 1..
	concat++;
	for (i=0; i<t_len; i++)
	{
		lua_rawgeti( L, 1, i+1 );             //S: tbl 1.. cse
		todo = t_tst_cse_hasField( L, "todo", 0 );
		lua_insert( L, 2 );   // make sure the test case is at stackpos 2
		// passed: ok/not ok/not run
		lua_getfield( L, 2, "pass" );         //S: tbl 1.. cse pss
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );             // pop pass nil
			lua_pushstring( L, "not run" );
			concat++;
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
		concat=0;
		luaL_addvalue( &lB );
		luaL_addchar( &lB, '\n' );
		lua_remove( L, 2 );          // remove cse S: ste
	}
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns len of T.Test.
 * \param   L    Lua state.
 * \lparam  ud   T.OrderedHashTable userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst__len( lua_State *L )
{
	t_tst_check( L, 1, 1 );
	lua_pushinteger( L, lua_rawlen( L, 1 ) );
	lua_remove( L, 1 );
	return 1;
}


/** -------------------------------------------------------------------------
 * Search all upvalues on all stack levels for a Test.Case.
 * \param    L        Lua state.
 * \lreturn  table    T.Test.Case Lua table instance.
 * \return   int/bool found or not.
 *-------------------------------------------------------------------------*/
static int
t_tst_findCaseOnStack( lua_State *L )
{
	lua_Debug   ar;
	int         i   = 0;
	int         nu  = 0;
	const char *nme;

	while ( lua_getstack( L, i++, &ar ))
	{
		lua_getinfo( L, "fu", &ar );       // get function onto stack
		//printf("\n\n\nLevel %d[%d] -> ", i, ar.nups);   t_stackDump(L);
		nu  = 0;
		while (nu++ < ar.nups)
		{
			nme = lua_getupvalue( L, -1, nu );
			//printf("   UPV %d - %s  \t-> ", nu, nme); t_stackDump(L);
			if (t_tst_cse_check( L, -1, 0 ))
			{
				//printf("  FOUND THE TEST CASE  \n");
				lua_remove( L, -2 );     // remove current function
				return 1;
			}
			lua_pop( L, 1 );            // remove upvalue
		}
		lua_pop( L, 1 );               // remove function that lua_getinfo put on stack
	}
	return 0;
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
 * Work on negative inices ONLY for recursive use
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


/** -------------------------------------------------------------------------
 * Mark a Test.Case as Todo.
 * \param    L     Lua state.
 * \lreturn  table imported library.
 *-------------------------------------------------------------------------*/
static int
lt_tst_Todo( lua_State *L )
{
	luaL_checkstring( L, 1 );
	if (t_tst_findCaseOnStack( L ))
	{
		lua_insert( L, 1 );
		lua_setfield( L, 1, "todo" );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Set the description for a Test.Case
 * \param    L     Lua state.
 * \lreturn  table imported library.
 *-------------------------------------------------------------------------*/
static int
lt_tst_Describe( lua_State *L )
{
	luaL_checkstring( L, 1 );
	if (t_tst_findCaseOnStack( L ))
	{
		lua_insert( L, 1 );
		lua_setfield( L, 1, "description" );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Actively skip a Test.Case.
 * Function does get executed until Test.skip('Reason') get's called.  Does
 * allow conditional skipping.  A skip is to throw a controlled luaL_error which
 * gets caught by t_tst_cse_traceback.
 * \param    L     Lua state.
 * \lreturn  table imported library.
 *-------------------------------------------------------------------------*/
static int
lt_tst_Skip( lua_State *L )
{
	luaL_checkstring( L, 1 );
	lua_pushstring( L, T_TST_CSE_SKIPINDICATOR );
	lua_insert( L, -2 );
	lua_concat( L, 2 );
	luaL_error( L, lua_tostring( L, -1 ) );
	return 0;
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
	, { "todo"               , lt_tst_Todo }
	, { "skip"               , lt_tst_Skip }
	, { "describe"           , lt_tst_Describe }
	, { NULL,  NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tst_m [] = {
	  { "__call"             , lt_tst__call }
	, { "__tostring"         , lt_tst__tostring }
	, { "__newindex"         , lt_tst__newindex }
	, { "__index"            , t_getFromProxyTable }
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
LUA_API int
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

