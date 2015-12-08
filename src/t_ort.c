/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_ort.c
 * \brief     data types for an ordered table
 *            Elements can be accessed by their name and their index. Basically
 *            an ordered hashmap.  It is implemented as intelligent mapper around
 *            a Lua table.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset

#include "t.h"
#include "t_ort.h"


/**--------------------------------------------------------------------------
 * Create a new t.OrderedTable and return it.
 * \param   L  The lua state.
 * \lreturn struct t_ort userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ort_New( lua_State *L )
{
	struct t_ort __attribute__ ((unused)) *ort = t_ort_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a t.OrderedTable and return it.
 * \param   L  The lua state.
 * \lparam  CLASS table t.OrderedTable.
 * \lreturn struct t_ort userdata.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_ort__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_ort_New( L );
}


/**--------------------------------------------------------------------------
 * Create a new t_ort userdata and push to LuaStack.
 * \param   L  The lua state.
 * \param   int start position on stack for elements.
 * \return  struct t_ort * pointer to new userdata on Lua Stack
 * --------------------------------------------------------------------------*/
struct t_ort
*t_ort_create_ud( lua_State *L )
{
	struct t_ort    *ort;

	ort = (struct t_ort *) lua_newuserdata( L, sizeof( struct t_ort ) );
	// create and populate index table
	lua_newtable( L );

	ort->tR = luaL_ref( L, LUA_REGISTRYINDEX );
	//ort->fd_sz   = sz;
	//ort->max_fd  = 0;
	//ort->tm_head = NULL;
	//ort->fd_set  = (struct t_ort_fd **) malloc( (ort->fd_sz+1) * sizeof( struct t_ort_fd * ) );
	//for (n=0; n<=ort->fd_sz; n++) ort->fd_set[ n ] = NULL;
	//t_ort_create_ud_impl( ort );
	luaL_getmetatable( L, "T.OrderedTable" );
	lua_setmetatable( L, -2 );
	return ort;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_ort
 * \param   L    The lua state.
 * \param   int      position on the stack
 * \param   int      check(boolean): if true error out on fail
 * \return  struct t_ort*  pointer to userdata on stack
 * --------------------------------------------------------------------------*/
struct t_ort
*t_ort_check_ud ( lua_State *L, int pos, int check )
{
	void *ud = luaL_checkudata( L, pos, "T.OrderedTable" );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`T.OrderedTable` expected" );
	return (NULL==ud) ? NULL : (struct t_ort *) ud;
}


/**--------------------------------------------------------------------------
 * Read an OrderedTable value.
 * \param   L    The lua state.
 * \lparam  userdata T.OrderedTable instance.
 * \lparam  key      string/integer.
 * \lreturn userdata T.OrderedTable instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ort__index( lua_State *L )
{
	const  char  *key;
	struct t_ort *ort = t_ort_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, ort->tR );
	if (LUA_TNUMBER == lua_type( L, -2 ) )
	{
		lua_rawgeti( L, -1, luaL_checkinteger( L, -2) );
		return 1;
	}
	else
	{
		key = luaL_checkstring( L, -2 );
		lua_pushvalue( L, -2 );        // Stack: ort,key,ref,key
		lua_rawget( L, -2 );           // Stack: ort,key,ref,i
		lua_rawgeti( L, -2, lua_tointeger( L, -1) );
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Read an OrderedTable value.
 * \param   L    The lua state.
 * \lparam  userdata T.OrderedTable instance.
 * \lparam  key      string/integer.
 * \lreturn userdata T.OrderedTable instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ort__newindex( lua_State *L )
{
	const char   *key;
	int           idx;
	size_t        len;
	struct t_ort *ort = t_ort_check_ud( L, -3, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, ort->tR ); // S: ort,key/id,val,rft
	len = lua_rawlen( L, -1 );

	if (LUA_TNUMBER == lua_type( L, -3 ) )
	{
		idx = luaL_checkinteger( L, -3 );
		luaL_argcheck( L, 1 <= idx && idx <= (int) len, -3,
			"Index must be greater than 1 and lesser than array length" );
		lua_pushvalue( L, -2 );
		lua_rawseti( L, -2, idx );
		return 1;
	}
	else
	{
		// S: ort,key/id,val,rft
		key = luaL_checkstring( L, -3 );
		lua_pushvalue( L, -3 );
		lua_rawget( L, -2 );
		if (lua_isnoneornil( L, -1 ))
		{
			lua_pop( L, 1 );
			lua_pushvalue( L, -3 );
			lua_pushinteger( L, len+1 );  // S: ort,key/id,val,rft,key,i
			lua_rawset( L, -3 );
			lua_pushvalue( L, -2 );
			lua_pushinteger( L, len+1 );  // S: ort,key/id,val,rft,val,i
			lua_rawset( L, -3 );
		}
		else
		{
			// S: ort,key/id,val,rft,idx
			idx = lua_tointeger( L, -1 );
			lua_pushvalue( L, -3 );
			lua_rawset( L, -3 );
		}
		lua_pop( L, 1 );
		return 0;
	}
}


/**--------------------------------------------------------------------------
 * Returns len of the ordrred table
 * \param   L    The Lua state
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ort__len( lua_State *L )
{
	struct t_ort *ort = t_ort_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, ort->tR );
	lua_pushinteger( L, lua_rawlen( L, -1 ) );
	lua_pop( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a ordered table stream.
 * \param   L     The lua state.
 * \lreturn string    formatted string representing ordered table.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_ort__tostring( lua_State *L )
{
	struct t_ort *ort = t_ort_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, ort->tR );
	lua_pushfstring( L, "T.OrderedTable[%d]: %p", lua_rawlen( L, -1 ), ort );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ort_fm [] = {
	{"__call",        lt_ort__Call},
	{NULL,            NULL}
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_ort_cf [] = {
	{"new",           lt_ort_New},
	{NULL,            NULL}
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_ort_m [] = {
	//{ "__tostring", lt_ort__tostring },
	//{ "__len",      lt_ort__len },
	{ "__index",    lt_ort__index },
	{ "__newindex", lt_ort__newindex },
	{NULL,    NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T.OrderedTable library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_ort( lua_State *L )
{
	// T.Pack.Struct instance metatable
	luaL_newmetatable( L, "T.OrderedTable" );   // stack: functions meta
	luaL_setfuncs( L, t_ort_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Pack.<member>
	luaL_newlib( L, t_ort_cf );
	luaL_newlib( L, t_ort_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
