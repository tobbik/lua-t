/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_oht.c
 * \brief     data types for an ordered Lua table
 *            Elements can be accessed by their name and their index. Basically
 *            an ordered hashmap.  It is implemented as intelligent mapper around
 *            a Lua table.  The basic design handles the values as following:
 *            1   = "value 1",
 *            2   = "value 2",
 *            3   = "value 3",
 *            4   = "value 4",
 *            "a" = 1,
 *            "b" = 2,
 *            "c" = 3,
 *            "d" = 4
 *
 *            1   = 'a',
 *            2   = 'b',
 *            3   = 'c',
 *            4   = 'd',
 *            "a" = "value 1",
 *            "b" = "value 2",
 *            "c" = "value 3",
 *            "d" = "value 4",
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset

#include "t.h"
#include "t_oht.h"


/**--------------------------------------------------------------------------
 * Create a new T.OrderedHashTable and return it.
 * \param   L        The lua state.
 * \lreturn struct   t_oht userdata.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_New( lua_State *L )
{
	struct t_oht __attribute__ ((unused)) *oht = t_oht_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a t.OrderedTable and return it.
 * \param   L        The lua state.
 * \lparam  CLASS    table T.OrderedHashTable.
 * \lreturn struct   t_oht userdata.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_oht__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_oht_New( L );
}


/**--------------------------------------------------------------------------
 * Create a new t_oht userdata and push to LuaStack.
 * \param   L        The lua state.
 * \param   int      Start position on stack for elements.
 * \return  struct   t_oht * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_oht
*t_oht_create_ud( lua_State *L )
{
	struct t_oht    *oht;

	oht = (struct t_oht *) lua_newuserdata( L, sizeof( struct t_oht ) );
	// create and populate table
	lua_newtable( L );
	oht->tR = luaL_ref( L, LUA_REGISTRYINDEX );

	luaL_getmetatable( L, T_OHT_TYPE );
	lua_setmetatable( L, -2 );
	return oht;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_oht
 * \param   L        The lua state.
 * \param   int      Position on the stack.
 * \param   int      check(boolean): if true error out on fail.
 * \return  struct   t_oht*  pointer to userdata on stack.
 * --------------------------------------------------------------------------*/
struct t_oht
*t_oht_check_ud ( lua_State *L, int pos, int check )
{
	void *ud = luaL_checkudata( L, pos, T_OHT_TYPE );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_OHT_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_oht *) ud;
}


/**--------------------------------------------------------------------------
 * Read element from OrderedHashTable value.
 * \param   L          The lua state.
 * \lparam  userdata   T.OrderedHashTable instance.
 * \lparam  key        string/integer.
 * \lreturn value      value from index/hash.
 * \return  int        # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__index( lua_State *L )
{
	const  char  *key;
	struct t_oht *oht = t_oht_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );
	if (LUA_TNUMBER == lua_type( L, -2 ) )
	{
		lua_rawgeti( L, -1, luaL_checkinteger( L, -2) );
		return 1;
	}
	else
	{
		key = luaL_checkstring( L, -2 );
		lua_pushvalue( L, -2 );        // S: oht,key,ref,key
		lua_rawget( L, -2 );           // S: oht,key,ref,i
		lua_rawgeti( L, -2, lua_tointeger( L, -1) );
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Read an OrderedTable value.
 * \param   L          The lua state.
 * \lparam  userdata   T.OrderedTable instance.
 * \lparam  key        string/integer.
 * \lparam  value      value for index/hash.
 * \return  int        # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__newindex( lua_State *L )
{
	const char   *key;
	int           idx;
	size_t        len;
	struct t_oht *oht = t_oht_check_ud( L, -3, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht,key/id,val,tbl
	lua_replace( L, -4 );
	len = lua_rawlen( L, -3 );                    // S: tbl,key/id,val

	// Numeric indices can only be used to replace values on an ordered  hash table
	if (LUA_TNUMBER == lua_type( L, -2 ) )
	{
		idx = luaL_checkinteger( L, -2 );
		luaL_argcheck( L, 1 <= idx && idx <= (int) len, -2,
			"Index must be greater than 1 and lesser than array length" );
		lua_rawseti( L, -3, idx );
		return 0;
	}
	else
	{
		key = luaL_checkstring( L, -2 );
		lua_pushvalue( L, -2 );    // S: tbl,key,val,key
		lua_rawget( L, -4 );       // S: tbl,key,val,idx?
		if (lua_isnoneornil( L, -1 )) // add a new value to the table
		{
			lua_pop( L, 1 );
			lua_pushvalue( L, -2 );
			lua_pushinteger( L, len+1 );  // S: tbl,key,val,key,i
			lua_rawset( L, -5 );          // S: tbl,key,val
			lua_rawseti( L, -3, len+1 );
		}
		else                         // replace a value in the table
		{
			idx = lua_tointeger( L, -1 );
			lua_pop( L, 1 );              // S: tbl,key,val
			lua_rawseti( L, -3, idx );
			lua_pop( L, 1 );              // S: tbl
		}
		return 0;
	}
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.OrderedHashTable.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   L lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_iter( lua_State *L )
{
	t_stackDump( L );
	struct t_oht *oht = t_oht_check_ud( L, lua_upvalueindex( 1 ), 1);
	// get current index and increment
	int           len = lua_tointeger( L, lua_upvalueindex( 2 ) );
	int           idx = lua_tointeger( L, lua_upvalueindex( 3 ) ) + 1;
	printf( "%d...%d\n", idx, len );

	// get the referenced table onto the stack

	if (idx > len)
		return 0;
	else
	{
		lua_pushinteger( L, idx );
		lua_replace( L, lua_upvalueindex( 2 ) );
		lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: f,f,tbl
		lua_pushinteger( L, idx );                    // S: f,f,tbl,idx
		//lua_rawget( L, -1, lua_pushinteger( L,  );                    // push value
		lua_rawgeti( L, -2, idx );                    // push value
		return 2;
	}
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L         Lua Virtual Machine.
 * \lparam  iterator  T.OrderedHashTable.
 * \lreturn pos       position in t_buf.
 * \return  int       # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_oht__pairs( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	//lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht,tbl
	//lua_replace( L, -2 );                         // S: tbl

	lua_pushnumber( L, lua_rawlen( L, -1 ) -1 );     // keep length for iteration
	lua_pushnumber( L, 0 );
	t_stackDump( L );
	lua_pushcclosure( L, &t_oht_iter, 2 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht,tbl
	lua_pushnil( L );
	t_stackDump( L );
	return 3;
}

/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.OrderedHashTable.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   L lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return  int    # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_iiter( lua_State *L )
{
	lua_Integer      idx  =     luaL_checkinteger( L, 2 ) + 1;
	luaL_checktype( L, 1, LUA_TTABLE );
	lua_pushinteger( L, idx );
	t_stackDump( L );
	if (LUA_TNIL != lua_rawgeti( L, -3, idx ))
	{
		lua_pushinteger( L, idx );
		lua_rawget( L, -5 );
		t_stackDump( L );
		return 3;
	}
	else
		return 1;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L         Lua Virtual Machine.
 * \lparam  iterator  T.OrderedHashTable.
 * \lreturn pos       position in t_buf.
 * \return  int       # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_oht__ipairs( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht,tbl
	lua_replace( L, -2 );                         // S: tbl

	lua_pushnumber( L, lua_rawlen( L, -1 ) );     // keep length for iteration
	lua_pushnumber( L, 0 );
	t_stackDump( L );
	lua_pushcclosure( L, &t_oht_iiter, 2 );
	t_stackDump( L );
	lua_pushvalue( L, -2 );
	lua_pushinteger( L, 0 );
	t_stackDump( L );
	return 3;
}



/**--------------------------------------------------------------------------
 * Returns len of the ordered hash table.
 * \param   L    The Lua state.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__len( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );
	lua_pushinteger( L, lua_rawlen( L, -1 ) );
	lua_remove( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a ordered table stream.
 * \param   L     The lua state.
 * \lreturn string    formatted string representing ordered table.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__tostring( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );
	lua_pushfstring( L, T_OHT_TYPE"[%d]: %p", lua_rawlen( L, -1 ), oht );
	lua_remove( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_oht_fm [] = {
	  { "__call"       , lt_oht__Call }
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_oht_cf [] = {
	  { "new"          , lt_oht_New }
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_oht_m [] = {
	  { "__tostring"   , lt_oht__tostring }
	, { "__len"        , lt_oht__len }
	, { "__index"      , lt_oht__index }
	, { "__newindex"   , lt_oht__newindex }
	, { "__pairs"      , lt_oht__pairs }
	, { "__ipairs"     , lt_oht__ipairs }
	, { NULL           , NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T.OrderedHashTable library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_oht( lua_State *L )
{
	// T.OrderedHashTable instance metatable
	luaL_newmetatable( L, T_OHT_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_oht_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.OrderedHashTable.<member>
	luaL_newlib( L, t_oht_cf );
	luaL_newlib( L, t_oht_fm );
	lua_setmetatable( L, -2 );
	return 1;
}
