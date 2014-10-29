/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      xt_pck_s.c
 * \brief     create a packer Struct
 *            combinators for packers to create structures
 * \author    tkieslich
 * \copyright See Copyright notice at the end of xt.h
 */

#include <memory.h>               // memset

#include "xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * Create a  xt.Packer.Struct Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  xt.Packer 
 * \lreturn userdata of xt.Packer.Struct.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pck_Struct( lua_State *luaVM )
{
	int                i;      ///< iterator for going through the arguments
	int                isIdx=0;///< set true if element goes into idx table
	size_t             sz=0;   ///< tally up the size of all elements in the Struct
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;      ///< temporary packer for iteration
	struct xt_pck_s   *ps;     ///< temporary struct for iteration
	struct xt_pck_s   *sp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	sz     = sizeof( struct xt_pck_s ) + (lua_gettop( luaVM ) - 1) * sizeof( int );
	sp     = (struct xt_pck_s *) lua_newuserdata( luaVM, sz );
	sp->n  = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the Struct userdata)
	sp->bR = LUA_NOREF;
	sp->sz = 0;
	memset( sp->p, 0, sp->n * sizeof( int ));

	lua_newtable( luaVM ); // Stack: ..., Struct,idx

	for (i=1; i<lua_gettop( luaVM )-1; i++)
	{
		// are we creating a named element?
		if (lua_istable( luaVM, i))
		{
			// Stack gymnastic:
			// enter into the idx table [neme]=id and [id]=name
			lua_pushnil( luaVM );
			if (lua_next( luaVM, i ))
			{
				// swap name and element      // Stack: ...,Struct,idx,name,Pack
				lua_insert( luaVM, -2 );      // Stack: ...,Struct,idx,Pack,name
				lua_pushinteger( luaVM, i );  // Stack: ...,Struct,idx,Pack,name,i
				lua_pushinteger( luaVM, i );  // Stack: ...,Struct,idx,Pack,name,i,i
				lua_pushvalue( luaVM, -3 );   // Stack: ...,Struct,idx,Pack,name,i,i,name
				// push name and order to idx table idx[name] = i
				lua_rawset( luaVM, -6 );      // Stack: ...,Struct,idx,Pack,name,i
				lua_rawset( luaVM, -4 );      // Stack: ...,Struct,idx,Pack
				lua_replace( luaVM, i );      // move packer into position on stack
				isIdx = 1;   // that means we have at least one named member
			}
			else
			{
				return xt_push_error( luaVM, "the table argument must contain one key/value pair.");
			}
		}
		// Stack: ...,Struct,idx,Pack/Struct
		// are we dealing with a struct? If so, create packer, preserve reference to struct
		lua_pushvalue( luaVM, i);
		ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct");
		if (NULL != ps)
		{
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must always be grouped by byte size " );
			else
				bc = 0;
			sp->sz += ps->sz;
		}
		else            // are we dealing with a packer
		{
			p = xt_pck_check_ud( luaVM, -1 );
			// handle Bit type packers
			if (XT_PCK_BIT == p->t)
			{
				p->bofs = bc%8;
				if ((bc + p->blen)/8 > bc/8)
					sp->sz += 1;
				bc = bc + p->blen;
			}
			else
			{
				if (bc%8)
					xt_push_error( luaVM, "bitsized fields must always be grouped by byte size " );
				else
					bc = 0;
				sp->sz += p->sz;
			}
		}
		// Stack: ...,Struct,idx,Pack
		sp->p[ i-1 ] = luaL_ref( luaVM, LUA_REGISTRYINDEX );
	}
	if (isIdx)
		sp->iR = luaL_ref( luaVM, LUA_REGISTRYINDEX);
	else
	{
		sp->iR = LUA_NOREF;
		lua_pop( luaVM, 1 );
	}
	luaL_getmetatable( luaVM, "xt.Packer.Struct" ); // Stack: ...,xt.Packer.Struct
	lua_setmetatable( luaVM, -2 ) ;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   luaVM    The lua state.
 * \param   int      position on the stack.
 * \lparam  userdata xt.Packer.Struct on the stack.
 * \return  xt_pck_s pointer.
 * --------------------------------------------------------------------------*/
struct xt_pck_s *xt_pck_s_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Packer.Struct" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Packer.Struct` expected" );
	return (struct xt_pck_s *) ud;
}


/**--------------------------------------------------------------------------
 * Read a Struct packer value.
 * \param   luaVM   The lua state.
 * \lparam  xt.Packer.Struct instance
 * \lparam  key     string/integer
 * \lreturn value   value from buffer according to packer definition
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_s__index( lua_State *luaVM )
{
	struct xt_pck_s *sp  = xt_pck_s_check_ud( luaVM, -2 );
	struct xt_pck   *p;

	// Access the idx table by key to get numeric index onto stack
	if (0 == lua_tonumber( luaVM, -1 ))
	{
		if (LUA_NOREF == sp->iR)
		{
			lua_pushnil( luaVM );
			return 1;
		}
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->iR );
		lua_pushvalue( luaVM, -2 );    // push the name again
		lua_rawget( luaVM, -2 );       // Stack: Struct, name, id
		lua_replace( luaVM, -2 );      // Stack: Struct, id
	}

	// Stack: Struct, index
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -1 ) <= sp->n, -1,
		"Index for xt.Packer.Struct access must be smaller than number of Packers in Struct" );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ luaL_checkint( luaVM, -1 ) -1 ] );

	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer.Struct" ))
		return 1;

	p = xt_pck_check_ud( luaVM, -1 );
	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data struct" );

	return xt_pck_read( luaVM, p, p->b );
}


/**--------------------------------------------------------------------------
 * update a packer value in an xt.Packer.Struct.
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string/integer
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_s__newindex( lua_State *luaVM )
{
	struct xt_pck_s *sp  = xt_pck_s_check_ud( luaVM, -3 );
	struct xt_pck   *p;
	int              retVal;

	// Access the idx table by key to get numeric index onto stack
	if (0 == lua_tonumber( luaVM, -2 ))
	{
		if (LUA_NOREF == sp->iR)
		{
			return xt_push_error( luaVM, "can't write value to Struct whith unknown key" );
		}
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->iR );
		lua_pushvalue( luaVM, -3 );    // Stack: Struct,name,value,idx,name
		lua_rawget( luaVM, -2 );       // Stack: Struct,name,value,idx,id
		lua_replace( luaVM, -4 );      // Stack: Struct,id,value,idx
		lua_pop( luaVM, 1 );           // Stack: Struct,id,value
	}

	// Stack: Struct, index, value
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -2 ) <= sp->n, -2,
		"Index for xt.Packer.Struct access must be smaller than number of Packers in Struct" );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ luaL_checkint( luaVM, -2 ) - 1 ] );

	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer.Struct" ))
		return xt_push_error( luaVM, "Can't write the value to a struct element which is itself a struct" );

	p = xt_pck_check_ud( luaVM, -1 );
	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data struct" );
	else
		lua_pushvalue( luaVM, -2); // repush new value; xt_pck_write expects it as last elem on stack

	if ((retVal = xt_pck_write( luaVM, p, p->b )) != 0)
		return retVal;
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to an xt.Packer.Struct.
 * \param  luaVM    lua Virtual Machine.
 * \lparam userdata xt.Packer.Struct.
 * \lparam userdata xt.Buffer.
 * \lparam pos      position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_s__call( lua_State *luaVM )
{
	struct xt_pck_s *sp;
	struct xt_pck   *p;
	struct xt_pck   *ps;
	struct xt_buf   *b;
	int              pos = 0;  ///< moving position in the puffer
	size_t           i;        ///< the iterator for all fields

	if (lua_isnumber( luaVM, -1 ))
		pos = luaL_checkint( luaVM, -1 );
	else
		lua_pushinteger( luaVM, 0 );

	sp  = xt_pck_s_check_ud( luaVM, -3 );

	// Delete all references -> this struct the is not associated;
	if (lua_isnoneornil( luaVM, -2 ))
	{
		for (i=0; i < sp->n; i++)
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ i ] );
			ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
			if (NULL != ps)
			{
				lua_pushcfunction( luaVM, lxt_pck_s__call );
				lua_insert( luaVM, -2 );  // move function before struct
				lua_call( luaVM, 1, 0 );

			}
			p = xt_pck_check_ud( luaVM, -1 );
			p->b = NULL;
			lua_pop( luaVM, 1 );
		}
		if (LUA_NOREF != sp->bR)
			luaL_unref( luaVM, LUA_REGISTRYINDEX, sp->bR );  // remove buffer at buf_ref from registry
	}
	else
	{
		b  = xt_buf_check_ud( luaVM, -2 );
		luaL_argcheck( luaVM, 0 <= pos && pos <= (int) b->len, -3,
								  "xt.Buffer position must be > 0 or < #buffer" );
		lua_pushvalue( luaVM, -2 );
		sp->bR = luaL_ref( luaVM, LUA_REGISTRYINDEX ); // recieve registry index, and pop b from stack
		sp->bP = pos;

		// TODO: Buffer size must equal Struct size?
		for (i=0; i < sp->n; i++)
		{
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ i ] );
			ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
			if (NULL != ps)
			{
				lua_pushcfunction( luaVM, lxt_pck_s__call );
				lua_insert( luaVM, -2 );  // move function before struct
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->bR );
				lua_pushinteger( luaVM, pos );
				lua_call( luaVM, 3, 0 );
				continue;
			}
			p = xt_pck_check_ud( luaVM, -1 );
			p->b = &(b->b[ pos ]);
			pos += p->sz;
			lua_pop( luaVM, 1 );
		}
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Garbage Collector. Dissassociates buffers from Packers.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Packer.Struct.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pck_s__gc( lua_State *luaVM )
{
	struct xt_pck_s *sp = xt_pck_s_check_ud( luaVM, 1 );
	struct xt_pck   *p;
	struct xt_pck_s *ps;
	size_t           i;       ///< the iterator for all fields

	if (LUA_NOREF != sp->bR)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, sp->bR ); // remove buffer at buf_ref from registry
	for (i=0; i < sp->n; i++)
	{
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ i ] );
		ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
		if (NULL != ps)
		{
			lua_pushcfunction( luaVM, lxt_pck_s__call );
			lua_insert( luaVM, -2 );  // move function before struct
			lua_call( luaVM, 1, 0 );

		}
		p = xt_pck_check_ud( luaVM, -1 );
		p->b = NULL;
		lua_pop( luaVM, 1 );
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Length of Struct in bytes
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pck_s__len( lua_State *luaVM )
{
	struct xt_pck_s *sp = xt_pck_s_check_ud( luaVM, 1 );

	lua_pushinteger( luaVM, sp->n );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a packer Struct.
 * \param   luaVM      The lua state.
 * \lparam  xt_pck_s   user_data.
 * \lreturn string     formatted string representing Struct.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck_s__tostring (lua_State *luaVM)
{
	struct xt_pck_s *sp = xt_pck_s_check_ud (luaVM, 1);
	lua_pushfstring( luaVM, "xt.Packer.Struct{%d}: %p", sp->n, sp );
	return 1;
}


/**--------------------------------------------------------------------------
 * the actual iterate(next) over the xt.Packer.Struct.
 * It will return key,value pairs in proper order as defined in the constructor.
 * \param   luaVM lua Virtual Machine.
 * \lparam  cfunction.
 * \lparam  previous key.
 * \lparam  current key.
 * \lreturn current key, current value.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int xt_pck_s_iter( lua_State *luaVM )
{
	struct xt_pck_s *sp = xt_pck_s_check_ud( luaVM, lua_upvalueindex( 1 ) );
	struct xt_pc_s  *ps;
	struct xt_pck   *p;
	int crs;

	if (LUA_NOREF == sp->iR)
		return 0;
	crs = lua_tointeger( luaVM, lua_upvalueindex( 2 ) );
	crs++;
	if (crs > (int) sp->n)
		return 0;
	else
	{
		lua_pushinteger( luaVM, crs );
		lua_replace( luaVM, lua_upvalueindex( 2 ) );
	}
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->iR );
	lua_pushinteger( luaVM, crs );
	lua_rawget( luaVM, -2 );          // Stack: _idx,name
	if (lua_isnil( luaVM, -1))        // didn't find a named entry ...
		lua_pushinteger( luaVM, crs ); // ... push index instead
	lua_remove( luaVM, -2 );          // remove idx table

	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ crs-1 ] ); // Stack: name,object
	ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
	if (NULL != ps)
		return 2;
	p = xt_pck_check_ud( luaVM, -1 );
	lua_pop( luaVM, 1 );

	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data struct" );

	return xt_pck_read( luaVM, p, p->b ) + 1;
}


/**--------------------------------------------------------------------------
 * Pairs method to iterate over the xt.Packer.Struct.
 * \param   luaVM lua Virtual Machine.
 * \lparam  iterator xt.Packer.Struct.
 * \lreturn pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_s__pairs( lua_State *luaVM )
{
	xt_pck_s_check_ud( luaVM, -1 );
	lua_pushnumber( luaVM, 0 );
	lua_pushcclosure( luaVM, &xt_pck_s_iter, 2 );
	lua_pushvalue( luaVM, -1 );
	lua_pushnil( luaVM );
	return 3;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Packer.Struct library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pck_s( lua_State *luaVM )
{
	// xt.Packer.Struct instance metatable
	luaL_newmetatable( luaVM, "xt.Packer.Struct" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pck_s__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pck_s__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pck_s__pairs );
	lua_setfield( luaVM, -2, "__pairs" );
	lua_pushcfunction( luaVM, lxt_pck_s__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_pck_s__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pck_s__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_pck_s__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
