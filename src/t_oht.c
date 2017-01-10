/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_oht.c
 * \brief     Functions for an ordered Lua table.
 *            Elements can be accessed by their name or their index.  Basically
 *            an ordered hashmap.  It is implemented as intelligent mapper around
 *            a Lua table.  The basic design handles the values as following:
 *            1   = 'a',
 *            2   = 'b',
 *            3   = 'c',
 *            4   = 'd',
 *            "a" = "value 1",
 *            "b" = "value 2",
 *            "c" = "value 3",
 *            "d" = "value 4"
 *   17164 Jan 10 13:46 src/t_oht.c
 *    1004 Jan  3 01:22 src/t_oht.h
 *   24376 Jan 10 13:46 src/t_oht.o
 *            This design allows for numeric iteration no matter if pairs or
 *            ipairs is used since the current index can be carried as an
 *            upvalue.







 *            1   = 'value 1',
 *            2   = 'value 2',
 *            3   = 'value 3',
 *            4   = 'value 4',
 *            "a" = 1,
 *            "b" = 2,
 *            "c" = 3,
 *            "d" = 4
 *            This design is not working because when using pairs, we can't
 *            easily get the index of a key to assure proper order.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>

#include "t.h"
#include "t_oht.h"

/**--------------------------------------------------------------------------
 * Delete an element from the table.
 * \param   L          Lua state.
 * \lparam  table      table.
 * \lparam  value      key/index.
 * \lparam  nil        nil.
 * --------------------------------------------------------------------------*/
static void
t_oht_deleteElement( lua_State *L )
{
	int         i = 0;
	size_t      len = lua_rawlen( L, -3 );

	// TODO: check that key actually exist and/or i is not outOfBound

	if (LUA_TNUMBER == lua_type( L, -2 ) )
	{
		// delete oht.key  -- reuse nil value for lua_next() start
		while (lua_next( L, -3 ))          //S:  … tbl idx key idx
		{
			if (lua_rawequal( L, -1, -3 ))
			{
				lua_pop( L, 1 );             //S:  … tbl idx key
				lua_pushnil( L );            //S:  … tbl idx key nil
				lua_rawset( L, -4 );         //S:  … tbl idx
				break;
			}
			else
				lua_pop( L, 1 );             //S:  … tbl idx key
		}
	}
	else
	{
		// delete oht.key
		lua_pushvalue( L, -2 );             //S: … tbl key nil key
		lua_rawget( L, -4 );                //S: … tbl key nil idx
		lua_insert( L, -3 );                //S: … tbl idx key nil
		lua_rawset( L, -4 );                //S: … tbl idx
	}

	// move up values > idx
	for (i=luaL_checkinteger( L, -1 ); i < (int) len; i++)
	{
		lua_rawgeti( L, -1, i + 1 );    // S: … tbl idx idxK
		lua_rawseti( L, -2, i );        // S: … tbl idx
	}
	lua_pop( L, 1 );                   // S: … tbl
	lua_pushnil( L );                  // S: … tbl key nil
	lua_rawseti( L, -2, len );
}


/**--------------------------------------------------------------------------
 * Insert an element into the table.
 * \param   L          Lua state.
 * \lparam  table      table.
 * \lparam  integer    index.
 * \lparam  value      key.
 * \lparam  value      value. CANNOT be nil.
 * --------------------------------------------------------------------------*/
static void
t_oht_insertElement( lua_State *L )
{
	size_t i     = luaL_checkinteger( L, -3 );
	size_t k     = 1;

	luaL_argcheck( L, 1 <= i && i <= lua_rawlen( L, -4 ), -3, "position out of bounds" );

	for (k=lua_rawlen( L, -4 ); k>=i; k--)    // S: … tbl idx key val
	{
		lua_rawgeti( L, -4, k );               // S: … tbl idx key val valK
		lua_rawseti( L, -5, k + 1 );           // S: … tbl idx key val
	}
	lua_rawseti( L, -4, i );                  // S: … tbl idx key
	lua_insert( L, -2 );                      // S: … tbl key idx
	lua_rawset( L, -3 );                      // S: … tbl
}


/**--------------------------------------------------------------------------
 * Get key in table for given index.
 * \param   L          Lua state.
 * \lparam  table      table.
 * \lparam  int        index.
 * \return  size_t     1-based index for *key* is in the table.
 * --------------------------------------------------------------------------*/
static void
t_oht_getKey( lua_State *L )
{
	int  equal = 0;
	lua_pushnil( L );                  //S:  … tbl idx nil
	while (lua_next( L, -3 ))          //S:  … tbl idx key idx
	{
		equal = (lua_rawequal( L, -1, -3 )) ? 1 : 0;
		lua_pop( L, 1 );                //S:  … tbl idx key
		if (equal) return;         // return with key on stack
	}
	lua_pushnil( L );             // return with nil on stack
	return;
}


/**--------------------------------------------------------------------------
 * Get key in table for given index.
 * \param   L      Lua state.
 * \lparam  ud     T.OrderedHashTable userdata instance.
 * \lparam  int    index.
 * \lreturn value  key or nil if outOfBound.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_GetKey( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    // S: oht idx tbl
	lua_insert( L, -2 );                             // S: oht tbl idx

	t_oht_getKey( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Insert key/value pair at specified index into OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.OrderedHashTable userdata instance.
 * \lparam  integer  index.
 * \lparam  value    key.
 * \lparam  value    value.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_Insert( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );  //S: oht idx key val tbl
	lua_replace( L, 1 );                           //S: tbl idx key val

	lua_rawgeti( L, 1, luaL_checkinteger( L, 2 ) ); //S: tbl idx key val val/nil
	if (lua_isnil( L, -1 ) )
	{
		lua_pop( L, 1 );                            //S: tbl idx key val
		t_oht_insertElement( L );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Create a new T.OrderedHashTable and return it.
 * \param   L        Lua state.
 * \lparam  CLASS    table OrderedHashTable.
 * \lreturn ud       T.OrderedHashTable userdata instance.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__Call( lua_State *L )
{
	struct t_oht __attribute__ ((unused)) *oht = t_oht_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new t_oht userdata and push to LuaStack.
 * \param   L        Lua state.
 * \return  struct   t_oht * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_oht
*t_oht_create_ud( lua_State *L )
{
	struct t_oht    *oht;

	oht = (struct t_oht *) lua_newuserdata( L, sizeof( struct t_oht ) );
	// create and set reference in userdata
	lua_newtable( L );
	oht->tR = luaL_ref( L, LUA_REGISTRYINDEX );

	luaL_getmetatable( L, T_OHT_TYPE );
	lua_setmetatable( L, -2 );
	return oht;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_oht.
 * \param   L        Lua state.
 * \param   int      Position on the stack.
 * \param   int      check(boolean): if true error out on fail.
 * \return  struct   t_oht*  pointer to userdata on stack.
 * --------------------------------------------------------------------------*/
struct t_oht
*t_oht_check_ud ( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_OHT_TYPE );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_OHT_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_oht *) ud;
}


/**--------------------------------------------------------------------------
 * Read element from a T.OrderedHashTable.
 * \param   L        Lua state.
 * \lreturn ud       T.OrderedHashTable userdata instance.
 * \lparam  key/idx  Hash key or index.
 * \lreturn value    value from index/hash.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__index( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );       // S: oht key/idx tbl
	lua_replace( L, -3 );                               // S: tbl key/idx

	if (LUA_TNUMBER != lua_type( L, -1 ) )
		lua_rawget( L, -2 );                             // S: tbl idx

	lua_rawgeti( L, -2, luaL_checkinteger( L, -1 ) );   // S: tbl idx val
	return 1;
}


/**--------------------------------------------------------------------------
 * Set an Element on a T.OrderedHashTable.
 * \param   L        Lua state.
 * \lreturn ud       T.OrderedHashTable userdata instance.
 * \lparam  key      string/integer.
 * \lparam  value    value for index/hash.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__newindex( lua_State *L )
{
	int           idx;
	size_t        len;
	struct t_oht *oht = t_oht_check_ud( L, -3, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht key/idx val tbl
	lua_replace( L, -4 );                         // S: tbl key/idx val
	len = lua_rawlen( L, -3 );

	if (lua_isnil( L, -1 ))
	{
		t_oht_deleteElement( L );
		return 0;
	}

	// Numeric indices can only be used to replace values on an T.OrderedHashTable
	if (LUA_TNUMBER == lua_type( L, -2 ))
	{
		idx = luaL_checkinteger( L, -2 );
		luaL_argcheck( L, 1 <= idx && idx <= (int) len, -2,
			"Index must be greater than 1 and lesser than array length" );
		lua_rawseti( L, -3, idx );           //S: tbl idx val
	}
	else
	{
		lua_pushvalue( L, -2 );              //S: tbl key val key
		lua_rawget( L, -4 );                 //S: tbl key val valold?
		if (lua_isnil( L, -1 ))      // add a new value to the table
		{
			lua_pop( L, 1 );          // pop nil
			lua_insert( L, -2 );              //S: tbl val key
			lua_pushinteger( L, len+1 );      //S: tbl val key len
			lua_rawset( L, -4 );              //S: tbl val
			lua_rawseti( L, -2, len+1 );
		}
		else                         // replace a value in the table
		{
			idx = luaL_checkinteger( L, -2 );
			lua_pop( L, 1 );                  //S: tbl key val
			lua_rawseti( L, -3, idx );
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * the actual iterate( next ) over the T.OrderedHashTable.
 * It will return id,value,key triplets in insertion order.  The current index
 * is passed via an upvalue.
 * \param   L        Lua state.
 * \lparam  table    Table to iterate.
 * \lparam  value    previous key.
 * \lreturn multiple current key, current value, current idx.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_iter( lua_State *L )
{
	int i = lua_tointeger( L, lua_upvalueindex( 1 ) ) + 1;
	luaL_checktype( L, -2, LUA_TTABLE );
	if (lua_rawgeti( L, -2, i ) == LUA_TNIL)
		return 1;
	else                         // S: tbl idx key
	{
		lua_replace( L, -2 );     // S: tbl key
		lua_pushvalue( L, -1 );   // S: tbl key key
		lua_rawget( L, -3 );      // S: tbl key val
		lua_pushinteger( L, i );  // S: tbl key val idx
		lua_pushinteger( L, i );  // S: tbl key val idx idx
		lua_replace( L, lua_upvalueindex( 1 ) );
		return 3;
	}
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.OrderedHashTable userdata instance.
 * \lreturn multiple iter function, table, first key
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_oht__pairs( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	lua_pushinteger( L, 0 );                       // S: oht 0
	lua_pushcclosure( L, &t_oht_iter, 1 );         // S: oht fnc
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );  // S: oht fnc tbl
	lua_rawgeti( L, -1, 1 );                       // S: oht fnc tbl key
	return 3;
}


/**--------------------------------------------------------------------------
 * the actual iterate( next ) over the T.OrderedHashTable.
 * It will return id,value,key triplets in insertion order.
 * \param   L        Lua state.
 * \lparam  table    Table to iterate.
 * \lparam  int      previous idx.
 * \lreturn multiple current idx, current value, current key.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_iiter( lua_State *L )
{
	lua_Integer i = luaL_checkinteger( L, -1 ) + 1;
	luaL_checktype( L, -2, LUA_TTABLE );
	if (lua_rawgeti( L, -2, i ) == LUA_TNIL)
		return 1;
	else                         // S: tbl idx key
	{
		lua_pushinteger( L, i );  // S: tbl idx key idx
		lua_insert( L, -2 );      // S: tbl idx idx key
		lua_rawget( L, -4 );      // S: tbl idx idx val
		lua_rawgeti( L, -4, i );  // S: tbl idx idx val key
		return 3;
	}
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.OrderedHashTable userdata instance.
 * \lreturn multiple iter function, table, first key
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_oht__ipairs( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	lua_pushcfunction( L, &t_oht_iiter );           // S: oht fnc
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );   // S: oht fnc tbl
	lua_pushinteger( L, 0 );                        // S: oht fnc tbl 0
	return 3;
}



/**--------------------------------------------------------------------------
 * Returns len of the ordered hash table.
 * \param   L    Lua state.
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
 * Return Tostring representation of a ordered table.
 * \param   L       Lua state.
 * \lreturn string  formatted string representing ordered table.
 * \return  int     # of values pushed onto the stack.
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
	  { "getKey"       , lt_oht_GetKey }
	//, { "getIndex"     , lt_oht_GetIndex }
	, { "insert"       , lt_oht_Insert }
	//, { "concat"       , lt_oht_Concat }
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
	, { NULL           , NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the T.OrderedHashTable library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      Lua state.
 * \lreturn table  the library.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_oht( lua_State *L )
{
	// T.OrderedHashTable instance metatable
	luaL_newmetatable( L, T_OHT_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_oht_m, 0 );
	lua_pop( L, 1 );                      // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.OrderedHashTable.<member>
	luaL_newlib( L, t_oht_cf );
	luaL_newlib( L, t_oht_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

