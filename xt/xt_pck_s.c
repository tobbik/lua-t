/**
 * \file     create a packer Sequence or Array
 * \brief    combinators for packers to create structures etc
*/
#include <memory.h>               // memset

#include "l_xt.h"
#include "xt_buf.h"


/**--------------------------------------------------------------------------
 * Create a  xt.Packer.Sequence Object and put it onto the stack.
 * \param   luaVM  The lua state.
 * \lparam  ... multiple of type  xt.Packer 
 * \lreturn userdata of xt.Packer.Sequence.
 * \return  # of results  passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
int lxt_pck_Struct( lua_State *luaVM )
{
	int                i;      ///< iterator for going through the arguments
	int                isNamed; ///< set true if element goes into idx table
	size_t             sz=0;   ///< tally up the size of all elements in the Struct
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;      ///< temporary packer for iteration
	struct xt_pck_s   *ps;     ///< temporary struct for iteration
	struct xt_pck_s   *sp;     ///< the userdata this constructor creates

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct xt_pck_s ) + (lua_gettop( luaVM ) - 1) * sizeof( int );
	sp = (struct xt_pck_s *) lua_newuserdata( luaVM, sz );
	sp->n = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the seq userdata)
	sp->buf_ref = LUA_NOREF;
	memset( sp->p, 0, sp->n * sizeof( int ));

	lua_newtable( luaVM ); // Stack: ..., Struct,idx

	for (i=1; i<lua_gettop( luaVM )-1; i++)
	{
		// are we creating a named element?
		if (lua_istable( luaVM, i))
		{
			lua_pushnil( luaVM );
			if (lua_next( luaVM, i ))
			{
				// Stack: ...,Struct,idx,name,Pack
				// swap name and element
				lua_insert( luaVM, -2 );      // Stack: ...,Struct,idx,Pack,name
				lua_pushinteger( luaVM, i );  // Stack: ...,Struct,idx,Pack,name,i
				// push name and order to idx table idx[name] = i
				lua_rawset( luaVM, -3 );      // Stack: ...,Struct,idx,Pack
				lua_replace( luaVM, i );      // move packer into position on stack
				isNamed = 1;   // that means we have at least one named member
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
	if (isNamed) 
		sp->idx_ref = luaL_ref( luaVM, LUA_REGISTRYINDEX);
	else
	{
		sp->idx_ref = LUA_NOREF;
		lua_pop( luaVM, 1 );
	}
	luaL_getmetatable( luaVM, "xt.Packer.Struct" ); // Stack: ...,xt.Packer.Sequence
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
struct xt_pck_s *xt_pck_s_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Packer.Struct" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Packer.Struct` expected" );
	return (struct xt_pck_s *) ud;
}


/**--------------------------------------------------------------------------
 * read a sequence packer value
 * \param   luaVM    The lua state.
 * \lparam  xt.Packer.Sequence instance
 * \lparam  key   integer
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pck_s__index( lua_State *luaVM )
{
	struct xt_pck_s *sp  = xt_pck_s_check_ud( luaVM, -2 );
	struct xt_pck   *p;

	// Access the idx table by key to get numeric index onto stack
	if (0 == lua_tonumber( luaVM, -1 ))
	{
		if (LUA_NOREF == sp->idx_ref)
		{
			lua_pushnil( luaVM );
			return 1;
		}
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->idx_ref );
		lua_pushvalue( luaVM, -2 );    // push the name again
		lua_rawget( luaVM, -2 );       // Stack: Struct, name, id
		lua_replace( luaVM, -2 );      // Stack: Struct, id
	}

	// Stack: Struct, index
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -1 ) <= sp->n, -1,
		"Index for xt.Packer.Struct access must be smaller than number of Packers in sequence" );
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
		if (LUA_NOREF == sp->idx_ref)
		{
			lua_pushnil( luaVM );
			return 1;
		}
		lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->idx_ref );
		lua_pushvalue( luaVM, -2 );    // push the name again
		lua_rawget( luaVM, -2 );       // Stack: Struct, name, id
		lua_replace( luaVM, -2 );      // Stack: Struct, id
	}

	// Stack: Sequence, index, value
	luaL_argcheck( luaVM, (size_t) luaL_checkint( luaVM, -2 ) <= sp->n, -2,
		"Index for xt.Packer.Sequence access must be smaller than number of Packers in sequence" );
	lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ luaL_checkint( luaVM, -2 ) - 1 ] );

	if (NULL != luaL_testudata( luaVM, -1, "xt.Packer.Struct" ))
		return xt_push_error( luaVM, "Can't write the value to a struct element which is itself a struct" );

	p = xt_pck_check_ud( luaVM, -1 );
	if (NULL == p->b)
		return xt_push_error( luaVM, "Can only read data from initialized data struct" );
	else
		lua_pushvalue( luaVM, -2); // repush new value; xt_pck_write expects it as last elem on stack

	stackDump(luaVM);
	if ((retVal = xt_pck_write( luaVM, p, p->b )) != 0)
		return retVal;
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to an xt.Packer.Struct.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Packer.Struct.
 * \lparam struct xt_buf.
 * \lparam pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
int lxt_pck_s__call( lua_State *luaVM )
{
	struct xt_pck_s *sp  = xt_pck_s_check_ud( luaVM, 1 );
	struct xt_pck   *p;
	struct xt_pck   *ps;
	struct xt_buf   *b;
	int              pos = 0;  ///< moving position in the puffer
	size_t           i;        ///< the iterator for all fields

	// Delete all references -> this struct the is not associated;
	if (lua_isnoneornil( luaVM, 2 ))
	{
		if (LUA_NOREF != sp->buf_ref)
			luaL_unref( luaVM, LUA_REGISTRYINDEX, sp->buf_ref );  // remove buffer at buf_ref from registry
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
	}
	else
	{
		if (lua_isnumber( luaVM, 3 ))
			pos = luaL_checkint( luaVM, 3 );
		b  = xt_buf_check_ud( luaVM, 2 );
		luaL_argcheck( luaVM, 0 <= pos && pos <= (int) b->len, 3,
								  "xt.Buffer position must be > 0 or < #buffer" );
		lua_pushvalue( luaVM, 2 );
		sp->buf_ref = luaL_ref( luaVM, LUA_REGISTRYINDEX ); // recieve registry index, and pop b from stack

		// TODO: Buffer size must equal Struct size?
		for (i=0; i < sp->n; i++)
		{
			printf("%d\n",sp->p[i]);
			lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->p[ i ] );
			stackDump( luaVM );
			ps = luaL_testudata( luaVM, -1, "xt.Packer.Struct" );
			if (NULL != ps)
			{
				lua_pushcfunction( luaVM, lxt_pck_s__call );
				lua_insert( luaVM, -2 );  // move function before struct
				lua_rawgeti( luaVM, LUA_REGISTRYINDEX, sp->buf_ref );
				lua_pushinteger( luaVM, pos );
				lua_call( luaVM, 3, 0 );
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
 * \lparam table xt.Packer.Sequence.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pck_s__gc( lua_State *luaVM )
{
	struct xt_pck_s *sp = xt_pck_s_check_ud( luaVM, 1 );
	struct xt_pck   *p;
	struct xt_pck_s *ps;
	size_t           i;       ///< the iterator for all fields

	if (LUA_NOREF != sp->buf_ref)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, sp->buf_ref ); // remove buffer at buf_ref from registry
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
 * ToString representation of a packer sequence.
 * \param   luaVM         The lua state.
 * \lparam  xt_pack_seq   user_data.
 * \lreturn string        formatted string representing Sequence.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pck_s__tostring (lua_State *luaVM)
{
	struct xt_pck_s *sp = xt_pck_s_check_ud (luaVM, 1);
	lua_pushfstring( luaVM, "xt.Packer.Struct{%d}: %p", sp->n, sp );
	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   pushes the xt.Packer.Sequence library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   luaVM     The lua state.
 * \lreturn string    the library
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
LUAMOD_API int luaopen_xt_pck_s( lua_State *luaVM )
{
	// xt.Packer.Sequence instance metatable
	luaL_newmetatable( luaVM, "xt.Packer.Struct" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pck_s__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pck_s__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
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
