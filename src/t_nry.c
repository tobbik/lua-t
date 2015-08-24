/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_nry.c
 * \brief     models an Numarray
 * This an example file as nrylained in the docs/Coding.rst.  It is an
 * implmentation of the 
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>               // memset

#include "t.h"
#include "t_nry.h"


/////////////////////////////////////////////////////////////////////////////
//  _                        _    ____ ___
// | |   _   _  __ _        / \  |  _ \_ _|
// | |  | | | |/ _` |_____ / _ \ | |_) | |
// | |__| |_| | (_| |_____/ ___ \|  __/| |
// |_____\__,_|\__,_|    /_/   \_\_|  |___|
/////////////////////////////////////////////////////////////////////////////
/** -------------------------------------------------------------------------
 * creates an Numarray from the function call.
 * The __New() function is always used to sanitize parmeters and negotiate
 * Constructor overload based on different types and number of parameters.
 * Takes optional arguments.
 *    - 1    arguments -> create a sized Numarray
 *    - mult arguments -> create a Numarray with mult numbers and fill in
 *    - mult arguments -> create partial Numarray stopping here
 * \param             L    lua state.
 * \lparam  int       size of Numarray.
 * \lparam  mult int  multiple integer filling array.
 * \return int  # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int lt_nry_New( lua_State *L )
{
	int                                     sz;
	int                                     i;
	struct t_nry  __attribute__ ((unused)) *a;

	if (1 == lua_gettop( L ))
	{
		sz = luaL_checkinteger( L, 1 );
		luaL_argcheck( L, sz>0, 1,
		                 "size of T.Numarray must be positive");
		a  = t_nry_create_ud( L, sz );
	}
	else
	{
		sz = lua_gettop( L );
		a  = t_nry_create_ud( L, sz );
		for (i=0; i<sz; i++)
		{
			luaL_checkinteger( L, i+1 );
			a->v[ i ] = luaL_checkinteger( L, i+1 );
		}
	}
	return 1;
}


/** -------------------------------------------------------------------------
 * creates an Numarray from the function call.
 * The __Call() function is almost always just a caller to __New(). It must
 * remove the class table from the stack.  Anytime __call is used the table it
 * is used on will be passed as the first parameter which is not desired in this
 * context.
 * Takes optional arguments.
 *    - 1    arguments -> create a sized Numarray
 *    - mult arguments -> create a Numarray with mult numbers and fill in
 *    - mult arguments -> create partial Numarray stopping here
 * \param             L    lua state.
 * \lparam  CLASS     table Numarray. This is how the metafunction call works.
 * \lparam  int       size of Numarray.
 * \lparam  mult int  multiple integer filling array.
 * \return int  # of values pushed onto the stack.
 * \lparam  (opt)  char first character.
 * \lparam  (opt)  char last  character.
 * \return int  # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int lt_nry__Call( lua_State *L )
{
	lua_remove( L, 1 );    // remove the T.Numarray Class table
	return lt_nry_New( L );
}


/**--------------------------------------------------------------------------
 * create a t_nry and push to LuaStack.
 * The paramters to the _create_ud function are specific to whatever type of
 * class the function is used for.  Sometimes it is useful to have this function
 * exposed so other lua-t code can create t_nry objects.  This is why it is not
 * static and listed in t_nry.h.
 * \param   L      The lua state.
 * \return  struct t_nry*  pointer to the  t_nry struct.
 * --------------------------------------------------------------------------*/
struct t_nry *t_nry_create_ud( lua_State *L, int sz )
{
	struct t_nry  *a;
	size_t         t_sz;

	// size = sizeof(...) -1 because the array has already one member
	t_sz = sizeof( struct t_nry ) + (sz - 1) * sizeof( int );
	a    = (struct t_nry *) lua_newuserdata( L, t_sz );
	memset( a->v, 0, sz+1 * sizeof( int ) );

	a->len = sz;
	luaL_getmetatable( L, "T.Numarray" );
	lua_setmetatable( L, -2 );
	return a;
}


/**--------------------------------------------------------------------------
 * Check if the item on stack position pos is an t_nry struct and return it
 * \param  L    the Lua State
 * \param  pos      position on the stack
 *
 * \return struct t_nry* pointer to t_nry struct
 * --------------------------------------------------------------------------*/
struct t_nry *t_nry_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, "T.Numarray" );
	luaL_argcheck( L, (ud != NULL || !check), pos, "`T.Numarray` expected" );
	return (NULL==ud) ? NULL : (struct t_nry *) ud;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// ================================= GENERIC LUA API========================
/**--------------------------------------------------------------------------
 * reads an address where an array element sits.
 * \param   L       lua Virtual Machine.
 * \lparam  struct  t_nry.
 * \lreturn value   unpacked value according to packer format.
 * \return  *int    pointer to the requested value.
 *  -------------------------------------------------------------------------*/
static int
*t_nry_getelem( lua_State *L )
{
	t_stackDump( L );
	struct t_nry *a   = t_nry_check_ud( L, 1, 1 );
	size_t        idx = luaL_checkinteger( L, 2 );

	luaL_argcheck( L, 1 <= idx && idx <= a->len, 2,
                       "index out of range");
	/// return element address
	return &a->v[ idx - 1 ];
}


/**--------------------------------------------------------------------------
 * gets a value from the array.
 * \param   L  lua Virtual Machine.
 * \lparam  struct t_nry.
 * \lparam  int           array index.
 * \lreturn int           Value stored at index.
 * \return  int  # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_nry__index( lua_State *L )
{
	lua_pushinteger( L, *t_nry_getelem( L ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * sets a value from the array.
 * \param  L  Lua Virtual Machine.
 * \lparam struct t_nry.
 * \lparam int           array index.
 * \lparam int           Value to stored at index.
 * \return int  # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_nry__newindex( lua_State *L )
{
	int n_val = luaL_checknumber ( L, 3 );
	*t_nry_getelem( L ) = n_val;
	return 0;
}


/**--------------------------------------------------------------------------
 * Reverse the array.
 * \param   L  lua Virtual Machine.
 * \lparam  struct t_nry.
 * \lparam  int           array index.
 * \lreturn int           Value stored at index.
 * \return  int  # of values pushed onto the stack.
 *  -------------------------------------------------------------------------*/
static int
lt_nry_reverse( lua_State *L )
{
	struct t_nry *a = t_nry_check_ud( L, 1, 1 );
	size_t        n;      ///< runner
	int           t;      ///< temp variable

	for (n=0; n<a->len; n++)
	{
		t                  = a->v[ n ];
		a->v[ n ]          = a->v[ a->len - n ];
		a->v[ a->len - n ] = t;
	}

	return 0;
}


/**--------------------------------------------------------------------------
 * Length of a T.Numarray instance as seen by Lua.
 * \param   L    The lua state.
 * \lreturn int  length of Numarray Instance.
 * \return  int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_nry__len( lua_State *L )
{
	struct t_nry *a = t_nry_check_ud( L, 1, 1 );
	lua_pushinteger( L, (int) a->len );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a Numarray instance.
 * \param   L     The lua state.
 * \lreturn string    formatted string representing NUmarray instance.
 * \return  int      # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int lt_nry__tostring( lua_State *L )
{
	struct t_nry *a = t_nry_check_ud( L, 1, 1 );
	lua_pushfstring( L, "T.Numarray[%d]: %p", a->len, a );
	return 1;
}


/**--------------------------------------------------------------------------
 * Numarray class metamethods library definition 
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_nry_fm [] = {
	{ "__call",        lt_nry__Call},
	{ NULL,            NULL}
};


/**--------------------------------------------------------------------------
 * Numarray class functions library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_nry_cf [] = {
	{ "new",       lt_nry_New },
	{ NULL, NULL }
};


/**--------------------------------------------------------------------------
 * Numarray object method library definition
 * Assigns Lua available names to C-functions on T.Numarray instances
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_nry_m [] = {
	{ "reverse",   lt_nry_reverse },
	// allow metamethods to be accessed in a more traditional OOP style
	{ "length",    lt_nry__len },
	{ "toString",  lt_nry__tostring },
	{ NULL, NULL }
};


/**--------------------------------------------------------------------------
 * \brief  pushes Numarray library onto the stack
 * Used to make it part of the entire lua-t library
 *  - creates Metatable with methods for objects
 *  - creates Metatable with functions for class
 * \param  L    The lua state.
 * \return int  # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_t_nry( lua_State *L )
{
	// T.Numarray stance metatable
	luaL_newmetatable( L, "T.Numarray" );

	// this is the metatables general __index; it is used to find
	// __len, __tostring, reverse, ...
	luaL_newlib( L, t_nry_m );
	// this is the metatables metatable, ised to do the actual array based access
	lua_newtable( L );
	lua_pushcfunction( L, lt_nry__index );
	lua_setfield( L, -2, "__index" );
	lua_pushcfunction( L, lt_nry__newindex );
	lua_setfield( L, -2, "__newindex" );
	t_stackDump( L );
	lua_setmetatable( L, -2 );
	// set t_nry_m as __index of Metatable
	lua_setfield( L, -2, "__index" );

	lua_pushcfunction( L, lt_nry__len );
	lua_setfield( L, -2, "__len" );
	lua_pushcfunction( L, lt_nry__tostring );
	lua_setfield( L, -2, "__tostring" );
	t_stackDump( L );
	lua_pop( L, 1 );        // remove metatable from stack

	// T.Numarray class
	luaL_newlib( L, t_nry_cf );
	luaL_newlib( L, t_nry_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

