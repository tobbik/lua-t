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
int lxt_pck_Sequence( lua_State *luaVM )
{
	int                i;      ///< iterator for going through the arguments
	size_t             sz=0;   ///< tally up the size of all elements in the Struct
	size_t             bc=0;   ///< count bitSize for bit type packers
	struct xt_pck     *p;
	struct xt_pck_seq *sq;

	// size = sizof(...) -1 because the array has already one member
	sz = sizeof( struct xt_pck_seq ) + (lua_gettop( luaVM ) - 1) * sizeof( struct xt_pck );
	sq = (struct xt_pck_seq *) lua_newuserdata( luaVM, sz );
	sq->n = lua_gettop( luaVM )-1;  // number of elements on stack -1 (the seq userdata)
	sq->buf_ref = LUA_NOREF;
	memset( sq->p, 0, sq->n * sizeof( struct xt_pck ));

	sz = 0;

	for (i=0; i<lua_gettop( luaVM )-1; i++)
	{
		p = xt_pack_check_ud( luaVM, i+1 );

		sq->p[ i ] = *p;
		// handle Bit type packers
		if (XT_PACK_BIT == sq->p[ i ].type)
		{
			sq->p[ i ].bofs = bc%8;
			if ((bc + sq->p[ i ].blen)/8 > bc/8)
				sz += 1;
			bc = bc + sq->p[ i ].blen;
		}
		else
		{
			if (bc%8)
				xt_push_error( luaVM, "bitsized fields must always be grouped by byte size " );
			else
				bc = 0;
			sz = sz + sq->p[ i ].sz;
		}
	}
	luaL_getmetatable( luaVM, "xt.Packer.Sequence" ); // Stack: ...,xt.Packer.Sequence
	lua_setmetatable( luaVM, -2 ) ;

	sq->sz = sz;

	return 1;
}


/**--------------------------------------------------------------------------
 * \brief   check a value on the stack for being a Test
 * \param   luaVM    The lua state.
 * \param   int      position on the stack
 * \lparam  the Test table on the stack
 * \lreturn leaves the test table on the stack
 * --------------------------------------------------------------------------*/
struct xt_pack_seq *xt_pack_seq_check_ud( lua_State *luaVM, int pos )
{
	void *ud = luaL_checkudata( luaVM, pos, "xt.Packer.Sequence" );
	luaL_argcheck( luaVM, ud != NULL, pos, "`xt.Packer.Sequence` expected" );
	return (struct xt_pack_seq *) ud;
}


/**--------------------------------------------------------------------------
 * read a sequence packer value
 * \param   luaVM    The lua state.
 * \lparam  xt.Packer.Sequence instance
 * \lparam  key   integer
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pack_seq__index( lua_State *luaVM )
{
	struct xt_pack_seq *sq  = xt_pack_seq_check_ud( luaVM, -2 );
	size_t              idx = luaL_checkint( luaVM, -1 );

	// Stack: Sequence, index
	luaL_argcheck( luaVM, idx <= sq->n, -1,
		"Index for xt.Packer.Sequence access must be smaller than number of Packers in sequence" );

	if (NULL == sq->p[ idx-1 ].b)
		return xt_push_error( luaVM, "Can only read data from initialized data sequences" );

	return xt_pack_read( luaVM, &(sq->p[ idx-1 ]), sq->p[ idx-1 ].b );
}


/**--------------------------------------------------------------------------
 * update a packer value
 * \param   luaVM    The lua state.
 * \lparam  Combinator instance
 * \lparam  key   string
 * \lparam  value LuaType
 * \return  The # of items pushed to the stack.
 * --------------------------------------------------------------------------*/
static int lxt_pack_seq__newindex( lua_State *luaVM )
{
	struct xt_pack_seq *sq  = xt_pack_seq_check_ud( luaVM, -3 );
	size_t              idx = luaL_checkint( luaVM, -2 );
	int                 retVal;

	// Stack: Sequence, index, value
	luaL_argcheck( luaVM, idx <= sq->n, -1,
		"Index for xt.Packer.Sequence access must be smaller than number of Packers in sequence" );

	if (NULL == sq->p[ idx ].b)
		return xt_push_error( luaVM, "Can only write data to initialized data sequences" );

	retVal = xt_pack_write( luaVM, &(sq->p[ idx ]), sq->p[ idx ].b );
	if (retVal)
		return retVal;
	else
		return 0;
}


/**--------------------------------------------------------------------------
 * Attach a buffer to an xt.Packer.Sequence.
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Packer.Sequence.
 * \lparam struct xt_buf.
 * \lparam pos    position in xt_buf.
 * \return integer number of values left on te stack.
 *  -------------------------------------------------------------------------*/
static int lxt_pack_seq__call( lua_State *luaVM )
{
	struct xt_pack_seq *sq = xt_pack_seq_check_ud( luaVM, 1 );
	struct xt_buf       *b;
	int                  pos = 0;
	size_t               i;       ///< the iterator for all fields
	size_t               sz  = 0;  ///< iterate over the size to assign proper buffer position

	if (lua_isnoneornil( luaVM, 2))
	{
		if (LUA_NOREF != sq->buf_ref)
			luaL_unref( luaVM, LUA_REGISTRYINDEX, sq->buf_ref ); // remove buffer at buf_ref from registry
		for (i=0; i < sq->n; i++)
		{
			sq->p[ i ].b = NULL;
		}
	}
	else
	{
		if (lua_isnumber( luaVM, 3))
			pos = luaL_checkint( luaVM, 3 );
		b  = xt_buf_check_ud( luaVM, 2 );
		luaL_argcheck( luaVM, 0 <= pos && pos <= (int) b->len, 3,
								  "xt.Buffer position must be > 0 or < #buffer" );
		lua_pushvalue( luaVM, 2 );
		sq->buf_ref = luaL_ref( luaVM, LUA_REGISTRYINDEX ); // recieve registry index, and pop b from stack

		// TODO: Buffer size must equal Struct size?
		for (i=0; i < sq->n; i++)
		{
			sq->p[ i ].b = &(b->b[ sz ]);
			sz += sq->p[ i ].sz;
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
static int lxt_pack_seq__gc( lua_State *luaVM )
{
	struct xt_pack_seq *sq = xt_pack_seq_check_ud( luaVM, 1 );
	size_t               i;       ///< the iterator for all fields

	if (LUA_NOREF != sq->buf_ref)
		luaL_unref( luaVM, LUA_REGISTRYINDEX, sq->buf_ref ); // remove buffer at buf_ref from registry
	for (i=0; i < sq->n; i++)
	{
		sq->p[ i ].b = NULL;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Length of Struct in bytes
 * \param  luaVM lua Virtual Machine.
 * \lparam table xt.Combinator.
 * \return integer number of values left on te stack.
 * -------------------------------------------------------------------------*/
static int lxt_pack_seq__len( lua_State *luaVM )
{
	struct xt_pack_seq *sq = xt_pack_seq_check_ud( luaVM, 1 );

	lua_pushinteger( luaVM, sq->n );
	return 1;
}


/**--------------------------------------------------------------------------
 * ToString representation of a packer sequence.
 * \param   luaVM         The lua state.
 * \lparam  xt_pack_seq   user_data.
 * \lreturn string        formatted string representing Sequence.
 * \return  The number of results to be passed back to the calling Lua script.
 * --------------------------------------------------------------------------*/
static int lxt_pack_seq__tostring (lua_State *luaVM)
{
	struct xt_pack_seq *sq = xt_pack_seq_check_ud (luaVM, 1);
	lua_pushfstring( luaVM, "xt.Packer.Sequence{%d}: %p", sq->n, sq );
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
LUAMOD_API int luaopen_xt_pack_seq( lua_State *luaVM )
{
	// xt.Packer.Sequence instance metatable
	luaL_newmetatable( luaVM, "xt.Packer.Sequence" );   // stack: functions meta
	lua_pushcfunction( luaVM, lxt_pack_seq__index );
	lua_setfield( luaVM, -2, "__index" );
	lua_pushcfunction( luaVM, lxt_pack_seq__newindex );
	lua_setfield( luaVM, -2, "__newindex" );
	lua_pushcfunction( luaVM, lxt_pack_seq__call );
	lua_setfield( luaVM, -2, "__call" );
	lua_pushcfunction( luaVM, lxt_pack_seq__tostring );
	lua_setfield( luaVM, -2, "__tostring" );
	lua_pushcfunction( luaVM, lxt_pack_seq__len );
	lua_setfield( luaVM, -2, "__len" );
	lua_pushcfunction( luaVM, lxt_pack_seq__gc );
	lua_setfield( luaVM, -2, "__gc" );
	lua_pop( luaVM, 1 );        // remove metatable from stack
	return 0;
}
