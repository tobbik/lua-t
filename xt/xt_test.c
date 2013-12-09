/*
 * \file    xt_test.c
 * \brief   xt unit testing framework
 * \detail  OOP wrapper for Test cases. Unit
*/
#include <string.h>            //strlen
#include <strings.h>           //strncasecmp

#include "l_xt.h"


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
static void fmt_stack_item (lua_State *luaVM, int i)
{
	int t;
	if (!luaL_callmeta (luaVM,i,"__tostring"))
	{
		t = lua_type(luaVM, i);
		switch (t) {
			case LUA_TSTRING:    /* strings */
				lua_pushvalue (luaVM, i);
				break;
			case LUA_TBOOLEAN:   /* booleans */
				if (lua_toboolean (luaVM, i)) lua_pushliteral (luaVM, "true");
				else lua_pushliteral (luaVM, "false");
				break;
			case LUA_TNUMBER:    /* numbers */
				lua_pushfstring (luaVM, "%f", lua_tonumber(luaVM, i));
				break;
			default:	            /* other values */
				lua_pushfstring (luaVM, "%s", lua_typename(luaVM, t));
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
 * \param   test_pos Position of main test instance on stack.
 * \lparam  test_case table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int traceback (lua_State *luaVM) {
	const char *msg = lua_tostring (luaVM, 1);
	if (msg) {
		luaL_traceback(luaVM, luaVM,msg, 1);
	}
	else if (!lua_isnoneornil(luaVM, 1)) {  /* is there an error object? */
    if (!luaL_callmeta(luaVM, 1, "__tostring"))  /* try its 'tostring' metamethod */
      lua_pushliteral(luaVM, "(no error message)");
  }
  return 1;
}


/**--------------------------------------------------------------------------
 * \brief   genrates a TAP report.
 * \param   luaVM    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test_totap (lua_State *luaVM)
{
	int         i;

	xt_test_check_ud  (luaVM, 1);
	lua_getfield (luaVM, 1, "_suitename");        // Stack: 2


	lua_getfield (luaVM, 1, "setUp");
	lua_pushvalue (luaVM, 1);
	if (lua_pcall (luaVM, 1, 0, 0))
		xt_push_error (luaVM, "Test setup failed %s", lua_tostring (luaVM, -1));
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last (numeric) index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}
		else
		{
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * \brief   executes the test suite.
 * \param   luaVM    The lua state.
 * \lparam  test instance table.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int xt_test__call (lua_State *luaVM)
{
	int         i;
	int         vrb = 1;

	xt_test_check_ud  (luaVM, 1);
	if (2==lua_gettop (luaVM)) {
		stackDump(luaVM);
		vrb = lua_toboolean (luaVM, 2);
		lua_pop(luaVM, 1);
	}
	lua_getfield (luaVM, 1, "_suitename");        // Stack: 2
	lua_pushcfunction (luaVM, traceback);               // Stack: 3

	lua_getfield (luaVM, 1, "setUp");
	lua_pushvalue (luaVM, 1);
	if (lua_pcall (luaVM, 1, 0, 0))
		xt_push_error (luaVM, "Test setup failed %s", lua_tostring (luaVM, -1));
	for ( i=1 ; ; i++ )
	{
		lua_rawgeti(luaVM, 1, i);
		// in table this is when the last (numeric) index is found
		if ( lua_isnil(luaVM, -1) )
		{
			lua_pop(luaVM, 1);
			break;
		}
		else
		{
			lua_getfield (luaVM, -1, "name");
			printf("%s:%s: ... ", lua_tostring (luaVM, 2), lua_tostring (luaVM, -1));
			lua_pop (luaVM, 1);
			lua_getfield (luaVM, -1, "func");
			lua_pushvalue (luaVM, 1);            // push suit as argument for t:test()
			if (lua_pcall (luaVM, 1, 0, 3))
			{
				printf("not ok\n");
				if(vrb) {
					printf("--------------------\nerror: %s\n---------------------\n",
						lua_tostring (luaVM, -1));
				}
				lua_setfield (luaVM, -2, "error");   // record error message
			}
			else
			{
				printf("ok\n");
			}
		}
		lua_pop (luaVM, 1);     // pop the test case table
	}
	lua_getfield (luaVM, 1, "tearDown");
	lua_pushvalue (luaVM, 1);
	if (lua_pcall (luaVM, 1, 0, 0))
		xt_push_error (luaVM, "Test tearDown failed %s", lua_tostring (luaVM, -1));

	return 0;
}


/** This can be used to read in source code according to debug info
 * untested and unfinished!
 */
void __attribute__ ((unused)) fetchDeco (lua_State *luaVM, const char* fn, int s, int e) {
	size_t           l;
	int              r=1;
	char            *p;
	luaL_Buffer      b;
	FILE            *f = fopen (fn, "r");
	luaL_buffinit (luaVM, &b);
	printf("%s    %d   %d  %zu\n", fn, s, e, lua_rawlen(luaVM, -1));

	while (r<s) {

		if (fgetc (f) == '\n')
			r++;
	}
	while (r<e) {
		p = luaL_prepbuffer(&b);
		if (fgets(p, LUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
			luaL_pushresult(&b);  /* close buffer */
		}
		l = strlen(p);
		printf("%s\n", p);
		if (l == 0 || p[l-1] != '\n')
			luaL_addsize(&b, l);
		else {
			luaL_addsize(&b, l - 1);  /* chop 'eol' if needed */
			luaL_pushresult(&b);  /* close buffer */
		}
	}
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
			lua_newtable (luaVM);
			lua_pushvalue(luaVM, 3);                   // copy f()  from 3 to 4
			lua_setfield (luaVM, -2, "func");
			lua_pushvalue(luaVM, 2);                   // copy name from 2 to 4
			lua_setfield (luaVM, -2, "name");

			lua_rawseti (luaVM, 1, lua_rawlen(luaVM, 1) +1);
		}
		else
		{
			return xt_push_error(luaVM,
				"test* named elements must be methods");
		}
	}
	// insert in local table
	lua_rawset (luaVM, 1);

	return 0;
}


/** ---------------------------------------------------------------------------
 * \brief internal table iterator that deep compares two tables
 * \deatil  work on negative inices ONLY for recursive use
 * \param   luaVM lua_State
 * \lparam  table 1
 * \lparam  table 2
 * \return  boolean in 1 or 0
 *--------------------------------------------------------------------------- */
static int tablecmp (lua_State *luaVM)
{
	if (LUA_TTABLE != lua_type (luaVM, -1)  || LUA_TTABLE != lua_type (luaVM, -2) )
	{
		return 0;
	}
	lua_pushnil (luaVM);           // Stack: tableA tableB  nil
	while (lua_next(luaVM, -3))    // Stack: tableA tableB  keyA  valueA
	{
		lua_pushvalue (luaVM, -2);  // Stack: tableA tableB  keyA  valueA  keyA
		lua_gettable( luaVM, -4);   // Stack: tableA tableB  keyA  valueA  valueB
		if (LUA_TTABLE == lua_type (luaVM, -2) && !tablecmp (luaVM))
		{
			lua_pop (luaVM, 3);      // Stack: tableA tableB 
			return 0;
		}
		if (!lua_compare (luaVM, -2, -1, LUA_OPEQ))
		{
			lua_pop (luaVM, 3);      // Stack: tableA tableB 
			return 0;
		}
		else {
			lua_pop(luaVM, 2);       // pop valueA and valueB
		}
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
		stackDump(luaVM);
		return xt_push_error (luaVM, "xt.Test._equals expects two or three arguments");
	}
	// compare types, references, metatable.__eq and values
	if (lua_compare (luaVM, 1, 2, LUA_OPEQ) ||
	 (LUA_TTABLE == lua_type (luaVM,1) && tablecmp (luaVM)))
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
			fmt_stack_item (luaVM, 1);
			lua_pushliteral (luaVM, " not equal to ");
			fmt_stack_item (luaVM, 2);
			lua_concat (luaVM, 3);
			return xt_push_error (luaVM, "%s\n\t%s",
				lua_tostring (luaVM, 3), lua_tostring (luaVM, 4));
		}
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
	{"_equal",              xt_test_equal},
	{NULL,                  NULL}
};


/**
 * \brief      the library class functions definition
 *             assigns Lua available names to C-functions
 */
static const luaL_Reg xt_test_cf [] =
{
	{"new",                 xt_test_new},
	{"_equal",              xt_test_equal},
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
	// just make metatable known to be able to register and check type
	// this is only avalable a <instance>:func()
	luaL_newmetatable(luaVM, "xt.Test");   // stack: functions meta
	luaL_newlib(luaVM, xt_test_m);
	lua_setfield(luaVM, -2, "__index");
	lua_pushcfunction(luaVM, xt_test__newindex);
	lua_setfield(luaVM, -2, "__newindex");
	lua_pushcfunction(luaVM, xt_test__call);
	lua_setfield(luaVM, -2, "__call");
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
