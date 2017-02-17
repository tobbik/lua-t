/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_set.c
 * \brief     Logic for a Set
 *            Implemented as a table which uses the elements as keys and their
 *            values are fixed as true.  The numeric part of the table is
 *            populate with boolean true as well to keep track of the length of
 *            the set.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>

#include "t.h"
#include "t_set.h"

/**--------------------------------------------------------------------------
 * Get the Length of a T.Set inner table.
 * \param   L            Lua state.
 * \param   pos          int; position on stack.
 * \return  int          Length of table.
 * --------------------------------------------------------------------------*/
static size_t
t_set_getLength( lua_State *L, int pos )
{
	size_t   n = 0;
	lua_pushnil( L );
	while (lua_next( L, pos ))
	{
		n++;
		lua_pop( L, 1 );
	}
	return n;
}


/**--------------------------------------------------------------------------
 * Add/Remove element to a T.Set.
 * Expects on the stack Table, Element, Value.  If value is nil, the element
 * will be removed from the table.
 * \param   L            Lua state.
 * \param   pos          int position of table on stack.
 * \lparam  table        Table.
 * \lparam  value        element for set to add.
 * \lparam  value        value if nil, delete element from table.
 * \return  int          0 if existed / 1 if added / -1 if deleted.
 * --------------------------------------------------------------------------*/
static int
t_set_setElement( lua_State *L, int pos )
{
	int existed = 0;
	int remove  = (lua_isnil( L, -1 )) ? 1 : 0;

	pos         = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;  // get absolute stack position
	luaL_checktype( L, pos, LUA_TTABLE );

	lua_pushvalue( L, -2 );                  //S: tbl … elm val/nil elm
	lua_rawget( L, pos );                    //S: tbl … elm val/nil true/nil?
	existed  = (lua_isnil( L, -1 )) ? 0 : 1;
	lua_pop( L, 2 );                         // pop new val and existing val

	if (! existed && ! remove)
	{
		lua_pushboolean( L, 1 );              //S: tbl … elm true
		lua_rawset( L, pos );
		return 1;
	}
	if (existed && remove)
	{
		lua_pushnil( L );                     //S: tbl … elm nil
		lua_rawset( L, pos );
		return -1;
	}
	lua_pop( L, 1 );
	return 0;
}


/** ---------------------------------------------------------------------------
 * Checks if sA is disjunt or a subset of sB.
 * \param  L            Lua state.
 * \param  int(bool)    isDisjunct( 1 ) or isSubset( 0 ).
 * \lparam table        T.Set A.
 * \lparam table        T.Set B.
 * \return int(bool)    1 or 0.
 *--------------------------------------------------------------------------- */
static int
t_set_contains( lua_State *L, int disjunct )
{
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
 * \lparam  table   T.Set A table.
 * \lparam  table   T.Set B table.
 * \lreturn table   T.Set Result table.
 * \return  size_t  # of elements in the union.
 * --------------------------------------------------------------------------*/
static void
t_set_union( lua_State *L )
{
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );
	lua_newtable( L );

	// clone sA > sR
	lua_pushnil( L );                 //S: … sA sB sR nil
	while (lua_next( L, -4 ))         //S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );        //S: … sA sB sR elm true elm
		lua_insert( L, -3 );           //S: … sA sB sR elm elm true
		t_set_setElement( L, -4 );     //S: … sA sB sR elm
	}
	// add elements form sB missing in sA
	lua_pushnil( L );                 //S: … sA sB sR nil
	while (lua_next( L, -3 ))         //S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );        //S: … sA sB sR elm true elm
		lua_insert( L, -3 );           //S: … sA sB sR elm elm true
		t_set_setElement( L, -4 );     //S: … sA sB sR elm
	}
}


/**--------------------------------------------------------------------------
 * Create the Intersection of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set A table.
 * \lparam  table   T.Set B table.
 * \lreturn table   T.Set Result table.
 * \return  size_t  # of elements in the intersection.
 * --------------------------------------------------------------------------*/
static void
t_set_intersection( lua_State *L )
{
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );
	lua_newtable( L );

	// iterate over sA->tR
	lua_pushnil( L );            //S: … sA sB sR nil
	while (lua_next( L, -4 ))    //S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   //S: … sA sB sR elm true elm
		lua_rawget( L, -5 );      //S: … sA sB sR elm true true/nil
		if (! lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );       //S: … sA sB sR elm true
			lua_pushvalue( L, -2); //S: … sA sB sR elm true elm
			lua_insert( L, -2 );   //S: … sA sB sR elm elm true
			t_set_setElement( L, -4 );
		}
		else
			lua_pop( L, 2 );
	}
}


/**--------------------------------------------------------------------------
 * Create the Complement of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set A table.
 * \lparam  table   T.Set B table.
 * \lreturn table   T.Set Result table.
 * \return  size_t  # of elements in the complement.
 * --------------------------------------------------------------------------*/
static void
t_set_complement( lua_State *L )
{
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );
	lua_newtable( L );

	// iterate over sA
	lua_pushnil( L );            //S: … sA sB sR nil
	while (lua_next( L, -4 ))    //S: … sA sB sR elm true
	{
		lua_pushvalue( L, -2 );   //S: … sA sB sR elm true elm
		lua_rawget( L, -5 );      //S: … sA sB sR elm true true/nil
		if (lua_isnil( L, -1 ))   // if not exist in sB don't add to sC
		{
			lua_pop( L, 1 );
			lua_pushvalue( L, -2); //S: … sA sB sR elm true elm
			lua_insert( L, -2 );   //S: … sA sB sR elm elm true
			t_set_setElement( L, -4 );
		}
		else
			lua_pop( L, 2 );
	}
}


/**--------------------------------------------------------------------------
 * Create the Symetric Difference of two set tables.
 * \param   L       Lua state.
 * \lparam  table   T.Set A table.
 * \lparam  table   T.Set B table.
 * \lreturn table   T.Set Result table.
 * \return  size_t  # of elements in the Symetric Difference.
 * --------------------------------------------------------------------------*/
static void
t_set_symdifference( lua_State *L )
{
	int p      = -4;
	luaL_checktype( L, -2, LUA_TTABLE );
	luaL_checktype( L, -1, LUA_TTABLE );
	lua_newtable( L );

	// first iterate sA->tR: add if not in sB->tR
	// then  iterate sB->tR: add if not in sA->tR
	while ( p < -2)
	{
		//printf( "%d -> %d\n", p, -5-(4+p) );
		lua_pushnil( L );             //S: … sA sB sR nil
		while (lua_next( L, p ))      //S: … sA sB sR elm true
		{
			lua_pushvalue( L, -2 );    //S: … sA sB sR elm true elm
			lua_rawget( L, -5-(4+p) ); //S: … sA sB sR elm true true/nil
			if (lua_isnil( L, -1 ))
			{
				lua_pop( L, 1 );        //S: … sA sB sR elm true
				lua_pushvalue( L, -2);  //S: … sA sB sR elm true elm
				lua_insert( L, -2 );    //S: … sA sB sR elm elm true
				t_set_setElement( L, -4 );
			}
			else
				lua_pop( L, 2 );
		}
		p++;
	}
}


/**--------------------------------------------------------------------------
 *  Return the underlying table from a T.Set as an indexed table.
 * \param   L      Lua state.
 * \lparam  ud     T.Set userdata instance.
 * \lreturn table  Numerically indexed table with all elements from the set in
 *                 arbitrar order.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set_GetTable( lua_State *L )
{
	t_set_check( L, 1, 1 );
	size_t runner     = 1;

	lua_newtable( L );
	lua_pushnil( L );              //S: tbl tbl nil
	while (lua_next( L, -3 ))      //S: tbl tbl elm tru
	{
		lua_pop( L, 1 );            //S: tbl tbl elm
		lua_pushvalue( L, -1 );     //S: tbl tbl elm elm
		lua_rawseti( L, -3, runner++ );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 *  Return string that formats members.
 * \param   L      Lua state.
 * \lparam  ud     T.Set userdata instance.
 * \lreturn table  Numerically indexed table with all elements from the set in
 *                 arbitrar order.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set_ToString( lua_State *L )
{
	int cnt = 0;
	t_set_check( L, 1, 1 );

	lua_pushstring( L, "{ " );
	lua_pushnil( L );              //S: tbl "{" nil
	while (lua_next( L, 1 ))       //S: tbl "{" elm tru
	{
		cnt++;
		lua_pop( L, 1 );            //S: tbl "{" elm
		t_fmtStackItem( L, -1, 1 ); //S: tbl "{" elm "e"
		lua_insert( L, -2 );
		lua_pushstring( L, ", " );
		lua_insert( L, -2 );        //S: tbl "{" "e" "," elm
	}
	if (cnt > 1 )
		lua_pop( L, 1 );            // pop trailing comma
	lua_pushstring( L, " }" );
	lua_concat( L, lua_gettop( L )-1 );// all elements - T.Set instance
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
	int doClone = t_set_check( L, 2, 0 );
	lua_remove( L, 1 );               // Remove T.Set Class table

	if (lua_istable( L, 1 ))
	{
		lua_newtable( L );
		lua_pushnil( L );              //S: tbl tbl nil
		while (lua_next( L, 1 ))       //S: tbl tbl elm tru
		{
			lua_pushvalue( L, -2 );     //S: tbl tbl elm tru elm
			if (doClone)
				lua_insert( L, -2 );     //S: tbl set elm elm tru
			t_set_setElement( L, 2 );
		}
	}
	else
		lua_newtable( L );

	t_set_create( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Create a new t_set table and push to Lua Stack.
 * \param   L     Lua state.
 * \lparam  table table of set structure.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
t_set_create( lua_State *L )
{
	lua_newtable( L );                   //S: … tbl set
	lua_insert( L, -2 );                 //S: … set tbl
	t_getProxyTableIndex( L );           //S: … set tbl {}
	lua_insert( L, -2 );                 //S: … set {} tbl
	lua_rawset( L, -3 );                 //S: … set

	luaL_getmetatable( L, T_SET_TYPE );
	lua_setmetatable( L, -2 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct t_set.
 * \param   L       Lua state.
 * \param   int     Position on the stack.
 * \param   int     check(boolean): if true error out on fail.
 * \return  struct  t_set* pointer to userdata on stack.
 * --------------------------------------------------------------------------*/
int
t_set_check( lua_State *L, int pos, int check )
{
	int isSet = t_checkTableType( L, pos, check, T_SET_TYPE );
	if (isSet)
		t_getProxyTable( L, pos );
	return isSet;
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
	t_set_check( L, 1, 1 );

	lua_rawget( L, 1 );           // S: tbl true/nil
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
	t_set_check( L, 1, 1 );
	t_set_setElement( L, 1 );
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
	t_set_check( L, 1, 1 );

	lua_pushcfunction( L, &t_set_iter );
	lua_insert( L, 1 );
	lua_pushnil( L );         //S: fnc tbl nil
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );

	t_set_union( L );
	t_set_create( L );
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );
	t_set_intersection( L );
	t_set_create( L );
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );
	t_set_complement( L );
	t_set_create( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Creates the Symetric Difference of two sets.
 * \param   L    Lua state.
 * \lparam  ud   T.Set userdata instance sA.
 * \lparam  ud   T.Set userdata instance sB.
 * \lreturn ud   T.Set userdata instance -> symetric difference of Set A and Set B.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__bxor( lua_State *L )
{
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );
	t_set_symdifference( L );
	t_set_create( L );
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );

	lua_pushboolean( L,
		t_set_getLength( L, 1 ) ==  t_set_getLength( L, 2 ) && t_set_contains( L, 0 )
	);
	return 1;
}


/**--------------------------------------------------------------------------
 * Tests if two T.Set are disjoint.  Re/abusing modulo operator.
 * \param   L       Lua state.
 * \lparam  ud      T.Set userdata instance sA.
 * \lparam  ud      T.Set userdata instance sB.
 * \lreturn boolean 1 if disjoint, else 0.
 * \return  int     # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_set__mod( lua_State *L )
{
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );

	lua_pushboolean( L, t_set_contains( L, 1 ) );
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );

	lua_pushboolean( L, t_set_contains( L, 0 ) );
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
	t_set_check( L, 1, 1 );
	t_set_check( L, 2, 1 );

	lua_pushboolean( L,
		t_set_getLength( L, 1 ) <  t_set_getLength( L, 2 ) && t_set_contains( L, 0 )
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
	t_set_check( L, 1, 1 );
	lua_pushinteger( L, t_set_getLength( L, 1 ) );
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
	const void *p = lua_topointer( L, 1 );
	t_set_check( L, 1, 1 );
	lua_pushfstring( L, T_SET_TYPE"[%d]: %p", t_set_getLength( L, 1 ), p );
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
	, { "toString"     , lt_set_ToString }
	, { NULL           , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_set_m [] = {
	  { "__tostring"   , lt_set__tostring }
	, { "__len"        , lt_set__len }
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

