/*
 * \file    xt_test.c
 * \brief   xt unit testing framework
 * \detail  OOP wrapper for Test cases. Unit
*/
#include <string.h>            // strlen
#include <strings.h>           // strncasecmp
#include <memory.h>            // memset
#include <stdlib.h>            // malloc

#include "l_xt.h"
#include "xt_time.h"


// forward declaration eliminates need for header file
// l_xt_test.c
void xt_test_check_ud (lua_State *luaVM, int pos);
int  xt_test_new(lua_State *luaVM);


/**--------------------------------------------------------------------------
 * push a string formatting a stack element
 * \param   luaVM  The lua state.
 * \param   pos    integer.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static void fmt_stack_item (lua_State *luaVM, int pos)
{
	int t;
	if (!luaL_callmeta (luaVM, pos, "__tostring"))
	{
		t = lua_type(luaVM, pos);
		switch (t) {
			case LUA_TSTRING:    /* strings */
				lua_pushvalue (luaVM, pos);
				break;
			case LUA_TBOOLEAN:   /* booleans */
				if (lua_toboolean (luaVM, pos)) lua_pushliteral (luaVM, "true");
				else lua_pushliteral (luaVM, "false");
				break;
			case LUA_TNUMBER:    /* numbers */
				lua_pushfstring (luaVM, "%f", lua_tonumber(luaVM, pos));
				break;
			default:	            /* other values */
				lua_pushfstring (luaVM, "%s", lua_typename(luaVM, pos));
				break;
		}
	}
}


/**--------------------------------------------------------------------------
 * construct a Test and return it.
 * \param   luaVM  The lua state.
 * \lparam  CLASS  table Test
 * \lparam  string name of the unit test
 * \lparam  string description of the unit test
 * \lreturn luatable representing a test
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test___call(lua_State *luaVM)
{
	lua_remove(luaVM, 1);
	return xt_test_new(luaVM);
}


/**--------------------------------------------------------------------------
 * create an ceTest Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  string name of the unit test
 * \lparam  string description of the unit test
 * \lreturn luatable representing a test
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int xt_test_new(lua_State *luaVM)
{
	lua_newtable (luaVM);
	if (lua_gettop (luaVM) ==2 && lua_isstring (luaVM, 2))
		lua_pushvalue (luaVM, 2);
	else
		lua_pushliteral (luaVM, "anonymous");
	lua_setfield (luaVM, -2 , "_suitename");
	luaL_getmetatable (luaVM, "xt.Test");
	lua_setmetatable (luaVM, -2);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \lparam  the Test table on the stack
 * \lreturn leaves the test table on the stack
 * --------------------------------------------------------------------------*/
void xt_test_check_ud (lua_State *luaVM, int pos)
{
	luaL_checktype(luaVM, pos, LUA_TTABLE);
   if (lua_getmetatable(luaVM, pos))  /* does it have a metatable? */
	{
		luaL_getmetatable(luaVM, "xt.Test");   /* get correct metatable */
		if (!lua_rawequal(luaVM, -1, -2))      /* not the same? */
			xt_push_error (luaVM, "wrong argumnet, `xt.Test` expected");
		lua_pop (luaVM, 2);
	}
	else
		xt_push_error (luaVM, "worong argument, `xt.Test` expected");
}


/**--------------------------------------------------------------------------
 * \brief   creates a traceback from a function call
 * \param   luaVM    The lua state.
 * \lparam  either assert result table or generig string
 * \lreturn a xt_test result Failure description table
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int traceback (lua_State *luaVM) {
	if (LUA_TSTRING == lua_type (luaVM, 1))
	{
		lua_newtable (luaVM);
		lua_insert (luaVM, 1);
		lua_setfield (luaVM, 1, "message");
	}
	if (LUA_TTABLE == lua_type (luaVM, 1))
	{
		luaL_where (luaVM, 2);
		lua_setfield (luaVM, 1, "location");
		luaL_traceback (luaVM, luaVM, NULL, 1);
		lua_setfield (luaVM, 1, "traceback");
	}

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   executes the test suite.
 * \param   luaVM    The lua state.
 * \lparam  test instance table.
 * \lreturn boolean true if all passed, otherwise false
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test__call (lua_State *luaVM)
{
	size_t          all  = 1,
	                pass = 0,
	                skip = 0;
	struct timeval *tm;

	xt_test_check_ud  (luaVM, 1);
	lua_pushstring (luaVM, "_time");
	tm =  xt_time_create_ud (luaVM);          // Stack: 2
	lua_rawset (luaVM, 1);

	lua_getfield (luaVM, 1, "setUp");
	lua_pushvalue (luaVM, 1);
	if (lua_pcall (luaVM, 1, 0, 0))
		xt_push_error (luaVM, "Test setup failed %s", lua_tostring (luaVM, -1));
	for ( all=1 ;all < lua_rawlen (luaVM, 1)+1 ; all++ )
	{
		lua_rawgeti(luaVM, 1, all);
		lua_getfield (luaVM, 2, "skip");
		skip = (lua_isnoneornil (luaVM, -1)) ? skip : skip+1;
		lua_pop (luaVM, 1);
		luaL_getmetafield (luaVM, -1, "__call");
		lua_pushvalue (luaVM, -2);         // push table with test function (has __call method)
		lua_pushvalue (luaVM, 1);          // push test suite table (aka. 'self' in test functions
		lua_pushinteger (luaVM, all);
		lua_call (luaVM, 3, 1);
		pass = (lua_toboolean (luaVM, -1)) ? pass+1 : pass;
		lua_pop (luaVM, 2);     // pop the test case table and return result
	}
	lua_getfield (luaVM, 1, "tearDown");
	lua_pushvalue (luaVM, 1);
	if (lua_pcall (luaVM, 1, 0, 0))
		xt_push_error (luaVM, "Test tearDown failed %s", lua_tostring (luaVM, -1));

	xt_time_Since (tm);
	--all;
	printf ("---------------------------------------------------------\n"
	        "Executed %lu tests in %03f seconds\n\n"
	        "Skipped : %lu\n"
	        "Failed  : %lu\n"
	        "status  : %s\n",
	   all, xt_time_Get_ms(tm)/1000.0,
		skip,
		all-pass,
		(all==pass)? "OK":"FAIL");

	lua_pushboolean (luaVM, (all==pass) ? 1 : 0);

	return 1;
}


/**----------------------------------------------------------------------------
 * \brief   inspects source for special lines in the comments
 * \detail  use lua debug facilities to determine lines of code and read the
 *          code from the source files
 *          - extracts TODO, SKIP and DESC from the comments
 *
 */
static void xt_get_fn_source (lua_State *luaVM)
{
	lua_Debug  ar;
	FILE      *f;
	int        r  = 0; ///< current line count
	char      *fnd;
	char      *p;
	luaL_Buffer lB;
	luaL_buffinit (luaVM, &lB);
	lua_State *L1 = luaVM;
	lua_pushstring(luaVM, ">S");
	lua_pushvalue(luaVM, 3);
	lua_xmove(luaVM, L1, 1);
	lua_getinfo (L1, ">S", &ar);
	lua_pop (luaVM, 1); // pop the ">S"
	lua_pushinteger (luaVM, ar.linedefined);
	lua_setfield (luaVM, -2, "line");

	f = fopen(ar.short_src, "r");
	while (r < ar.lastlinedefined)
	{
		p = luaL_prepbuffer (&lB);
		sprintf (p, "\n    %4d: ", r+1);
		if (fgets(p+11, LUAL_BUFFERSIZE, f) == NULL) break;  // eof?
		//TODO: reasonable line end check
		if (++r < ar.linedefined) continue;
		luaL_addsize (&lB, strlen (p)-1);
		if (NULL != (fnd=strstr(p,"-- #TODO:"))) {
			lua_pushlstring (luaVM, fnd+9, strlen(fnd+9)-1);
			lua_setfield   (luaVM, -3, "todo");
		}
		if (NULL != (fnd=strstr(p,"-- #SKIP:"))) {
			lua_pushlstring (luaVM, fnd+9, strlen(fnd+9)-1);
			lua_setfield   (luaVM, -3, "skip");
		}
		if (NULL != (fnd=strstr(p,"-- #DESC:"))) {
			lua_pushlstring (luaVM, fnd+9, strlen(fnd+9)-1);
			lua_setfield   (luaVM, -3, "desc");
		}
	}
	luaL_pushresult (&lB);
	lua_setfield   (luaVM, -2, "src");

	fclose (f);
}


/**--------------------------------------------------------------------------
 * \brief   gets called for assignement of variables to test instance
 * \detail  has three jobs:
 *          - make sure no one adds numeric elements (internal test order)
 *          - no overwrite or create of _* functions
 *          - add ^test.* methods to internal test order
 * \param   luaVM    The lua state.
 * \lparam  table test instance
 * \lparam  key   string
 * \lparam  value LuaType
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test__newindex (lua_State *luaVM)
{
	const char *name;
	xt_test_check_ud (luaVM, 1);                     // on Stack [1]
	name = luaL_checkstring(luaVM, 2);               // on Stack [2]

	// throw out illegal assignments
	if (lua_isnumber(luaVM, 2))
		return xt_push_error(luaVM,
			"Can't overwrite or create numeric indices");
	if ('_' == *name)
		return xt_push_error(luaVM,
			"Can't overwrite or create internal test methods");

	// insert a testcase
	if (0==strncasecmp (name, "test", 4) )
	{
		// assigning a new test
		if (lua_isfunction(luaVM, 3))
		{
			// TODO: find a more elegant way without that much stack gymnastics
			lua_newtable (luaVM);                // Stack 4: empty table
			xt_get_fn_source(luaVM);
			luaL_getmetatable (luaVM, "xt.Test.Case");
			lua_setmetatable (luaVM, 4);

			lua_pushvalue (luaVM , 2);           // copy name from 2 to 5
			lua_setfield (luaVM, 4, "name");

			lua_insert (luaVM, 3);               // move new table to Stack 3
			lua_setfield (luaVM, 3, "f");

			lua_pushvalue (luaVM, 3);            // make copy of table for test [n]
			lua_rawseti (luaVM,   1, lua_rawlen(luaVM, 1) +1); // insert as test [n]
		}
		else
		{
			return xt_push_error(luaVM,
				"test* named elements must be methods");
		}
	}
	// insert as test.name
	lua_rawset (luaVM, 1);

	return 0;
}


/** ---------------------------------------------------------------------------
 * \brief   compares the last two values on the stack (table deep recursion)
 * \detail  work on negative inices ONLY for recursive use
 * \param   luaVM lua_State
 * \lparam  value 1
 * \lparam  value 2
 * \return  boolean in 1 or 0
 * TODO: push an expressive error message onto the stack
 *--------------------------------------------------------------------------- */
static int is_really_equal (lua_State *luaVM)
{
	// if lua considers them equal ---> true
	// catches value, reference an meta.__eq
	if (lua_compare (luaVM, -2, -1, LUA_OPEQ)) return 1;
	// metamethod prevails
	if (luaL_getmetafield (luaVM, -2, "__eq")) return lua_compare (luaVM, -2, -1, LUA_OPEQ);
	if (LUA_TTABLE != lua_type (luaVM, -2))    return 0;
	if (lua_rawlen (luaVM, -1) != lua_rawlen (luaVM, -2)) return 0;
	lua_pushnil (luaVM);           // Stack: tableA tableB  nil
	while (lua_next(luaVM, -3))    // Stack: tableA tableB  keyA  valueA
	{
		lua_pushvalue (luaVM, -2);  // Stack: tableA tableB  keyA  valueA  keyA
		lua_gettable( luaVM, -4);   // Stack: tableA tableB  keyA  valueA  valueB
		if (! is_really_equal (luaVM) )
		{
			lua_pop (luaVM, 3);      // Stack: tableA tableB
			return 0;
		}
		lua_pop(luaVM, 2);       // pop valueA and valueB
		// Stack tableA tableB  keyA
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   compares items for equality
 * \detail  based on the following tests (in order, fails for first non equal)
 *          - same type
 *          - same reference (for tables, functions and userdata)
 *          - same metatable (for tables and userdata)
 *          - runs __eq in metatable if present
 *          - same content   (table deep inspection)
 * \param   luaVM    The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test_equal (lua_State *luaVM)
{
	if (lua_gettop (luaVM)<2 || lua_gettop (luaVM)>3)
	{
		return xt_push_error (luaVM, "xt.Test._equals expects two or three arguments");
	}
	if (3==lua_gettop (luaVM))
	{
		lua_insert (luaVM, 1);
	}
	// Deep table comparison
	if (is_really_equal (luaVM))
	{
		lua_pushboolean (luaVM, 1);
		lua_insert (luaVM, 3);
		return lua_gettop (luaVM) -2;
	}
	// this is the error case
	if (2==lua_gettop (luaVM))
	{
		lua_pushboolean (luaVM, 0);
		return 1;
	}
	else
	{
		lua_newtable (luaVM);
		lua_pushvalue (luaVM, 1);
		lua_setfield (luaVM, -2, "message");
		fmt_stack_item (luaVM, 2);
		lua_setfield (luaVM, -2, "expected");
		fmt_stack_item (luaVM, 3);
		lua_setfield (luaVM, -2, "got");
		lua_pushliteral (luaVM, "value expected not equal to value got");
		lua_setfield (luaVM, -2, "assert");
		return lua_error (luaVM);
	}
}


/**--------------------------------------------------------------------------
 * \brief   compares items for non equality
 * \detail  based on the following tests (in order, fails for first non equal)
 *          - same type
 *          - same reference (for tables, functions and userdata)
 *          - same metatable (for tables and userdata)
 *          - runs __eq in metatable if present
 *          - same content   (table deep inspection)
 * \param   luaVM    The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test_equal_not (lua_State *luaVM)
{
	if (lua_gettop (luaVM)<2 || lua_gettop (luaVM)>3)
	{
		return xt_push_error (luaVM, "xt.Test._equals expects two or three arguments");
	}
	if (3==lua_gettop (luaVM))
	{
		lua_insert (luaVM, 1);
	}
	// Deep table comparison
	if (! is_really_equal (luaVM))
	{
		lua_pushboolean (luaVM, 1);
		lua_insert (luaVM, 3);
		return lua_gettop (luaVM) -2;
	}
	// this is the error case
	if (2==lua_gettop (luaVM))
	{
		lua_pushboolean (luaVM, 0);
		return 1;
	}
	else
	{
		lua_newtable (luaVM);
		lua_pushvalue (luaVM, 1);
		lua_setfield (luaVM, -2, "message");
		fmt_stack_item (luaVM, 2);
		lua_setfield (luaVM, -2, "expected");
		fmt_stack_item (luaVM, 3);
		lua_setfield (luaVM, -2, "got");
		lua_pushliteral (luaVM, "value expected is equal to value got");
		lua_setfield (luaVM, -2, "assert");
		return lua_error (luaVM);
	}
}


/**--------------------------------------------------------------------------
 * \brief   compares lua values (lesser than)
 * \param   luaVM    The lua state.
 * \lparam  element A
 * \lparam  element B
 * \lreturn boolean true-equal, false-not equal
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test_lt (lua_State *luaVM)
{
	if (lua_gettop (luaVM)<2 || lua_gettop (luaVM)>3)
	{
		return xt_push_error (luaVM, "xt.Test._lt expects two or three arguments");
	}
	// compare types, references, metatable.__eq and values
	if (lua_compare (luaVM, 1, 2, LUA_OPLT))
	{
		lua_pushboolean (luaVM, 1);
		lua_insert (luaVM, 3);
		return lua_gettop (luaVM) -2;
	}
	else
	{
		if (2==lua_gettop (luaVM))
		{
			lua_pushboolean (luaVM, 0);
			return 1;
		}
		else
		{
			lua_newtable (luaVM);
			lua_pushvalue (luaVM, 3);
			lua_setfield (luaVM, -2, "message");
			fmt_stack_item (luaVM, 1);
			lua_setfield (luaVM, -2, "expected");
			fmt_stack_item (luaVM, 2);
			lua_setfield (luaVM, -2, "got");
			lua_pushliteral (luaVM, "value expected not lesser than value got");
			lua_setfield (luaVM, -2, "assert");
			return lua_error (luaVM);
		}
	}
}


/**
 * \brief adds diagnostic output information for one test into a TAP line
 *        luaL_prepbuffer can create userdata on the stack - this function
 *        accounts for that by calling luaL_prepbuffer before everything else
 *        Expects on stack:
 *        1. test case
 *        2. boolean(false) for failed test
 *        3. (userdata) possible by-product of luaL_Buffer
 * \param   luaVM the Lua state.
 * \param  *lB    an already initialized Lua Buffer.
 * \param   p     the position of the test on the stack
 * \param   m     the name of the diagnostic field
 */
static void add_tap_diagnostics (lua_State *luaVM, luaL_Buffer *lB, const char *m)
{
	char *a = luaL_prepbuffer (lB);
	lua_getfield (luaVM, 1, m);
	if (! lua_isnoneornil (luaVM, -1))
	{
		sprintf (a, "\n%-10s : %s", m, lua_tostring (luaVM, -1));
		luaL_gsub (luaVM, a, "\n","\n    ");
		luaL_addsize (lB, 0);
		luaL_addvalue (lB);
	}
	else
	{
		luaL_addsize (lB, 0);
	}
	lua_pop (luaVM, 1);
}


/**
 * \brief formats one test case into a TAP line.
 * \param   luaVM the Lua state.
 * \lparam  xt_test_case  table with the on test_* function.
 * \lparam  xt_test       test suite table.
 */
static int xt_test_case__tostring (lua_State *luaVM)
{
	luaL_Buffer lB;
	luaL_buffinit (luaVM, &lB);
	lua_getfield (luaVM, 1, "pass");   // Stack: 2
	if (lua_isnil (luaVM, -1) )
		luaL_addstring (&lB, "not run");
	else 
		luaL_addstring (&lB, (lua_toboolean (luaVM, -1)) ? "ok ":"not ok ");
	lua_getfield (luaVM, 1, "ord");   // Stack: 3
	luaL_addvalue (&lB);
	luaL_addstring (&lB, " - ");
	// print desc or name
	lua_getfield (luaVM, 1, "desc");    // Stack: 3
	if (lua_isnoneornil (luaVM, -1)) {
		lua_pop (luaVM, 1);              // pop nil desc
		lua_getfield (luaVM, 1, "name"); // Stack: 4
		luaL_addvalue (&lB);              // add name and pop it
	}
	else {
		luaL_addvalue (&lB);              // add desc and pop it
	}
	// Add skip info
	lua_getfield (luaVM, 1, "skip");    // Stack: 3
	if (! lua_isnoneornil (luaVM, -1)) {
		luaL_addstring (&lB, " #SKIP: ");
		luaL_addvalue (&lB);
	}
	else {    // Add todo information?
		lua_pop (luaVM, 1);    // pop skip
		lua_getfield (luaVM, 1, "todo");    // Stack: 3
		if (! lua_isnoneornil (luaVM, -1)) {
			luaL_addstring (&lB, " #TODO: ");
			luaL_addvalue (&lB);
		}
		else {
			lua_pop (luaVM, 1);   // pop todo nil
		}
	}
	luaL_addchar (&lB, '\n');
	// Add diagnostics
	if (! lua_toboolean (luaVM, 2)) {
		luaL_addstring (&lB, "    ---");
		add_tap_diagnostics (luaVM, &lB, "name");
		add_tap_diagnostics (luaVM, &lB, "message");
		add_tap_diagnostics (luaVM, &lB, "assert");
		add_tap_diagnostics (luaVM, &lB, "expected");
		add_tap_diagnostics (luaVM, &lB, "got");
		add_tap_diagnostics (luaVM, &lB, "location");
		add_tap_diagnostics (luaVM, &lB, "traceback");
		add_tap_diagnostics (luaVM, &lB, "src");
		luaL_addstring (&lB, "\n    ...\n");
	}
	lua_pop (luaVM, lua_gettop (luaVM)-1);
	luaL_pushresult(&lB);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   generates a TAP report.
 * \param   luaVM    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test__tostring (lua_State *luaVM)
{
	luaL_Buffer lB;
	int         i=1, t_len;
	xt_test_check_ud  (luaVM, 1);
	t_len           = luaL_len (luaVM, 1);
	luaL_buffinit (luaVM, &lB);
	luaL_addstring(&lB, "1..");
	lua_pushinteger (luaVM, t_len);
	luaL_addvalue(&lB);
	luaL_addchar(&lB, '\n');
	for (; i < t_len+1; i++)
	{
		lua_rawgeti(luaVM, 1, i);
		luaL_getmetafield (luaVM, -1, "__tostring");
		lua_pushvalue (luaVM, 2);         // push table with test function (has __tostring method)
		lua_call (luaVM, 1, 1);
		luaL_addvalue (&lB);               // adds return value to buffer and pops it
		lua_pop (luaVM, 1);                // pop the test case table
	}
	luaL_pushresult(&lB);
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   __call metatable for a single test function.
 * \detail  Stack
 *          1 Test function table
 *          2 Test Suite table
 *          3 order execution number
 * \param   luaVM    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test_case__call (lua_State *luaVM)
{
	int n;           ///< execution order number
	n = luaL_optint (luaVM, 3, 0);
	lua_pushcfunction (luaVM, traceback); // Stack: 4
	lua_getfield (luaVM, 2, "_suitename");// Stack: 5
	lua_getfield (luaVM, 1, "name");      // Stack: 6
	lua_getfield (luaVM, 1, "line");      // Stack: 7
	printf ("%2d - %s.%s:%ld  ... ",
	   n                      , lua_tostring (luaVM, 5),
	   lua_tostring (luaVM, 6), luaL_checkinteger (luaVM, 7));
	lua_pushinteger (luaVM, n);
	lua_setfield (luaVM, 1, "ord");
	fflush (stdout);
	lua_pop (luaVM, 3);                   // pop _suitname,name,line
	lua_getfield (luaVM, 1, "skip");      // Stack: 5
	if (! lua_isnoneornil (luaVM, -1)) {
		printf ("# SKIP:%s\n", lua_tostring (luaVM, -1));
		return 0;
	}
	else {
		lua_pop (luaVM, 1);                // pop skip
		lua_getfield (luaVM, 1, "f");      // Stack: 5
	}

	lua_pushvalue (luaVM, 2);             // push suite as argument for t:test()
	if (lua_pcall (luaVM, 1, 0, 4))
	{
		// Stack: 5  Error table
		printf ("fail\n");
		lua_pushnil (luaVM);
		while (lua_next (luaVM, -2))   // copy error elements to test table 
		{
			lua_setfield (luaVM, 1, lua_tostring (luaVM, -2) );
		}
		lua_pop (luaVM, 2);        // pop the error table and traceback
		lua_pushboolean (luaVM, 0);
		lua_setfield (luaVM, 1, "pass");
		lua_pushboolean (luaVM, 0);
		return 1;
	}
	else
	{
		printf ("ok\n");
		lua_pop (luaVM, 1);        // pop the traceback function
		lua_pushboolean (luaVM, 1);
		lua_setfield (luaVM, 1, "pass");
		lua_pushboolean (luaVM, 1);
		return 1;
	}
}


/**
 * \brief    the metatble for the module
 */
static const struct luaL_Reg xt_test_fm [] = {
	{"__call",              xt_test___call},
	{NULL,                  NULL}
};


/**
 * \brief      the library definition
 *             assigns Lua available names to C-functions
 */
static const struct luaL_Reg xt_test_m [] = {
	{"run",                 xt_test__call},
	{"_eq",                 xt_test_equal},
	{"_eq_not",             xt_test_equal_not},
	{"_lt",                 xt_test_lt},
	{NULL,                  NULL}
};


/**
 * \brief      the library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_test_cf [] =
{
	{"new",                 xt_test_new},
	{"_eq",                 xt_test_equal},
	{"_eq_not",             xt_test_equal_not},
	{"_lt",                 xt_test_lt},
	{NULL,                  NULL}
};


/**--------------------------------------------------------------------------
 * \brief   pushes the Test library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int luaopen_xt_test (lua_State *luaVM) {
	// internal metatable that allows the it to be called
	luaL_newmetatable(luaVM, "xt.Test.Case");   // stack: functions meta
	lua_pushcfunction(luaVM, xt_test_case__call);
	lua_setfield(luaVM, -2, "__call");
	lua_pushcfunction(luaVM, xt_test_case__tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// just make metatable known to be able to register and check type
	// this is only avalable a <instance>:func()
	luaL_newmetatable(luaVM, "xt.Test");   // stack: functions meta
	luaL_newlib(luaVM, xt_test_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, xt_test__newindex);
	lua_setfield(luaVM, -2, "__newindex");
	lua_pushcfunction(luaVM, xt_test__call);
	lua_setfield(luaVM, -2, "__call");
	lua_pushcfunction(luaVM, xt_test__tostring);
	lua_setfield(luaVM, -2, "__tostring");
	lua_pop(luaVM, 1);        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as Test.new() ...
	luaL_newlib(luaVM, xt_test_cf);
	// set the methods as metatable
	// this is only avalable a <instance>:func()
	luaL_newlib(luaVM, xt_test_fm);
	lua_setmetatable(luaVM, -2);
	return 1;
}


