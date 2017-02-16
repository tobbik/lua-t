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

#include "t.h"
#include "t_tst.h"
#include "t_tim.h"


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
	char           *msg;
	const char     *loc;
	if (LUA_TSTRING == lua_type( L, 1 ))
	{
		lua_newtable( L );
		lua_insert( L, 1 );
		loc = lua_tostring( L, 2 );
		msg = strchr( loc, ':' ) +1;   // find separator of filename and line number
		msg = strchr( msg, ':' ) + 2;  // find separator before message
		if ( 0==strncmp( T_TST_CSE_SKIPINDICATOR, msg, strlen( T_TST_CSE_SKIPINDICATOR) ))
		{
			lua_pushstring( L, msg+strlen( T_TST_CSE_SKIPINDICATOR) );
			lua_setfield( L, 1, "skip" );
			lua_pop( L, 1 );    // pop original massage
		}
		else
		{
			lua_pushstring( L, msg );
			lua_setfield( L, 1, "message" );
			lua_pushlstring( L, loc, msg-loc-2 );
			lua_setfield( L, 1, "location" );
			lua_pop( L, 1 );    // pop original massage
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
 * Inspects source for special lines in the comments
 * Use lua debug facilities to determine lines of code and read the code from
 * the source files
 * \param   L        Lua state.
 * \param   int      position of Test.Case on stack.
 * \lparam  table    T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
static void
t_tst_cse_getFuncSource( lua_State *L, int pos, luaL_Buffer *lB )
{
	lua_Debug  ar;
	FILE      *f;
	int        r = 0; ///< current line count
	char      *p;
	size_t     w;

	lua_getfield( L, pos, "function" );
	lua_getinfo( L, ">S", &ar );

	p = luaL_prepbuffer( lB );
	w = sprintf( p, "\n    source:" );
	luaL_addsize( lB, w );
	f = fopen( ar.short_src, "r" );
	while (r < ar.lastlinedefined)
	{
		p = luaL_prepbuffer( lB );
		w = sprintf( p, "\n        %d: ", r+1 );
		if (NULL == fgets( p+w, LUAL_BUFFERSIZE-w, f ))
			break;  // eof?
		//TODO: reasonable line end check
		if (++r < ar.linedefined)
			continue;
		luaL_addsize( lB, strlen( p )-1 );
	}

	fclose (f);
}


/**--------------------------------------------------------------------------
 * construct a Test Case and return it.
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
 * create the Test Case table and return it.
 * \param   L      Lua state.
 * \lparam  string name of the unit test case function.
 * \lparam  func   test case function.
 * \lreturn table  T.Test.Case Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_tst_cse_create( lua_State *L )
{
	luaL_argcheck( L, lua_type( L, -2 ) == LUA_TSTRING,   -2, "testcasename must be a string." );
	luaL_argcheck( L, lua_type( L, -1 ) == LUA_TFUNCTION, -1, "testcase value must be a function." );
	lua_newtable( L );                 //S: nme fnc tbl
	lua_insert( L, -3 );               //S: tbl nme fnc
	lua_setfield( L, -3, "function" ); //S: tbl nme
	lua_setfield( L, -2, "name" );     //S: tbl

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
	return t_checkTableType( L, pos, check, T_TST_CSE_TYPE );
}


/**--------------------------------------------------------------------------
 * Add diagnostic output information for Test.Case into a TAP line
 * Expects on stack:
 *        1. test case
 *        2. boolean(false) for failed test
 *        3. (userdata) possible by-product of luaL_Buffer
 * \param   L     Lua state.
 * \param  *lB    an already initialized Lua Buffer.
 * \param   pos   position of T.Test.Case on stack (must be positive!).
 * \param   m     the name of the diagnostic field.
 * \lparam  table T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
static void
t_tst_cse_addTapDetail( lua_State *L, luaL_Buffer *lB, int pos, const char *m )
{
	lua_getfield( L, pos, m );
	if (! lua_isnil( L, -1 ))
	{
		lua_pushfstring( L, "\n%s: %s", m, lua_tostring( L, -1 ) ); //S: … val str
		luaL_gsub( L, luaL_checkstring( L, -1 ), "\n", "\n    " );  //S: … val str str
		luaL_addvalue( lB );                                        //S: … val str
		lua_pop( L, 2 );
	}
	else
		lua_pop( L, 1 );
}


/**--------------------------------------------------------------------------
 * Add diagnostic output information for Test.Case to a luaL_Buffer
 * \param   L     Lua state.
 * \param  *lB    an already initialized Lua Buffer.
 * \param   int   position on the stack for Test.Case instance.
 * \lparam  table T.Test.Case Lua table instance.
 * --------------------------------------------------------------------------*/
void
t_tst_cse_addTapDiagnostic( lua_State *L, luaL_Buffer *lB, int pos )
{
	luaL_addstring( lB, "    ---" );
	t_tst_cse_addTapDetail( L, lB, pos, "description" );
	t_tst_cse_addTapDetail( L, lB, pos, "name" );
	t_tst_cse_addTapDetail( L, lB, pos, "pass" );
	t_tst_cse_addTapDetail( L, lB, pos, "skip" );
	t_tst_cse_addTapDetail( L, lB, pos, "todo" );
	t_tst_cse_addTapDetail( L, lB, pos, "message" );
	t_tst_cse_addTapDetail( L, lB, pos, "location" );
	t_tst_cse_addTapDetail( L, lB, pos, "traceback" );
	t_tst_cse_getFuncSource( L, pos, lB );

	luaL_addstring( lB, "\n    ...\n" );
}


/**--------------------------------------------------------------------------
 * Push Test.Case name information on stack
 * \param    L      Lua state.
 * \param    pos    int position of Test.Case on Stack.
 * \lparam   table  T.Test.Case Lua table instance.
 * \lresult  string T.Test.Case description.
 * --------------------------------------------------------------------------*/
void
t_tst_cse_getDescription( lua_State *L, int pos )
{
	int concat = 0;
	// print description or name
	lua_getfield( L, pos, "description" );    //S: cse dsc
	if( lua_isnil( L, -1 ))
	{
		lua_pop( L, 1 );
		lua_getfield( L, pos, "name" ); //S: cse nme
	}
	concat++;                          //S: … nme
	// Add skip info
	lua_getfield( L, pos, "skip" );    //S: … nme skp
	if (! lua_isnil( L, -1 ))
	{
		lua_pushstring( L, " # SKIP: " );
		lua_insert( L, -2 );            //S: … nme skp
		concat+=2;
	}
	else
		lua_pop( L, 1 );                // pop skip nil
	// Add todo information?
	lua_getfield( L, pos, "todo" );    //S: … nme tdo
	if (! lua_isnil( L, -1 ))
	{
		lua_pushstring( L, " # TODO: " );
		lua_insert( L, -2 );            //S: … nme tdo
		concat+=2;
	}
	else
		lua_pop( L, 1 );                // pop todo nil
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
	luaL_addvalue( &lB );
	luaL_addchar( &lB, '\n' );
	t_tst_cse_addTapDiagnostic( L, &lB, 1 );
	luaL_pushresult( &lB );
	return 1;
}


/**--------------------------------------------------------------------------
 * Execution envelope for a test harness.
 * \param   L        Lua state.
 * \param   field    const char* name to execute.
 * \param   pos      position of T.Test table on stack to fetch function from.
 * \lparam  table    T.Test.Suite Lua table instance.
 * --------------------------------------------------------------------------*/
static void
t_tst_cse_envelope( lua_State *L, char *field, int pos )
{
	lua_getfield( L, pos, field );
	if (! lua_isnil( L, -1 ))
	{
		lua_pushvalue( L, pos );        // S: cse ste fnc ste
		if (lua_pcall( L, 1, 0, 0 ))
			luaL_error( L, "Test %s failed %s", field, lua_tostring( L, -1 ) );
	}
	else
		lua_pop( L, -1 );
}


/**--------------------------------------------------------------------------
 * Execution wrapper for T.Test.Case.
 * Stack:
 *          1 Test function table
 *          2 Test Suite table
 * \param   L      Lua state.
 * \lparam  table  T.Test.Suite Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
t_tst_cse_execute( lua_State *L )
{
	int is_pass   = 0,
	    is_todo   = 0,
	    is_skip   = 0;

	lua_getfield( L, 1, "function" );             // S: cse ste fnc
	lua_insert( L, -2 );                          // S: cse fnc ste
	lua_pushcfunction( L, t_tst_cse_traceback );  // S: cse fnc ste tbk
	lua_insert( L, 2 );                           // S: cse tbk fnc ste

	if (lua_pcall( L, 1, 0, 2 ))
	{
		lua_pushnil( L );                          // S: tbk cse fnc ste err nil
		while (lua_next( L, -2 ))                  // copy elements from err to Case
			lua_setfield( L, 1, lua_tostring( L, -2 ) );
		lua_pop( L, 2 );                           // pop the err tbl and tbk func
	}
	else
	{
		lua_pop( L, 1 );                           // pop the tbk function
		is_pass = 1;
	}
	is_todo = t_tst_cse_hasField( L, "todo", 0 );
	is_skip = t_tst_cse_hasField( L, "skip", 0 );
	lua_pushboolean( L, is_pass || is_skip );     // S: cse pass
	lua_setfield( L, 1, "pass" );
	if (is_skip || (is_todo && ! is_pass) || (! is_todo && is_pass))
		lua_pushboolean( L, 1 );                   // S: cse tru
	else
		lua_pushboolean( L, 0 );                   // S: cse fls
	return 1;
}


/**--------------------------------------------------------------------------
 * Execute T.Test.Case
 * Stack:
 *          1 Test function table
 *          2 Test Suite table
 * \param   L      Lua state.
 * \lparam  table  T.Test.Suite Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_tst_cse__call( lua_State *L )
{
	t_tst_cse_envelope( L, "setUp", 2 );
	lua_pushvalue( L, 1 );                        // S: cse ste cse
	lua_pushcclosure( L, &t_tst_cse_execute, 1 ); // S: cse ste cls
	lua_pushvalue( L, 1 );                        // S: cse ste cls cse
	lua_pushvalue( L, 2 );                        // S: cse ste cls cse ste
	lua_call( L, 2, 1 );

	t_tst_cse_envelope( L, "tearDown", 2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Is this T.Test.Case marked as "field"
 * \param   L        Lua state.
 * \param   fld      const char field name string.
 * \param   leave    int/bool leave on stack if present/true.
 * \lparam  table    T.Test.Case Lua table instance.
 * \return  int/bool Is it marked as "field"
 * --------------------------------------------------------------------------*/
int t_tst_cse_hasField( lua_State *L, const char *fld, int leave )
{
	int retval = 0;
	lua_getfield( L, -1, fld );
	if (! lua_isnil(L, -1 ))
		retval = (lua_isboolean( L, -1 )) ? lua_toboolean( L, -1 ) : 1;
	if (leave && retval)
		return retval;
	else
		lua_pop( L, 1 );

	return retval;
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
	  { NULL,  NULL }
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
 * Pushes the Test library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
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

