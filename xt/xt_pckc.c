/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_pckc.c
 * \brief     create a packer Struct/Array
 *            combinators for packers to create structures
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */


#include "xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  xt.Pack.
 * \lreturn userdata of xt.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pckc_Struct( lua_State *luaVM )
{
	int                i;      ///< iterator for going through the arguments
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;      ///< temporary packer/struct for iteration
	struct xt_pck     *cp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	cp     = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ) );
	cp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	cp->sz = 0;
	cp->oC = 0;
	cp->t  = XT_PCK_STRUCT;

	lua_createtable( luaVM, cp->n, cp->n ); // Stack: ..., Struct,idx

	for (i=1; i<lua_gettop( luaVM )-1; i++)
	{
		// are we creating a named element?
		if (lua_istable( luaVM, i))
		{
			// Stack gymnastic:
			// enter into the idx table [name]=pack and [id]=name
			lua_pushnil( luaVM );
			if (!lua_next( luaVM, i ))    // Stack: ...,Struct,idx,name,Pack
				return xt_push_error( luaVM, "the table argument must contain one key/value pair.");
		}
		else
		{
			lua_pushfstring( luaVM, "%d", i ); // Stack: ...,Struct,idx,"i"
			lua_pushvalue( luaVM, i );         // Stack: ...,Struct,idx,"i",Pack
		}
		// populate idx table
		lua_insert( luaVM, -2);               // Stack: ...,Struct,idx,Pack,name
		lua_pushvalue( luaVM, -1);            // Stack: ...,Struct,idx,Pack,name,name
		lua_rawseti( luaVM, -4, i );          // Stack: ...,Struct,idx,Pack,name
		lua_pushvalue( luaVM, -2);            // Stack: ...,Struct,idx,Pack,name,Pack
		lua_rawset( luaVM, -4 );              // Stack: ...,Struct,idx,Pack
		// Stack: ...,Struct,idx,Pack/Struct
		p = xt_pckc_check_ud( luaVM, -1 );    // allow xt.Pack or xt.Pack.Struct
		// handle Bit type packers
		p->oC = cp->sz;                       // offset within combinator
		if (XT_PCK_BIT == p->t)
		{
			p->oB = bc%8;
			if ((bc + p->lB)/8 > bc/8)
				cp->sz += 1;
			bc = bc + p->lB;
		}
		else
		{
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must always be grouped by byte size" );
			else
				bc = 0;
			cp->sz += p->sz;
		}
		lua_pop( luaVM, 1 );   // pop packer from stack for next round
	}
	cp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX);
	luaL_getmetatable( luaVM, "xt.Pack.Struct" ); // Stack: ...,xt.Pack.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * Create a  xt.Pack.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  xt.Pack, xt.Pack.Struct, xt.Pack.Array .
 * \lparam  len              Number of elements in the array.
 * \lreturn userdata of xt.Pack.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pckc_Array( lua_State *luaVM )
{
	size_t             i;      ///< iterator for creation of idx
	struct xt_pck     *p;      ///< packer
	struct xt_pck     *ap;     ///< array userdata to be created

	p = xt_pckc_check_ud( luaVM, -2 );    // allow x.Pack or xt.Pack.Struct
	ap     = (struct xt_pck *) lua_newuserdata( luaVM, sizeof( struct xt_pck ) );
	ap->n  = luaL_checkint( luaVM, -2 );       // how many elements in the array
	ap->sz = ap->n * sizeof( p->sz );
	ap->oC = 0;
	ap->t  = XT_PCK_ARRAY;

	lua_newtable( luaVM );     // Stack: Pack,n,Array,idx
	for ( i=1; i<=ap->n; i++ )
	{
		lua_pushvalue( luaVM, 1 );    // Stack: Pack,n,Array,idx,Pack
		lua_rawseti( luaVM, -2, i );  // push references to the very same Packer to index table
	}

	ap->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX ); // pop the type and keep reference

	luaL_getmetatable( luaVM, "xt.Pack.Struct" );
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being an xt.Pack OR * xt.Pack.Struct/Array
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \lparam  userdata xt.Pack.Struct on the stack.
 * \return  xt_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct xt_pck *xt_pckc_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_testudata( luaVM, pos, "xt.Pack.Struct" );
	if (NULL != ud)
		return (struct xt_pck *) ud;
	ud = luaL_checkudata( luaVM, pos, "xt.Pack" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Pack.Struct` or `xt.Pack` expected" );
	return (struct xt_pck *) ud;
}


/**--------------------------------------------------------------------------
 * Read a Struct packer value.
 * \param   luaVM    The lua state.
 * \lparam  userdata xt.Pack.Struct instance.
 * \lparam  key      string/integer.
 * \lreturn userdata Pack or Struct instance.
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pckc__index( lua_State *luaVM )
{
	struct xt_pck *cp = xt_pckc_check_ud( luaVM, -2 );
	struct xt_pck *p;

	// Access the idx table by key to get numeric index onto stack
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, cp->iR );
	// if (XT_PCK_STRUCT==cp->t  && ! lua_tonumber( luaVM, -3 ))
	if (XT_PCK_STRUCT == cp->t)
	{
		if (lua_tonumber( luaVM, -2 ))
			lua_rawgeti( luaVM, -1, lua_tointeger( luaVM, -2 ) );
		else
			lua_pushvalue( luaVM, -2 );
		lua_rawget( luaVM, -2 );        // Stack: Struct, name, idx, Pack or name
		p = xt_pckc_check_ud( luaVM, -1);
	}
	// no checks -> if it didn't find something nil gets returned and that's expected
	return 1;
}


/**--------------------------------------------------------------------------
 * update a packer value in an xt.Pack.Struct.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pckc__newindex( lua_State *luaVM )
{
	xt_pckc_check_ud( luaVM, -2 );

	return xt_push_error( luaVM, "Packers are static and can't be updated!" );
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Dissassociates buffers from Packs.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Pack.Struct.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckc__gc( lua_State *luaVM )
{
	struct xt_pck *cp = xt_pckc_check_ud( luaVM, 1 );
	luaL_unref( luaVM, LUA_REGISTRYINDEX, cp->iR );
	return 0;
}


/**--------------------------------------------------------------------------
 * Length of Struct in bytes
 * \param  luaVM    lua Virtual Machine.
 * \lparam userdata xt.Pack.Struct instance.
 * \return int      # of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pckc__len( lua_State *luaVM )
{
	struct xt_pck *cp = xt_pckc_check_ud( luaVM, 1 );

	lua_pushinteger( luaVM, cp->sz );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a xt.Pack.Struct.
 * \param   luaVM      The lua state.
 * \lparam  xt_pck_s   user_data.
 * \lreturn string     formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pckc__tostring( lua_State *luaVM )
{
	struct xt_pck *p = xt_pckc_check_ud( luaVM, 1 );

	switch( p->t )
	{
		case XT_PCK_STRUCT:
			lua_pushfstring( luaVM, "xt.Pack{STRUCT[%d]:%d}: %p", p->n, p->sz, p );
			break;
		case XT_PCK_ARRAY:
			lua_pushfstring( luaVM, "xt.Pack{ARRAY[%d]:%d}: %p", p->n, p->sz, p );
			break;
		default:
			xt_push_error( luaVM, "Can't read value from unknown packer type" );
	}
	return 1;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the xt.Pack.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int xt_pckc_iter( lua_State *luaVM )
{
	struct xt_pck *cp = xt_pckc_check_ud( luaVM, lua_upvalueindex( 1 ) );
	int crs;

	crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) );
	crs++;
	if (crs > (int) cp->n)
		return 0;
	else
	{
		lua_pushinteger( luaVM, crs );
		lua_replace( luaVM, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, cp->iR );
	lua_rawgeti( luaVM, -2 ,crs );    // Stack: _idx, name
	lua_pushvalue( luaVM, -1);        // Stack: _idx, name, name
	lua_rawget( luaVM, -3 );          // Stack: _idx, name, Pack
	lua_remove( luaVM, -3 );          // remove idx table
	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the xt.Pack.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator xt.Pack.Struct.
 * \lreturn pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pckc__pairs( lua_State *luaVM )
{
	xt_pckc_check_ud( luaVM, 1 );
	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &xt_pckc_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Pack.Struct library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pckc( lua_State *luaVM )
{
	// xt.Pack.Struct instance metatable
	luaL_newmetatable( luaVM, "xt.Pack.Struct" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pckc__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pckc__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pckc__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lxt_pckc__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pckc__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_pckc__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
