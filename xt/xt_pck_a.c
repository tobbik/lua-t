/**
 * \file     create a packer Array
 * \brief    combinators for packers to create array
*/
#include <memory.h>               // memset

#include "l_xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * Create a  xt.Packer.Array Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  type identifier  xt.Packer, xt.Packer.Struct, xt.Packer.Array 
 * \lparam  len              Number of elements in the array.
 * \lreturn userdata of xt.Packer.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pck_Array( lua_State *luaVM )
{
	int                ref;    ///< LUA_REGISTRYINDEX reference for the type
	size_t             sz=0;   ///< tally up the size of all elements in the Struct
	int                n=0;    ///< how many members in the array
	struct xt_pck     *p;      ///< packer
	struct xt_pck_s   *ps;     ///< struct
	struct xt_pck_a   *pa;     ///< array
	struct xt_pck_a   *ap;     ///< array userdata to be created

	n = luaL_checkint( luaVM, -1 );    // how many elements in the array
	lua_pop( luaVM, 1 );               // pop the size
	// Testing for types
	p  = luaL_testudata( luaVM, -1, "xt.Packer" );
	if (NULL != p)
		sz = n * p->sz;
	ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
	if (NULL != ps)
		sz = n * ps->sz;
	pa = luaL_testudata( luaVM, -1, "xt.Packer.Array" );
	if (NULL != pa)
		sz = n * pa->sz;

	luaL_argcheck(luaVM, (0 != sz), -2, "First argument must be a xt.Packer, xt.Packer.Struct or xt.Packer.Array" );
	ref = luaL_ref( luaVM, LUA_REGISTRYINDEX ); // pop the type and keep reference

	ap     = (struct xt_pck_a *) lua_newuserdata( luaVM, sizeof( struct xt_pck_a ) );
	ap->n  = n;
	ap->sz = sz;
	ap->tR = ref;
	ap->bR = LUA_NOREF;
	ap->bP = 0;
	luaL_getmetatable( luaVM, "xt.Packer.Array" );
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \lparam  the Test table on the stack
 * \lreturn leaves the test table on the stack
 * --------------------------------------------------------------------------*/
struct xt_pck_a *xt_pck_a_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Packer.Array" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Packer.Array` expected" );
	return (struct xt_pck_a *) ud;
}


/**--------------------------------------------------------------------------
 * Read a Array packer value.
 * \param   luaVM   The lua state.
 * \lparam  xt.Packer.Array instance
 * \lparam  index   integer
 * \lreturn value   value from buffer according to packer definition
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_a__index( lua_State *luaVM )
{
	struct xt_pck_a *ap = xt_pck_a_check_ud( luaVM, -2 );
	struct xt_buf   *b;
	struct xt_pck   *p;
	struct xt_pck_s *ps;
	struct xt_pck_s *psc;
	struct xt_pck_a *pa;
	struct xt_pck_a *pac;

	// Stack: Struct, index
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -1 ) <= ap->n, -1,
		"Index for xt.Packer.Array access must be smaller than number of Packers in Array" );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, ap->tR );

	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer" ))
	{
		p = xt_pck_check_ud( luaVM, -1 );
		if (LUA_NOREF == ap->bR)
			return xt_push_error( luaVM, "Can't read from an uninitialized array" );
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, ap->bR );
		b = xt_buf_check_ud( luaVM, -1);
		lua_pop( luaVM, 2);    // pop type and buffer

		return xt_pck_read( luaVM, p, &(b->b[ p->sz*(luaL_checkint( luaVM, -1 ) -1) ]) );
	}
	// if it's not give me a copy of the type with an adjusted buffer attached
	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer.Struct" ))
	{
		ps  = xt_pck_s_check_ud( luaVM, -1 );
		psc = (struct xt_pck_s *) lua_newuserdata( luaVM, sizeof( ps ) );
		psc = ps;
		psc->bP = (LUA_NOREF == psc->bR) ? 0 : psc->bP + psc->sz * (luaL_checkint( luaVM, -2 ) -1);
		return 1;
	}
	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer.Array" ))
	{
		pa  = xt_pck_a_check_ud( luaVM, -1 );
		pac = (struct xt_pck_a *) lua_newuserdata( luaVM, sizeof( pa ) );
		pac = pa;
		pac->bP = (LUA_NOREF == pac->bR) ? 0 : pac->bP + pac->sz * (luaL_checkint( luaVM, -2 ) -1);
		return 1;
	}
	return xt_push_error( luaVM, "Wrong Datatype in xt.Packer.Array" );
}


/**--------------------------------------------------------------------------
 * update a packer value in an xt.Packer.Array.
 * \param   luaVM    The lua state.
 * \lparam  userdata xt.Packer.Array instance
 * \lparam  index    integer
 * \lparam  value    LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_a__newindex( lua_State *luaVM )
{
	struct xt_pck_a *ap  = xt_pck_a_check_ud( luaVM, -3 );
	struct xt_pck   *p;
	struct xt_buf   *b;
	int              retVal;

	// Stack: Struct, index, value
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -2 ) <= ap->n, -2,
		"Index for xt.Packer.Array access must be smaller than number of Packers in Array" );
	if (LUA_NOREF == ap->bR)
		return xt_push_error( luaVM, "Can only write data to an initialized data Array" );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, ap->bR );
	b = xt_buf_check_ud( luaVM, -1);
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, ap->tR );
	p = xt_pck_check_ud( luaVM, -1 );
	lua_pop( luaVM, 2);    // buffer and type

	if ((retVal = xt_pck_write( luaVM, p, &(b->b[ p->sz*(luaL_checkint( luaVM, -2 ) -1) ]) )) != 0 )
		return retVal;
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to an xt.Packer.Array.
 * \param  luaVM    lua Virtual Machine.
 * \lparam userdata xt.Packer.Array.
 * \lparam userdata xt.Buffer.
 * \lparam pos      position in xt_buf.
 * \return integer  number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_a__call( lua_State *luaVM )
{
	struct xt_pck_a *ap;
	struct xt_buf   *b;
	int              pos = 0;  ///< moving position in the puffer

	if (lua_isnumber( luaVM, -1 ))
		pos = luaL_checkint( luaVM, -1 );
	else
		lua_pushinteger( luaVM, 0 );

	ap = xt_pck_a_check_ud( luaVM, -3 );

	// Delete all references -> this struct the is not associated;
	if (lua_isnoneornil( luaVM, -2 ))
	{
		if (LUA_NOREF != ap->bR)
			luaL_unref( luaVM, LUA_REGISTRYINDEX, ap->bR );  // remove buffer at buf_ref from registry
		ap->bR = LUA_NOREF;
		ap->bP = 0;
	}
	else
	{
		b  = xt_buf_check_ud( luaVM, -2 );
		lua_pop( luaVM, 1);
		ap->bR = luaL_ref( luaVM, LUA_REGISTRYINDEX );
		ap->bP = pos;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Dissassociates buffers from Packers.
 * \param  luaVM     lua Virtual Machine.
 * \lparam userdata  xt.Packer.Array.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pck_a__gc( lua_State *luaVM )
{
	struct xt_pck_a *ap = xt_pck_a_check_ud( luaVM, 1 );

	if (LUA_NOREF != ap->bR)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, ap->bR ); // remove buffer at buf_ref from registry
	ap->bP = 0;
	return 0;
}


/**--------------------------------------------------------------------------
 * Length of Array in elements
 * \param  luaVM     lua Virtual Machine.
 * \lparam userdata  xt.Packer.Array.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pck_a__len( lua_State *luaVM )
{
	struct xt_pck_a *ap = xt_pck_a_check_ud( luaVM, 1 );

	lua_pushinteger( luaVM, ap->n );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a packer Array.
 * \param   luaVM     lua Virtual Machine.
 * \lparam  userdata  xt.Packer.Array.
 * \lreturn string     formatted string representing Array.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck_a__tostring (lua_State *luaVM)
{
	struct xt_pck_a *ap = xt_pck_a_check_ud (luaVM, 1);
	lua_pushfstring( luaVM, "xt.Packer.Array{%d}: %p", ap->n, ap );
	return 1;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the xt.Packer.Array.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int xt_pck_a_iter( lua_State *luaVM )
{
	struct xt_pck_a *ap = xt_pck_a_check_ud( luaVM, lua_upvalueindex( 1 ) );
	//struct xt_pc_a  *pa;
	//struct xt_pck   *p;
	int crs;

	crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) );
	crs++;
	if (crs > (int) ap->n)
		return 0;
	lua_pushcfunction( luaVM, lxt_pck_a__index );
	lua_pushvalue( luaVM, lua_upvalueindex( 1 ) );
	lua_pushinteger( luaVM, crs );
	lua_call( luaVM, 2, 1 );

	lua_pushinteger( luaVM, crs );
	lua_replace( luaVM, lua_upvalueindex( 2 ) );
	lua_pushinteger( luaVM, crs );
	lua_replace( luaVM, -3);

	return 2;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the xt.Packer.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator xt.Packer.Struct.
 * \lreturn pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_a__pairs( lua_State *luaVM )
{
	xt_pck_a_check_ud( luaVM, -1 );
	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &xt_pck_a_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}



/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Packer.Array library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pck_a( lua_State *luaVM )
{
	// xt.Packer.Struct instance metatable
	luaL_newmetatable( luaVM, "xt.Packer.Array" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pck_a__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pck_a__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pck_a__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lxt_pck_a__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_pck_a__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pck_a__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_pck_a__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
