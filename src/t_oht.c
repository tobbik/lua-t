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
 17380 Jan 10 15:15 src/t_oht.c
  1004 Jan  3 01:22 src/t_oht.h
 24376 Jan 10 15:15 src/t_oht.o
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>

#include "t.h"
#include "t_oht.h"

/**--------------------------------------------------------------------------
 * Delete an element from the table.
 * \param   L        Lua state.
 * \lparam  table    Table referenced from t_oht ud.
 * \lparam  value    key/index.
 * \lparam  nil      nil.
 * --------------------------------------------------------------------------*/
void
t_oht_deleteElement( lua_State *L )
{
	lua_Integer i;
	lua_Integer l = (lua_Integer) lua_rawlen( L, -3 );
	lua_Integer k;

	// TODO: check that key actually exist and/or i is not outOfBound
	if (LUA_TNUMBER == lua_type( L, -2 ) )
	{
		i = luaL_checkinteger( L, -2 );
		lua_rawgeti( L, -3, i );              //S: … tbl idx nil key
		lua_replace( L, -3 );                 //S: … tbl key nil
	}
	else
	{
		for (i=1; i < l; i++)
		{
			lua_rawgeti( L, -3, i );           //S: … tbl key nil keyK
			k = (lua_rawequal( L, -1, -3 )) ? 1 : 0;
			lua_pop( L, 1 );
			if (k) break;
		}
	}
	lua_rawset( L, -3 );                     //S: … tbl

	for (k=i; k < l; k++)
	{
		lua_rawgeti( L, -1, k + 1 );          //S: … tbl key
		lua_rawseti( L, -2, k );              //S: … tbl
	}
	lua_pushnil( L );                        //S: … tbl key nil
	lua_rawseti( L, -2, l );
}


/**--------------------------------------------------------------------------
 * Add an Element on a T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  table    Table referenced from t_oht ud.
 * \lparam  value    key/index.
 * \lparam  value    value for key/index.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
void
t_oht_addElement( lua_State *L )
{
	lua_Integer  idx;
	luaL_argcheck( L, lua_istable( L, -3 ), -3, "First Argument must be a table" );
	if (lua_isnil( L, -1 ))                          //S:… tbl i/k nil
		t_oht_deleteElement( L );
	else
	{
		// Numeric indices can only be used to replace values on an T.OrderedHashTable
		if (LUA_TNUMBER == lua_type( L, -2 ))
		{
			idx = luaL_checkinteger( L, -2 );
			luaL_argcheck( L, 1 <= idx && idx <= (int) lua_rawlen( L, -3 ), -2,
				"Index must be greater than 1 and lesser than length of "T_OHT_TYPE );
			lua_rawgeti( L, -3, idx );                 //S:… tbl idx val key
			lua_replace( L, -3 );                      //S:… tbl key val
			lua_rawset( L, -3 );
		}
		else
		{
			lua_pushvalue( L, -2 );                    //S:… tbl key val key
			lua_rawget( L, -4 );                       //S:… tbl key val valold?
			if (lua_isnil( L, -1 ))         // add a new value to the table
			{
				lua_pushvalue( L, -3 );                 //S:… tbl key val nil key
				lua_rawseti( L, -5, lua_rawlen( L, -5 )+1 );
			}

			lua_pop( L, 1 );                           //S:… tbl key val
			lua_rawset( L, -3 );
		}
	}

}


/**--------------------------------------------------------------------------
 * Insert an element into the table.
 * \param   L          Lua state.
 * \lparam  table      table.
 * \lparam  integer    index.
 * \lparam  value      key.
 * \lparam  value      value. CANNOT be nil.
 * --------------------------------------------------------------------------*/
void
t_oht_insertElement( lua_State *L )
{
	size_t i = luaL_checkinteger( L, -3 );
	size_t l = lua_rawlen( L, -4 );

	luaL_argcheck( L, 1 <= i && i <= l, -4, "position out of bounds" );

	for (; l>=i; l--)
	{
		lua_rawgeti( L, -4, l );               //S: … tbl idx key val keyK
		lua_rawseti( L, -5, l + 1 );           //S: … tbl idx key val
	}
	lua_pushvalue( L, -2 );                   //S: … tbl idx key val key
	lua_rawseti( L, -5, i );                  //S: … tbl idx key val
	lua_rawset( L, -4 );                      //S: … tbl idx
	lua_pop( L, 1 );                          //S: … tbl idx
}


/**--------------------------------------------------------------------------
 * Get index in table where a key is located.
 * \param   L          Lua state.
 * \lparam  table      table.
 * \lparam  value      key.
 * \lreturn idx/nil    nil or index
 * --------------------------------------------------------------------------*/
static void
t_oht_getIndex( lua_State *L )
{
	size_t i;

	for ( i=1; i < lua_rawlen( L, -2 ) + 1; i++ )
	{
		lua_rawgeti( L, -2, i );  // S: … tbl key keyI
		if (lua_rawequal( L, -1, -2 ))
		{
			lua_pushinteger( L, i );
			return;
		}
		lua_pop( L, 1 );
	}
	lua_pushnil( L );              // key not found
}


/**--------------------------------------------------------------------------
 * Concat all values into a string.
 * \param   L      Lua state.
 * \lparam  ud     T.OrderedHashTable userdata instance.
 * \lparam  string separator.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_Concat( lua_State *L )
{
	struct t_oht *oht  = t_oht_check_ud( L, 1, 1 );
	size_t        lsep;
	const char   *sep = luaL_optlstring( L, 2, "", &lsep );
	luaL_Buffer   b;
	size_t        i;
	size_t        len;

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    //S: oht sep tbl
	lua_replace( L, 1 );                             //S: tbl sep
	len = lua_rawlen( L, 1 );
	luaL_buffinit( L, &b );
	for (i=0; i < len; i++)
	{
		lua_rawgeti( L, 1, i+1 );                     //S: tbl sep key
		lua_rawget( L, 1 );                           //S: tbl sep val
		if (!lua_isstring( L, -1 ) )
			luaL_error( L, "invalid value (%s) at index %d in OrderedHashTable for 'concat'",
			           luaL_typename( L, -1 ), i+1 );
		luaL_addvalue( &b );
		if (i<len-1)
			luaL_addlstring( &b, sep, lsep ) ;
	}
	luaL_pushresult( &b );
	return 1;
}


/**--------------------------------------------------------------------------
 * Get index in table where a key is located.
 * \param   L      Lua state.
 * \lparam  ud     T.OrderedHashTable userdata instance.
 * \lparam  value  key.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_GetIndex( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    //S: oht key tbl
	lua_insert( L, -2 );                             //S: oht tbl key

	t_oht_getIndex( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Get key from table where a index is located.
 * \param   L     Lua state.
 * \lparam  ud    T.OrderedHashTable userdata instance.
 * \lparam  int   index.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_GetKey( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    //S: oht idx tbl

	lua_rawgeti( L, -1, luaL_checkinteger( L, -2 ) );
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
	size_t          i = luaL_checkinteger( L, 2 );
	luaL_argcheck( L, 1 <= i && i <= lua_rawlen( L, 1 ), 2, "position out of bounds");

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    //S: oht idx key val ref
	lua_replace( L, -5 );             //S: ref idx key val

	lua_pushvalue( L, -2 );           //S: ref idx key val key
	lua_rawget( L, -5 );              //S: ref idx key val key val/nil
	if (lua_isnil( L, -1 ) )
	{
		lua_pop( L, 1 );
		t_oht_insertElement( L );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Read all arguments from Stack.
 * \param   L        Lua state.
 * \return  struct   t_oht * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
static void
t_oht_readArguments( lua_State *L, struct t_oht *oht, int sp, int ep )
{
	size_t  i  = 0;         ///< iterator for going through the arguments
	size_t  n  = ep-sp + 1; ///< process how many arguments
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );    //S: sp … ep oht tbl

	// populate Oht table
	while (i < n)
	{
		luaL_argcheck( L, lua_istable( L, sp ), i+1,
			"Arguments must be tables with a single key/value pair" );
		// get key/value from table
		lua_pushnil( L );                //S: sp … ep oht tbl nil
		luaL_argcheck( L, lua_next( L, sp ), ep-n-1,
			"The table argument must contain one key/value pair." );
		lua_remove( L, sp );             // remove the table now key/pck pair is on stack
		t_oht_addElement( L );           //S: sp … ep oht tbl key val
		i++;
	}
	lua_pop( L, 1 );                    //S: oht
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
	struct t_oht *oht = t_oht_create_ud( L );
	lua_remove( L, 1 );          // remove T.OrderedHashTable class
	if (lua_istable( L , 1 ))
		t_oht_readArguments( L, oht, 1, lua_gettop( L )-1 );
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

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );        //S: oht key/idx tbl
	lua_replace( L, -3 );                                //S: tbl key/idx

	if (LUA_TNUMBER == lua_type( L, -1 ) )
	{
		lua_rawgeti( L, -2, luaL_checkinteger( L, -1 ) ); //S: tbl key
		lua_replace( L, -2 );
	}

	lua_rawget( L, -2 );                                 //S: tbl val
	return 1;
}


/**--------------------------------------------------------------------------
 * Set an Element on a T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.OrderedHashTable userdata instance.
 * \lparam  key      string/integer.
 * \lparam  value    value for index/hash.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht__newindex( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); //S: oht key/idx val tbl
	lua_replace( L, 1 );                          //S: tbl key/idx val
	t_oht_addElement( L );

	return 0;
}


/**--------------------------------------------------------------------------
 * Actual iterate( next ) over the T.OrderedHashTable.
 * It will return id,value,key triplets in insertion order.  The current index
 * is passed via an upvalue.
 * \param   L        Lua state.
 * \lparam  table    Table to iterate.
 * \lparam  value    previous key.
 * \lreturn multiple current key, current value, current idx.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_pairs( lua_State *L )
{
	int i = lua_tointeger( L, lua_upvalueindex( 1 ) ) + 1;
	luaL_checktype( L, -2, LUA_TTABLE );

	if (lua_rawgeti( L, -2, i ) == LUA_TNIL)
		return 1;
	else                         // S: tbl oky nky
	{
		lua_pushinteger( L, i );  // S: tbl oky nky idx
		lua_replace( L, lua_upvalueindex( 1 ) );
		lua_replace( L, -2 );     // S: tbl key
		lua_pushvalue( L, -1 );   // S: tbl key key
		lua_rawget( L, -3 );      // S: tbl key val
		lua_pushinteger( L, i );  // S: tbl key val idx
		return 3;
	}
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.OrderedHashTable userdata instance.
 * \lreturn multiple iter function, table, first key.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_oht__pairs( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	lua_pushinteger( L, 0 );                       // S: oht 0
	lua_pushcclosure( L, &t_oht_pairs, 1 );        // S: oht fnc
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );  // S: oht fnc tbl
	lua_rawgeti( L, -1, 1 );                       // S: oht fnc tbl key
	return 3;
}


/**--------------------------------------------------------------------------
 * Actual iterate( next ) over the T.OrderedHashTable.
 * It will return id,value,key triplets in insertion order.
 * \param   L        Lua state.
 * \lparam  table    Table to iterate.
 * \lparam  int      previous idx.
 * \lreturn multiple current idx, current value, current key.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_oht_ipairs( lua_State *L )
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

	lua_pushcfunction( L, &t_oht_ipairs );          // S: oht fnc
	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR );   // S: oht fnc tbl
	lua_pushinteger( L, 0 );                        // S: oht fnc tbl 0
	return 3;
}


/**--------------------------------------------------------------------------
 * Returns len of T.OrderedHashTable.
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
 * Get the underlying table from a T.OrderedHashTable userdata.
 * \param   L      Lua state.
 * \lparam  ud     T.OrderedHashTable userdata instance.
 * \lreturn table  Tbale reference in t_oht struct.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_oht_GetTable( lua_State *L )
{
	struct t_oht *oht = t_oht_check_ud( L, -1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, oht->tR ); // S: oht tbl
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
	  { "getIndex"     , lt_oht_GetIndex }
	, { "getKey"       , lt_oht_GetKey }
	, { "insert"       , lt_oht_Insert }
	, { "concat"       , lt_oht_Concat }
	, { "getTable"     , lt_oht_GetTable }
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

