/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_set.c
 * \brief     Logic for a Set
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>

#include "t.h"
#include "t_set.h"

/**--------------------------------------------------------------------------
 * Add/Remove element to a T.Set.
 * Expects on the stack Table, Element, Value.  If value is nil, the element
 * will be removed from the table.
 * \param   L            Lua state.
 * \param   struct t_set set pointer.
 * \lparam  table        Table.
 * \lparam  value        element for set to add.
 * \lparam  value        value if nil, delete element from table.
 * \return  int/bool     0 if element existed / 1 if element was added.
 * --------------------------------------------------------------------------*/
static int
t_set_setElement( lua_State *L, struct t_set *set )
{
	int existed      = 0;
	int remove       = (lua_isnil( L, -1 )) ? 1 : 0;
	luaL_checktype( L, -3, LUA_TTABLE );

	lua_pushvalue( L, -2 );                  // S: tbl elm val/nil elm
	lua_rawget( L, -4 );                     // S: tbl elm val/nil true/nil?
	existed  = (lua_isnil( L, -1 )) ? 0 : 1;
	lua_pop( L, 2 );                         // pop new val and existing val

	if (!existed && !remove)
	{
		(set->len)++;
		lua_pushboolean( L, 1 );              // S: tbl elm true
		lua_rawset( L, -3 );
	}
	if ( existed &&  remove)
	{
		(set->len)--;
		lua_pushnil( L );                     // S: tbl elm nil
		lua_rawset( L, -3 );
	}
	return existed;
}


/** ---------------------------------------------------------------------------
 * Checks if sA is disjunt or a subset of sB.
 * \param  L            Lua state.
 * \param  int(bool)    isDisjunct( 1 ) or isSubset( 0 ).
 * \param  struct t_set sA.
 * \param  struct t_set sB.
 * \return int(bool)    1 or 0.
 *--------------------------------------------------------------------------- */
static int
t_set_contains( lua_State *L, int disjunct, struct t_set *sA, struct t_set *sB )
{
	lua_rawgeti( L, LUA_REGISTRYINDEX, sA->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sB->tR );

	lua_pushnil( L );           //S: … tblA tblB nil
	while (lua_next( L, -3 ))   //S: … tblA tblB keyA valA
	{
		lua_pushvalue( L, -2 );  //S: … tblA tblB keyA valA keyA
		lua_gettable( L, -4 );   //S: … tblA tblB keyA valA valB
		// treated as exclusive or:
		// if (0 != 1) -> execute when element does not exist in sB
		// if (1 != 0) -> execute when element exist in sB
		if (disjunct != lua_isnil( L, -1 ))
		{
			lua_pop( L, 5 );      //S: …
			return 0;
		}
		// pop valA and valB
		lua_pop( L, 2 );         //S: … tblA tblB keyA
	}
	lua_pop( L, 2 );            //S: …
	return 1;
}


/**--------------------------------------------------------------------------
 * Create the Union of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set struct t_set A table.
 * \lparam  table   T.Set struct t_set B table.
 * \lparam  table   T.Set struct t_set Result table.
 * \return  size_t  # of elements in the union.
 * --------------------------------------------------------------------------*/
static size_t
t_set_union( lua_State *L )
{
	size_t cnt = 0;
	luaL_checktype( L, -3, LUA_TTABLE );
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );

	// clone sA->tR
	lua_pushnil( L );            // S: … sA sB sR nil
	while (lua_next( L, -4 ))    // S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   // S: … sA sB sR elm true elm
		lua_insert( L, -2 );      // S: … sA sB sR elm elm true
		lua_settable( L, -4 );    // S: … sA sB sR elm
		cnt++;
	}
	// add sB->tR elements missing in sA->tR
	lua_pushnil( L );            // S: … sA sB sR nil
	while (lua_next( L, -3 ))    // S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   // S: … sA sB sR elm true elm
		lua_rawget( L, -6 );      // S: … sA sB sR elm true true/nil
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );       // pop nil
			lua_pushvalue( L, -2 );// S: … sA sB sR elm true elm
			lua_insert( L, -2 );   // S: … sA sB sR elm elm true
			lua_settable( L, -4 ); // S: … sA sB sR elm
			cnt++;
		}
		else
			lua_pop( L, 2 );
	}
	return cnt;
}


/**--------------------------------------------------------------------------
 * Create the Intersection of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set struct t_set A table.
 * \lparam  table   T.Set struct t_set B table.
 * \lparam  table   T.Set struct t_set Result table.
 * \return  size_t  # of elements in the intersection.
 * --------------------------------------------------------------------------*/
static size_t
t_set_intersection( lua_State *L )
{
	size_t cnt = 0;
	luaL_checktype( L, -3, LUA_TTABLE );
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );

	// iterate over sA->tR
	lua_pushnil( L );            // S: … sA sB sR nil
	while (lua_next( L, -4 ))    // S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   // S: … sA sB sR elm true elm
		lua_rawget( L, -5 );      // S: … sA sB sR elm true true/nil
		if (! lua_isnil( L, -1 ))
		{
			lua_pushvalue( L, -3); // S: … sA sB sR elm true elm
			lua_insert( L, -2 );   // S: … sA sB sR elm elm true
			lua_settable( L, -5 ); // S: … sA sB sR elm
			lua_pop( L, 1 );
			cnt++;
		}
		else
			lua_pop( L, 2 );
	}
	return cnt;
}


/**--------------------------------------------------------------------------
 * Create the Complement of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set struct t_set A table.
 * \lparam  table   T.Set struct t_set B table.
 * \lparam  table   T.Set struct t_set Result table.
 * \return  size_t  # of elements in the complement.
 * --------------------------------------------------------------------------*/
static size_t
t_set_complement( lua_State *L )
{
	size_t cnt = 0;
	luaL_checktype( L, -3, LUA_TTABLE );
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );

	// iterate over sA->tR
	lua_pushnil( L );            // S: … sA sB sR nil
	while (lua_next( L, -4 ))    // S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   // S: … sA sB sR elm true elm
		lua_rawget( L, -5 );      // S: … sA sB sR elm true true/nil
		if (lua_isnil( L, -1 ))   // if not exist in sB don't add to sC
		{
			lua_pop( L, 1 );       // pop the nil
			lua_pushvalue( L, -2); // S: … sA sB sR elm true elm
			lua_insert( L, -2 );   // S: … sA sB sR elm elm true
			lua_settable( L, -4 ); // S: … sA sB sR elm
			cnt++;
		}
		else
			lua_pop( L, 2 );
	}
	return cnt;
}


/**--------------------------------------------------------------------------
 * Create the Symetric Difference of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set struct t_set A table.
 * \lparam  table   T.Set struct t_set B table.
 * \lparam  table   T.Set struct t_set Result table.
 * \return  size_t  # of elements in the Symetric Difference.
 * --------------------------------------------------------------------------*/
static size_t
t_set_symdifference( lua_State *L )
{
	size_t cnt = 0;
	int p      = -4;
	luaL_checktype( L, -3, LUA_TTABLE );
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );

	// first iterate sA->tR: add if not in sB->tR
	// then  iterate sB->tR: add if not in sA->tR
	while ( p < -2)
	{
		//printf( "%d -> %d\n", p, -5-(4+p) );
		lua_pushnil( L );             // S: … sA sB sR nil
		while (lua_next( L, p ))      // S: … sA sB sR elm true
		{
			lua_pushvalue( L, -2 );    // S: … sA sB sR elm true elm
			lua_rawget( L, -5-(4+p) ); // S: … sA sB sR elm true true/nil
			if (lua_isnil( L, -1 ))
			{
				lua_pop( L, 1 );
				lua_pushvalue( L, -2);  // S: … sA sB sR elm true true elm
				lua_insert( L, -2 );    // S: … sA sB sR elm elm true
				lua_settable( L, -4 );  // S: … sA sB sR elm
				cnt++;
			}
			else
				lua_pop( L, 2 );
		}
		p++;
	}
	return cnt;
}


/**--------------------------------------------------------------------------
 * Get the underlying table from a T.Set userdata.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set_GetTable( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, -1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR ); // S: set tbl
	return 1;
}


/**--------------------------------------------------------------------------
 * Serialize the underlying table from a T.Set userdata.
 * \param   L      Lua state.
 * \lparam  ud     T.Set userdata instance.
 * \lreturn table  Numerically indexed table with all elements from the set in
 *                 arbitrar order.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set_ToTable( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, -1, 1 );
	size_t runner     = 1;

	lua_createtable( L, set->len, 0 );
	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR );
	lua_pushnil( L );              // S: set tbl tbl nil
	while (lua_next( L, -2 ))
	{
		lua_pushvalue( L, -2 );     // S: set tbl tbl nil elm tru elm
		lua_rawseti( L, -5, runner++ );
		lua_pop( L, 1 );            // S: set tbl tbl elm
	}
	lua_pop( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a T.Set and return it.
 * \param   L     Lua state.
 * \lparam  CLASS table T.Set.
 * \lreturn ud    T.Set userdata instance.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_set__Call( lua_State *L )
{
	struct t_set                          *org_set = t_set_check_ud( L, -1, 0 );
	struct t_set __attribute__ ((unused)) *set;

	if (NULL != org_set)
	{
		lua_rawgeti( L, LUA_REGISTRYINDEX, org_set->tR ); // S: set tbl
		set = t_set_create_ud( L, -1, -1 );
		lua_remove( L, -2 );
	}
	else if (lua_istable( L, -1 ))
		set = t_set_create_ud( L, -1, 1 );
	else
		set = t_set_create_ud( L, 0, 0 );

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new t_set userdata and push to Lua Stack.
 * \param   L        Lua state.
 * \param   int pos  create stack from table at position to create set.
 * \param   int mode >0 -> use table values, <0 use table keys.
 * \return  struct   t_set * pointer to new userdata on Lua Stack.
 * --------------------------------------------------------------------------*/
struct t_set
*t_set_create_ud( lua_State *L, int pos, int mode )
{
	struct t_set    *set;
	int              cnt     = 0;

	if (0 != pos) luaL_checktype( L, pos, LUA_TTABLE );
	pos = (pos<0) ? pos-3 : pos;

	set = (struct t_set *) lua_newuserdata( L, sizeof( struct t_set ) );
	// create table
	lua_newtable( L );                // S: tbl? set tbl
	if (mode)                         // populate if desired
	{
		lua_pushnil( L );              // S: tbl set tbl nil
		while (lua_next( L, pos ))
		{
			if (mode < 0)
			{
				lua_pushvalue( L, -2 );
				lua_insert( L, -2 );     // S: tbl set tbl key key tru
			}
			else
				lua_pushboolean( L, 1 ); // S: tbl set tbl key val tru
			lua_rawset( L, -4 );
			cnt++;
		}
	}
	set->tR  = luaL_ref( L, LUA_REGISTRYINDEX );
	set->len = cnt;

	luaL_getmetatable( L, T_SET_TYPE );
	lua_setmetatable( L, -2 );
	return set;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_set.
 * \param   L       Lua state.
 * \param   int     Position on the stack.
 * \param   int     check(boolean): if true error out on fail.
 * \return  struct  t_set* pointer to userdata on stack.
 * --------------------------------------------------------------------------*/
struct t_set
*t_set_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_SET_TYPE );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_SET_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct t_set *) ud;
}


/**--------------------------------------------------------------------------
 * Read element from Set value.
 * Instance[value] will return true if the set contains the value.  Otherwise
 * returns nil( which is falsy in Lua ).
 * \param   L        Lua state.
 * \lparam  ud       T.Set userdata instance.
 * \lparam  key      Lua value.
 * \lreturn boolean  true if exists else false.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__index( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, -2, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR );
	lua_replace( L, -3 );          // S: tbl,key
	lua_rawget( L, -2 );           // S: tbl,val/key?
	if (lua_isnil( L, -1 ))
		lua_pushboolean( L, 0 );
	else
		lua_pushboolean( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set/Delete element in T.Set value.
 * Using instance[value] = nil deletes an element.  Instance[value] = true adds
 * the value to the set.
 * \param   L      Lua state.
 * \lparam  ud     T.Set userdata instance.
 * \lparam  key    string/integer.
 * \lparam  value  value for index/hash.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__newindex( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, -3, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR ); // S: set elm val tbl
	lua_replace( L, -4 );                         // S: tbl elm val
	t_set_setElement( L, set );
	return 0;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the T.Set.
 * It will return all values key, value pairs.
 * \param   L        Lua state.
 * \lparam  table    Table to iterate.
 * \lparam  value    previous key.
 * \lreturn multiple key, true.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
t_set_iter( lua_State *L )
{
	luaL_checktype( L, -2, LUA_TTABLE );
	if (lua_next( L, -2 ))
	{
		lua_pushvalue( L, -2 );
		lua_insert( L, -2 );
		return 2;
	}
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the T.OrderedHashTable.
 * \param   L        Lua state.
 * \lparam  ud       T.Set userdata instance.
 * \lreturn multiple function, table, 0.
 * \return  int      # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_set__pairs( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, -1, 1 );

	lua_pushcfunction( L, &t_set_iter );
	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR );
	lua_pushnil( L );         // S: set fnc tbl nil
	lua_remove( L, -4 );      // remove T.Set from stack
	return 3;
}


/**--------------------------------------------------------------------------
 * Creates the Union of two sets.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance sA.
 * \lparam  ud   T.Set userdata instance sB.
 * \lreturn ud   T.Set userdata instance -> union of Set A and Set B.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__bor( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );
	struct t_set *sU = t_set_create_ud( L, 0, 0 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, sA->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sB->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sU->tR );

	sU->len = t_set_union( L );
	lua_pop( L, 3 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Creates the Intersection of two sets.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance sA.
 * \lparam  ud   T.Set userdata instance sB.
 * \lreturn ud   T.Set userdata instance -> intersection of Set A and Set B.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__band( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );
	struct t_set *sI = t_set_create_ud( L, 0, 0 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, sA->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sB->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sI->tR );

	sI->len = t_set_intersection( L );
	lua_pop( L, 3 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Creates the Difference (complement) of two sets.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance sA.
 * \lparam  ud   T.Set userdata instance sB.
 * \lreturn ud   T.Set userdata instance -> difference of Set A and Set B.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__sub( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );
	struct t_set *sU = t_set_create_ud( L, 0, 0 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, sA->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sB->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sU->tR );

	sU->len = t_set_complement( L );
	lua_pop( L, 3 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Creates the Symmetric Difference of two sets.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance sA.
 * \lparam  ud   T.Set userdata instance sB.
 * \lreturn ud   T.Set userdata instance -> symetric difference of Set A and Set B.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__bxor( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );
	struct t_set *sU = t_set_create_ud( L, 0, 0 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, sA->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sB->tR );
	lua_rawgeti( L, LUA_REGISTRYINDEX, sU->tR );

	sU->len = t_set_symdifference( L );
	lua_pop( L, 3 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Compare two T.Set for equality.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance sA.
 * \lparam  ud      T.Set userdata instance sB.
 * \lreturn boolean True if A and B contain the same elements. Else false.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__eq( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );

	lua_pushboolean( L,
		sA->len == sB->len && t_set_contains( L, 0, sA, sB )
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * Tests if two T.Set are disjoint.  Re- or abusing modulo operator.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance sA.
 * \lparam  ud      T.Set userdata instance sB.
 * \lreturn boolean 1 if disjoint, else 0.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__mod( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, 1, 1 );
	struct t_set *sB = t_set_check_ud( L, 2, 1 );

	lua_pushboolean( L, t_set_contains( L, 1, sA, sB ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Compare two T.Set for being a subset.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance sA.
 * \lparam  ud      T.Set userdata instance sB.
 * \lreturn boolean True if B contains all elements of A. Else false.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__le( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );

	lua_pushboolean( L, t_set_contains( L, 0, sA, sB ) );
	return 1;
}

/**--------------------------------------------------------------------------
 * Compare two T.Set for being a subset and NOT equal.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance sA.
 * \lparam  ud      T.Set userdata instance sB.
 * \lreturn boolean True if B contains all elements of A and #B > #A. Else
 *                  false.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__lt( lua_State *L )
{
	struct t_set *sA = t_set_check_ud( L, -2, 1 );
	struct t_set *sB = t_set_check_ud( L, -1, 1 );

	lua_pushboolean( L,
		sA->len != sB->len && t_set_contains( L, 0, sA, sB )
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * Returns the # of elements in T.Set instance.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__len( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, 1, 1 );

	lua_pushinteger( L, set->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * Return Tostring representation of a T.Set instance.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance.
 * \lreturn string  Formatted string representing Set.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__tostring( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, 1, 1 );

	lua_rawgeti( L, LUA_REGISTRYINDEX, set->tR );
	lua_pushfstring( L, T_SET_TYPE"[%d]: %p", set->len, set );
	lua_remove( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * __gc Garbage Collector. Releases references from Lua Registry.
 * \param  L     Lua state.
 * \lparam ud    T.Set userdata instance.
 * \return int   # of values pushed onto the stack.
 * -------------------------------------------------------------------------*/
static int
lt_set__gc( lua_State *L )
{
	struct t_set *set = t_set_check_ud( L, 1, 1 );

	luaL_unref( L, LUA_REGISTRYINDEX, set->tR );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_set_fm [] = {
	  { "__call"       , lt_set__Call }
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_set_cf [] = {
	  { "getTable"     , lt_set_GetTable }
	, { "toTable"      , lt_set_ToTable }
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_set_m [] = {
	  { "__tostring"   , lt_set__tostring }
	, { "__len"        , lt_set__len }
	, { "__gc"         , lt_set__gc }
	, { "__index"      , lt_set__index }
	, { "__newindex"   , lt_set__newindex }
	, { "__pairs"      , lt_set__pairs }
	, { "__eq"         , lt_set__eq }       // == equality test
	, { "__lt"         , lt_set__lt }       // <= subset or equal test
	, { "__le"         , lt_set__le }       // <  subset and NOT equal test
	, { "__bor"        , lt_set__bor }      // |  union
	, { "__band"       , lt_set__band }     // &  intersection
	, { "__sub"        , lt_set__sub }      // -  complement
	, { "__bxor"       , lt_set__bxor }     // ~  symmetric difference
	, { "__mod"        , lt_set__mod }      // %  test for disjoint sets
	, { NULL           , NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T.Set library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L      Lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_set( lua_State *L )
{
	// T.Set instance metatable
	luaL_newmetatable( L, T_SET_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_set_m, 0 );
	lua_pop( L, 1 );        // remove metatable from stack

	// Push the class onto the stack
	// this is avalable as T.Set.<member>
	luaL_newlib( L, t_set_cf );
	luaL_newlib( L, t_set_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

