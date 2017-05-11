/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_tst_cse.c
 * \brief     lua-t unit testing framework Case (T.Test.Case)
 *            implemented as Lua Table
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


/**--------------------------------------------------------------------------
 * Creates a traceback from a function call
 * \param   L      Lua state.
 * \lparam  either assert result table or generic string.
 * \lreturn a t_tst result Failure description table.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_tst_cse_traceback( lua_State *L )
{
	char           *msg = NULL;
	const char     *loc;
	if (LUA_TSTRING == lua_type( L, 1 ))
	{
		lua_newtable( L );
		lua_insert( L, 1 );            // S: tbl msg
		loc = lua_tostring( L, 2 );
		msg = strchr( loc, ':' );      // find separator of filename and line number
		if (msg)
		{
			msg++;
			msg = strchr( msg, ':' ) + 2;  // find separator before message
			if (0==strncmp( T_TST_CSE_SKIPINDICATOR, msg, strlen( T_TST_CSE_SKIPINDICATOR ) ))
			{
				lua_pushstring( L, msg+strlen( T_TST_CSE_SKIPINDICATOR) );
				lua_setfield( L, 1, "skip" );
				lua_pushboolean( L, 1 );
				lua_setfield( L, 1, "pass" );
				lua_pop( L, 1 );    // pop original massage
			}
			else
			{
				lua_pushboolean( L, 0 );
				lua_setfield( L, 1, "pass" );
				lua_pushstring( L, msg );
				lua_setfield( L, 1, "message" );
				lua_pushlstring( L, loc, msg-loc-2 );
				lua_setfield( L, 1, "location" );
				lua_pop( L, 1 );                 // pop original message
				luaL_traceback( L, L, NULL, 1 );
				lua_setfield( L, 1, "traceback" );
			}
		}
		else
		{
			lua_setfield( L, 1, "message" );    // set original message
			luaL_where( L, 2 );
			lua_setfield( L, 1, "location" );
			luaL_traceback( L, L, NULL, 1 );
			lua_setfield( L, 1, "traceback" );
		}
	}
	else
		if (LUA_TTABLE == lua_type( L, 1 ))
		{
			//luaL_where( L, 2 );
			//lua_setfield( L, 1, "location" );
			luaL_traceback( L, L, NULL, 1 );
			lua_setfield( L, 1, "traceback" );
		}

	return 1;
}


/**----------------------------------------------------------------------------
 * Put source of function onto stack
 * Use lua debug facilities to determine lines of code and read the code from
 * the source files
 * \param   L        Lua state.
 * \param   int      position of Test.Case on stack.
 * \lparam  table    T.Test.Case Lua table instance.
 * \lreturn string   Source of function.
 * --------------------------------------------------------------------------*/
static size_t
t_tst_cse_getFuncSource( lua_State *L, int pos )
{
	lua_Debug  ar;
	FILE      *f;
	int        r      = 0; ///< current line count
	char       b[ LUAL_BUFFERSIZE ];
	size_t     w;
	size_t     concat = 0;

	lua_getfield( L, pos, "function" );
	lua_getinfo( L, ">S", &ar );

	lua_pushstring( L, "\n    source:" );
	concat++;
	f = fopen( ar.short_src, "r" );
	while (r < ar.lastlinedefined)
	{
		w = sprintf( &(b[0]), "\n        %d: ", r+1 );
		if (NULL == fgets( &(b[0])+w, LUAL_BUFFERSIZE-w, f ))
			break;  // eof?
		//TODO: reasonable line end check
		if (++r < ar.linedefined)
			continue;
		lua_pushlstring( L, &(b[0]), strlen( &(b[0]) )-1 );
		concat++;
	}
	fclose (f);
	lua_concat( L, concat );
	return 1;
}


/** -------------------------------------------------------------------------
 * Search all upvalues on all stack levels for a Test.Case.
 * \param    L        Lua state.
 * \lreturn  table    T.Test.Case Lua table instance.
 * \return   int/bool found or not.
 *-------------------------------------------------------------------------*/
static int
t_tst_cse_findOnStack( lua_State *L )
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


/**--------------------------------------------------------------------------
 * Construct a Test.Case and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Test.Case
 * \lparam  string name of the unit test case function.
 * \lparam  func   test case function.
 * \lreturn table  T.Test.Case Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst_cse__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return t_tst_cse_create( L );
}


/**--------------------------------------------------------------------------
 * Create the Test.Case table and leave it on the stack.
 * \param   L      Lua state.
 * \lparam  string name of the unit test case function.
 * \lparam  func   test case function.
 * \lreturn table  T.Test.Case Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_tst_cse_create( lua_State *L )
{
	const char *name;

	luaL_argcheck( L, lua_type( L, -2 ) == LUA_TSTRING,   -2,
		"testcase description must be a string." );
	luaL_argcheck( L, lua_type( L, -1 ) == LUA_TFUNCTION, -1,
		"testcase value must be a function." );
	name = luaL_checkstring( L, -2 );
	if (0==strncasecmp( name, "test_cb_", 8 ))
		lua_pushstring( L, "callback" );
	else if (0==strncasecmp( name, "test_cr_", 8 ))
		lua_pushstring( L, "coroutine" );
	else
		lua_pushstring( L, "standard" );

	lua_newtable( L );                    //S: nme fnc typ tbl
	lua_insert( L, -4 );                  //S: tbl nme fnc typ
	lua_setfield( L, -4, "testtype" );    //S: tbl nme fnc fnc
	lua_setfield( L, -3, "function" );    //S: tbl nme
	lua_setfield( L, -2, "description" ); //S: tbl

	luaL_getmetatable( L, T_TST_CSE_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a Test.Case
 * \param   L        Lua state.
 * \param   int      position on the stack.
 * \param   int      hardCheck; error out if not a Test.Case.
 * \lparam  table    T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
int
t_tst_cse_check( lua_State *L, int pos, int check )
{
	if (lua_istable( L, pos) && lua_getmetatable( L, pos ))        // does it have a metatable?
	{
		luaL_getmetatable( L, T_TST_CSE_TYPE ); // get correct metatable
		if (! lua_rawequal( L, -1, -2 ))     // not the same?
		{
			lua_pop( L, 2 );
			if (check)
				luaL_error( L, "wrong argument, `"T_TST_CSE_TYPE"` expected" );
			else
				return 0;
		}
		else
		{
			lua_pop( L, 2 );
			return 1;
		}
	}
	else
		if (check)
			luaL_error( L, "wrong argument, `"T_TST_CSE_TYPE"` expected" );
		else
			return 0;
	return 1;
}


/**--------------------------------------------------------------------------
 * Add diagnostic output information for Test.Case into a TAP line
 * \param   L     Lua state.
 * \param   pos   position of T.Test.Case on stack (must be positive!).
 * \param   m     the name of the diagnostic field.
 * \lparam  table T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
static size_t
t_tst_cse_pushTapDetail( lua_State *L, int pos, const char *m )
{
	lua_getfield( L, pos, m );                                     //S: … val
	if (! lua_isnil( L, -1 ))
	{
		if (lua_isboolean( L, -1 ))
			lua_pushfstring( L, "\n%s: %s", m, (lua_toboolean( L, -1 )) ? "True":"False" ); //S: … val str
		else
			lua_pushfstring( L, "\n%s: %s", m, lua_tostring( L, -1 ) ); //S: … val str

		luaL_gsub( L, luaL_checkstring( L, -1 ), "\n", "\n    " );  //S: … val str str
		lua_remove( L, -2 );                                        //S: … val str
		lua_remove( L, -2 );                                        //S: … str
		return 1;
	}
	else
		lua_pop( L, 1 );
	return 0;
}


/**--------------------------------------------------------------------------
 * Add diagnostic output information for Test.Case to a luaL_Buffer
 * \param   L      Lua state.
 * \param   int    position on the stack for Test.Case instance.
 * \lparam  table  T.Test.Case Lua table instance.
 * \lreturn string TAP formatted Test.Case Details.
 * --------------------------------------------------------------------------*/
void
t_tst_cse_addTapDiagnostic( lua_State *L, int pos )
{
	size_t concat = 1;
	lua_pushstring( L, "\n    ---" );
	concat += t_tst_cse_pushTapDetail( L, pos, "description" );
	concat += t_tst_cse_pushTapDetail( L, pos, "testtype" );
	concat += t_tst_cse_pushTapDetail( L, pos, "pass" );
	concat += t_tst_cse_pushTapDetail( L, pos, "skip" );
	concat += t_tst_cse_pushTapDetail( L, pos, "todo" );
	concat += t_tst_cse_pushTapDetail( L, pos, "message" );
	concat += t_tst_cse_pushTapDetail( L, pos, "location" );
	concat += t_tst_cse_pushTapDetail( L, pos, "traceback" );
	concat += t_tst_cse_getFuncSource( L, pos );

	lua_pushstring( L, "\n    ...\n" );
	lua_concat( L, concat+1 );;
}


/**--------------------------------------------------------------------------
 * Push Test.Case description information on stack
 * \param    L      Lua state.
 * \param    pos    int position of Test.Case on Stack.
 * \lparam   table  T.Test.Case Lua table instance.
 * \lresult  string T.Test.Case description.
 * --------------------------------------------------------------------------*/
void
t_tst_cse_getDescription( lua_State *L, int pos )
{
	int concat = 0;
	// description
	lua_getfield( L, pos, "description" );    //S: cse dsc
	concat++;                          //S: … dsc
	if (t_tst_cse_hasField( L, pos, "skip", 1 ))
	{
		lua_pushstring( L, " # SKIP: " );
		lua_insert( L, -2 );            //S: … nme skp
		concat+=2;
	}
	if (t_tst_cse_hasField( L, pos, "todo", 1 ))
	{
		lua_pushstring( L, " # TODO: " );
		lua_insert( L, -2 );            //S: … nme skp
		concat+=2;
	}
	if (concat > 1) lua_concat( L, concat );
}


/**--------------------------------------------------------------------------
 * Formats a test case into a TAP line.
 * \param   L      Lua state.
 * \lparam  table  T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
static int
lt_tst_cse__tostring( lua_State *L )
{
	luaL_Buffer lB;
	t_tst_cse_check( L, 1, 1 );
	luaL_buffinit( L, &lB );

	t_tst_cse_getDescription( L, 1 );
	lua_pushstring( L, "\n" );
	t_tst_cse_addTapDiagnostic( L, 1 );
	lua_concat( L, 3 );
	luaL_addvalue( &lB );
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Prune T.Test.Case from previous execution.
 * Stack:  T.Test.Case
 * \param   L      Lua state.
 * \lparam  table  T.Test.Case Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_tst_cse_prune( lua_State *L )
{
	t_tst_cse_check( L, -1, 1 );
	lua_pushnil( L );
	lua_setfield( L, -2, "pass" );
	lua_pushnil( L );
	lua_setfield( L, -2, "skip" );
	lua_pushnil( L );
	lua_setfield( L, -2, "todo" );
	lua_pushnil( L );
	lua_setfield( L, -2, "message" );
	lua_pushnil( L );
	lua_setfield( L, -2, "location" );
	lua_pushnil( L );
	lua_setfield( L, -2, "traceback" );
	lua_pushnil( L );
	lua_setfield( L, -2, "executionTime" );
	return 0;
}


/**--------------------------------------------------------------------------
 * Re-entrant executor of async T.Test.Case.
 * \param   L      Lua state.
 * \upvalue cse    T.Test.Case instance.
 * \upvalue ste    T.Test Suite instance.
 * \upvalue nxt    t_tst_cse_state of next execution step.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_tst_cse_runSync( lua_State *L )
{
	lua_pushvalue( L, lua_upvalueindex( 1 ) );          //S: cse
	lua_pushvalue( L, lua_upvalueindex( 2 ) );          //S: cse ste

	lua_getfield( L, 2, "beforeEach" );           //S: cse ste bfe
	if (! lua_isnil( L , -1 ))
	{
		lua_pushvalue( L, 2 );                     //S: cse ste bfe ste
		lua_call( L, 1, 0 );
	}
	else
		lua_pop( L, 1 );

	lua_pushcfunction( L, t_tst_cse_traceback );  //S: cse ste tbk
	lua_getfield( L, 1, "function" );             //S: cse ste tbk exc
	lua_pushvalue( L, 2 );                        //S: cse ste tbk exc ste
	if (lua_pcall( L, 1, 0, 3 ))
	{
		lua_pushnil( L );                          //S: cse ste tbk err nil
		while (lua_next( L, -2 ))                  // copy elements from err to Case
			lua_setfield( L, 1, lua_tostring( L, -2 ) );
		lua_pop( L, 2 );                           // pop error table and traceback func
	}
	else
	{
		lua_pop( L, 1 );
		lua_pushboolean( L, 1 );                   //S: cse ste true
		lua_setfield( L, 1, "pass" );
	}

	lua_getfield( L, 2, "afterEach" );            //S: cse ste afe
	if (! lua_isnil( L , -1 ))
	{
		lua_pushvalue( L, 2 );
		lua_call( L, 1, 0 );
	}
	else
		lua_pop( L, 1 );

	t_tst_done( L );                              //S: cse ste
	return 1;
}

static int t_tst_cse_runAsync( lua_State *L );

static void
t_tst_cse_setupClosure( lua_State *L, int state )
{
	lua_pushvalue( L, lua_upvalueindex( 1 ) );     //S: … ste cse
	lua_pushvalue( L, lua_upvalueindex( 2 ) );     //S: … ste cse ste
	lua_pushinteger( L, state );                   //S: … ste cse ste state
	lua_pushcclosure( L, &t_tst_cse_runAsync, 3 ); //S: … ste dne
}


/**--------------------------------------------------------------------------
 * Re-entrant executor of async T.Test.Case.
 * \param   L      Lua state.
 * \upvalue cse    T.Test.Case instance.
 * \upvalue ste    T.Test Suite instance.
 * \upvalue nxt    t_tst_cse_state of next execution step.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_tst_cse_runAsync( lua_State *L )
{
	lua_pop( L, lua_gettop( L ) );                      // wipe stack; remove anything passed to done(...)
	lua_pushvalue( L, lua_upvalueindex( 1 ) );          //S: cse
	lua_pushvalue( L, lua_upvalueindex( 2 ) );          //S: cse ste
	//t_stackDump(L);

	switch (lua_tointeger( L, lua_upvalueindex( 3 ) ))
	{
		case T_TST_CSE_BFE:
			lua_getfield( L, 2, "beforeEach_cb" );        //S: cse ste bfe
			if (! lua_isnil( L , -1 ))
			{
				lua_replace( L, 1 );                       //S: bfe ste
				t_tst_cse_setupClosure( L, T_TST_CSE_EXC );//S: bfa ste exc
				lua_call( L, 2, 0 );
				break;
			}
			else
				lua_pop( L, 1 );                           // pop nil and fall through to exc
		case T_TST_CSE_EXC:
			lua_getfield( L, 1, "function" );             //S: cse ste exc
			lua_insert( L, 2 );                           //S: cse exc ste
			lua_pushcfunction( L, t_tst_cse_traceback );  //S: cse exc cse tbk
			lua_insert( L, 2 );                           //S: cse tbk exc cse
			t_tst_cse_setupClosure( L, T_TST_CSE_AFE );   //S: cse tbk exc ste sfe
			if (lua_pcall( L, 2, 0, 2 ))
			{
				lua_pushnil( L );                          //S: cse tbk err nil
				while (lua_next( L, -2 ))                  // copy elements from err to Case
					lua_setfield( L, 1, lua_tostring( L, -2 ) );
				lua_pop( L, 2 );                           // pop the err tbl and tbk func
				// restore stack by re-adding the suite instance; fall through to the
				// afterEach_cb ... done state to still perform cleanup if needed
				lua_pushvalue( L, lua_upvalueindex( 2 ) );          //S: cse ste
			}
			else
			{
				lua_pop( L, 1 );                           // pop the tbk function
				break;
			}
		case T_TST_CSE_AFE:
			// set pass=true unless traceback has already handles it
			lua_getfield( L, 1, "pass" );                 //S: cse ste pss
			if (lua_isnil( L, -1 ))
			{
				lua_pushboolean( L, 1 );
				lua_setfield( L, 1, "pass" );              //S: cse ste
			}
			lua_pop( L, 2 );                              // pop execTime and pass/nil
			lua_getfield( L, 2, "afterEach_cb" );         //S: cse ste afe
			if (! lua_isnil( L , -1 ))
			{
				lua_replace( L, 1 );                       //S: afe ste
				t_tst_cse_setupClosure( L, T_TST_CSE_DNE );//S: bfa ste dne
				lua_call( L, 2, 0 );
				break;
			}
			else
				lua_pop( L, 1 );                           // pop nil and fall through to done
		case T_TST_CSE_DNE:
			t_tst_done( L );
			break;
		default:
			return luaL_error( L, "Unknown state to asynchronous test case execution" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * Execute T.Test.Case.
 * Stack:  T.Test.Case T.Test
 * \param   L      Lua state.
 * \lparam  table  T.Test.Case Lua table instance.
 * \lparam  table  T.Test Suite Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_tst_cse__call( lua_State *L )
{
	t_tst_cse_check( L, 1, 1 );
	t_tst_check( L, 2 );                           //S: cse ste
	if (t_tst_cse_isType( L, 1, "callback" ))
	{
		lua_pushinteger( L, T_TST_CSE_BFE );           //S: cse ste bfe
		lua_pushcclosure( L, &t_tst_cse_runAsync, 3 ); //S: fnc
	}
	else
		lua_pushcclosure( L, &t_tst_cse_runSync, 2 );  //S: fnc
	lua_call( L, 0, 0 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Is this T.Test.Case marked as "field".
 * \param   L        Lua state.
 * \param   fld      const char field name string.
 * \param   leave    int/bool leave on stack if present or true.
 * \lparam  table    T.Test.Case Lua table instance.
 * \return  int/bool Is it marked as "field"
 * --------------------------------------------------------------------------*/
int
t_tst_cse_hasField( lua_State *L, const int pos, const char *fld, int leave )
{
	int retval = 0;
	lua_getfield( L, pos, fld );
	if (! lua_isnil(L, -1 ))
		retval = (lua_isboolean( L, -1 )) ? lua_toboolean( L, -1 ) : 1;
	if (leave && retval)
		return retval;
	else
		lua_pop( L, 1 );

	return retval;
}


/**--------------------------------------------------------------------------
 * Is this test of type ?
 * \param   L        Lua state.
 * \param   fld      const char field name string.
 * \lparam  table    T.Test.Case Lua table instance.
 * \return  int/bool Is it marked as "field"
 * --------------------------------------------------------------------------*/
int t_tst_cse_isType( lua_State *L, int pos, const char *typeName )
{
	int retval = 0;
	lua_getfield( L, pos, "testtype" );
	lua_pushstring( L, typeName );
	retval = lua_rawequal( L, -1, -2 );
	lua_pop( L, 2 );

	return retval;
}


/** -------------------------------------------------------------------------
 * Mark a Test.Case as Todo.
 * \param    L     Lua state.
 * \lreturn  void.
 * \return   int   # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_tst_cse_Todo( lua_State *L )
{
	luaL_checkstring( L, 1 );
	if (t_tst_cse_findOnStack( L ))
	{
		lua_insert( L, 1 );
		lua_setfield( L, 1, "todo" );
	}
	return 0;
}


/** -------------------------------------------------------------------------
 * Set the description for a Test.Case
 * \param    L     Lua state.
 * \lreturn  void.
 * \return   int   # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_tst_cse_Describe( lua_State *L )
{
	luaL_checkstring( L, 1 );
	if (t_tst_cse_findOnStack( L ))
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
 * \lreturn  void.
 * \return   int   # of values pushed onto the stack.
 *-------------------------------------------------------------------------*/
static int
lt_tst_cse_Skip( lua_State *L )
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
static const struct luaL_Reg t_tst_cse_fm [] = {
	  { "__call"      , lt_tst_cse__Call }
	, { NULL          , NULL }
};


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_tst_cse_cf [] = {
	  { "todo"        , lt_tst_cse_Todo }
	, { "skip"        , lt_tst_cse_Skip }
	, { "describe"    , lt_tst_cse_Describe }
	, { NULL          , NULL }
};


/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_tst_cse_m [] = {
	  { "__call"       , lt_tst_cse__call }
	, { "__tostring"   , lt_tst_cse__tostring }
	, { NULL           ,  NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Test.Case library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      Lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_tst_cse( lua_State *L )
{
	// T.Test.Case instance metatable
	luaL_newmetatable( L, T_TST_CSE_TYPE );
	luaL_setfuncs( L, t_tst_cse_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Test.Case class
	luaL_newlib( L, t_tst_cse_cf );
	luaL_newlib( L, t_tst_cse_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

